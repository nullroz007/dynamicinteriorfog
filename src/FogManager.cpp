#include <FogManager.h>
#include <Shader.h>

using namespace SKSE;
namespace NullMod {

bool isFog(RE::TESBoundObject* baseObject) {
  auto stat = baseObject->As<RE::TESObjectSTAT>();
  if (!stat) return false;

  for (const auto& effectPrefix : VALID_EFFECT_PREFIXES) {
    if (stat->model.contains(string_view(effectPrefix))) return true;
  }

  return false;
}

void FogManager::Serialize(json& j) {
  j = nlohmann::json{{"minimumAlpha", minAlpha}, {"fallbackAlpha", fallbackAlpha},
    {"fadeDistance", fadeDistance}, {"lerpFactor", lerpFactor}};
}

bool FogManager::CellIsTracked(RE::FormID cellID) {
  std::lock_guard<std::mutex> lock(trackedRefsLock);
  for (const auto& fogRef : trackedRefs) {
    auto ref = fogRef.handle.get();
    if (!ref) continue;
    if (ref->GetParentCell()->formID == cellID) return true;
  }

  return false;
}

ShapeHashMap FogManager::GetShapesForRef(RE::TESObjectREFR* ref) {
  ShapeHashMap results;
  auto container = ref ? ref->Get3D() : nullptr;
 
  RE::BSVisit::TraverseScenegraphGeometries(container, [&](RE::BSGeometry* geom) -> RE::BSVisit::BSVisitControl {
    auto triShape = geom->AsTriShape();
    if (!triShape) return RE::BSVisit::BSVisitControl::kContinue;

    ShapeRef shapeRef;
    auto& runtimeData = triShape->GetGeometryRuntimeData();
    for (int i = 0; i < RE::BSGeometry::States::kTotal; i++) {
      auto prop = runtimeData.properties[i].get();
      auto effectShader = prop ? netimmerse_cast<RE::BSEffectShaderProperty*>(prop) : nullptr;
      if (!effectShader) continue;

      auto matAlpha = effectShader->QMaterialAlpha();
      if (matAlpha >= 1.0) matAlpha = fallbackAlpha;
      matAlpha = std::max(matAlpha, minAlpha);
      ShaderData shaderData{i, matAlpha};
      shapeRef.push_back(std::move(shaderData));
    }

    results[geom->name] = std::move(shapeRef);
    return RE::BSVisit::BSVisitControl::kContinue;
  });

  return results;
}

void FogManager::TrackRefDeferred(RE::TESObjectREFR* ref, int attempts) {
  if (attempts > 120) {
    log::warn("Failed to load 3D for {:08X}", ref->formID);
    return;
  }

  auto shapes = GetShapesForRef(ref);
  if (shapes.empty()) {
    SKSE::GetTaskInterface()->AddTask([this, ref, attempts]() { TrackRefDeferred(ref, attempts + 1); });
    return;
  }

  std::lock_guard<std::mutex> lock(trackedRefsLock);
  trackedRefs.push_back({ref->GetHandle(), std::move(shapes)});
}

void FogManager::CleanupRefs() {
  std::lock_guard<std::mutex> lock(trackedRefsLock);
  trackedRefs.clear();
}

void FogManager::ProcessCell(RE::TESObjectCELL* cell) {
  CleanupRefs();
  cell->ForEachReference([&](RE::TESObjectREFR& ref) -> RE::BSContainer::ForEachResult {
    auto baseObject = ref.GetBaseObject();
    if (!baseObject) return RE::BSContainer::ForEachResult::kContinue;
    if (isFog(baseObject)) TrackRefDeferred(&ref, 0);
    return RE::BSContainer::ForEachResult::kContinue;
  });
}

RE::BSEventNotifyControl FogManager::ProcessEvent(const RE::BGSActorCellEvent* event,
  RE::BSTEventSource<RE::BGSActorCellEvent>*) {
  auto player = RE::PlayerCharacter::GetSingleton();
  if (event->actor != player->GetHandle()) return RE::BSEventNotifyControl::kContinue;

  if (event->flags == RE::BGSActorCellEvent::CellFlag::kLeave) {
    CleanupRefs();
  } else if (event->flags == RE::BGSActorCellEvent::CellFlag::kEnter) {
    auto cellForm = RE::TESForm::LookupByID(event->cellID);
    auto cell = cellForm->As<RE::TESObjectCELL>();
    if (cell) ProcessCell(cell);
  }

  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl FogManager::ProcessEvent(const RE::TESLoadGameEvent* event,
  RE::BSTEventSource<RE::TESLoadGameEvent>*) {
  auto player = RE::PlayerCharacter::GetSingleton();
  auto playerHolder = player->AsBGSActorCellEventSource();
  playerHolder->AddEventSink<RE::BGSActorCellEvent>(this);
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl FogManager::ProcessEvent(const RE::TESCellFullyLoadedEvent* event,
  RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) {
  RE::TESObjectCELL* cell = event->cell;
  if (!cell->IsInteriorCell()) return RE::BSEventNotifyControl::kContinue;
  if (!CellIsTracked(cell->formID)) ProcessCell(cell);

  return RE::BSEventNotifyControl::kContinue;
}

void FogManager::Init() {
  auto configManager = Config::GetSingleton();

  minAlpha = configManager->get<float>("minimumAlpha", 0.0f);
  fallbackAlpha = configManager->get<float>("fallbackAlpha", 0.36f);
  fadeDistance = configManager->get<float>("fadeDistance", 150.f);
  lerpFactor = configManager->get<float>("lerpFactor", 0.1f);
  log::info("Initialised Config");

  auto scriptEventHolder = RE::ScriptEventSourceHolder::GetSingleton();
  scriptEventHolder->AddEventSink<RE::TESCellFullyLoadedEvent>(this);
  scriptEventHolder->AddEventSink<RE::TESLoadGameEvent>(this);
  log::info("Installed Events");
}
} // namespace NullMod