#include <FogManager.h>
#include <Offsets.h>
#include <Shader.h>

using namespace SKSE;
namespace NullMod {
bool isFog(RE::TESBoundObject* baseObject) {
  static const std::regex regexpr(R"(FXMistLow\d+(HalfVis)?)");

  auto stat = baseObject->As<RE::TESObjectSTAT>();
  if (!stat) return false;

  const char* modelPath = stat->model.c_str();
  if (!modelPath) return false;

  return std::regex_search(modelPath, regexpr);
}

void FogManager::Serialize(json& j) {
  j = nlohmann::json{
      {"minimumAlpha", minAlpha},
      {"fallbackAlpha", fallbackAlpha}, 
      {"visibleDistance", visibleDistance}, 
      {"invisibleDistance", invisibleDistance}
  };
}

bool FogManager::CellIsTracked(RE::FormID cellID) {
  std::lock_guard<std::mutex> lock(trackedRefsLock);
  for (auto fogRef : trackedRefs) {
    auto ref = fogRef.handle.get();
    if (!ref) return false;
    if (ref->GetParentCell()->formID == cellID) return true;
  }

  return false;
}

std::vector<ShapeRef> FogManager::GetShadersForRef(RE::TESObjectREFR* ref) {
  std::vector<ShapeRef> results;
  auto container = ref ? ref->Get3D() : nullptr;
  auto rootNode = container ? container->AsNode() : nullptr;

  if (!rootNode) return results;
  for (auto child : rootNode->GetChildren()) {
    auto triShape = child ? child->AsTriShape() : nullptr;
    if (!triShape) continue;

    ShapeRef shapeRef;
    auto& runtimeData = triShape->GetGeometryRuntimeData();
    for (int i = 0; i < RE::BSGeometry::States::kTotal; i++) {
      auto prop = runtimeData.properties[i].get();
      auto effectShader = prop ? netimmerse_cast<RE::BSEffectShaderProperty*>(prop) : nullptr;
      if (effectShader) {
        auto matAlpha = effectShader->QMaterialAlpha();
        if (matAlpha >= 1.0) matAlpha = fallbackAlpha;
        matAlpha = std::max(matAlpha, minAlpha);
        log::info("Creating ShaderData: i={:d} matAlpha={:1.2f}, minAlpha={:1.2f}", i, matAlpha, minAlpha);
        shapeRef.push_back({i, matAlpha});
      }
    }

    results.push_back(std::move(shapeRef));
  }

  return results;
}

void FogManager::TrackRefDeferred(RE::TESObjectREFR* ref, int attempts) {
  if (attempts > 120) {
    log::warn("Failed to load 3D for {:08X}", ref->formID);
    return;
  }

  auto shaders = GetShadersForRef(ref);
  if (shaders.empty()) {
    SKSE::GetTaskInterface()->AddTask([this, ref, attempts]() { 
       TrackRefDeferred(ref, attempts + 1);
    });

    return;
  }

  auto baseObject = ref->GetBaseObject();
  auto stat = baseObject->As<RE::TESObjectSTAT>();
  const char* modelPath = stat->model.c_str();
  log::info("Tracking: FormID={}, RefID={}, Path={}", baseObject->formID, ref->formID, modelPath);

  std::lock_guard<std::mutex> lock(trackedRefsLock);
  FogRef fogRef = {ref->GetHandle(), shaders};
  trackedRefs.push_back(fogRef);
}

void FogManager::CleanupRefs() {
  std::lock_guard<std::mutex> lock(trackedRefsLock);
  for (const auto& [handle, shapes] : trackedRefs) {
    auto ref = handle.get();
    if (!ref) continue;

    auto baseObject = ref->GetBaseObject();
    if (!baseObject) continue;
    auto stat = baseObject->As<RE::TESObjectSTAT>();
    const char* modelPath = stat->model.c_str();
    log::info("Untracking: FormID={}, RefID={}, Path={}", baseObject->formID, ref->formID, modelPath);
  }

  trackedRefs.clear();
}

void FogManager::ProcessCell(RE::TESObjectCELL* cell) {
  auto player = RE::PlayerCharacter::GetSingleton();
  CleanupRefs();
  cell->ForEachReference([&](RE::TESObjectREFR& ref) -> RE::BSContainer::ForEachResult {
    auto baseObject = ref.GetBaseObject();
    if (!baseObject) return RE::BSContainer::ForEachResult::kContinue;

    if (isFog(baseObject)) {
      auto stat = baseObject->As<RE::TESObjectSTAT>();
      const char* modelPath = stat->model.c_str();
      TrackRefDeferred(&ref, 0);
    }

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

  minAlpha = configManager->get<float>("minimumAlpha", 0.0);
  fallbackAlpha = configManager->get<float>("fallbackAlpha", 0.36);
  invisibleDistance = configManager->get<float>("invisibleDistance", 200.f);
  visibleDistance = configManager->get<float>("visibleDistance", 400.f);

  useTint = configManager->get<bool>("useTint", false);
  tint.red = configManager->getFrom<float>("tint", "r", 1.0f);
  tint.green = configManager->getFrom<float>("tint", "g", 0.0f);
  tint.blue = configManager->getFrom<float>("tint", "b", 0.0f);

  log::info("Initialised Config");

  auto scriptEventHolder = RE::ScriptEventSourceHolder::GetSingleton();
  scriptEventHolder->AddEventSink<RE::TESCellFullyLoadedEvent>(this);
  scriptEventHolder->AddEventSink<RE::TESLoadGameEvent>(this);
  log::info("Installed Events");
}
}  // namespace NullMod