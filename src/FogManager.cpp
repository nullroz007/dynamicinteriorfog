#include <FogManager.h>
#include <Offsets.h>
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

template <class T>
T* LookupForm(RE::FormID formID) {
  auto dataHandler = RE::TESDataHandler::GetSingleton();
  if (!dataHandler) return nullptr;

  uint32_t modIndex = (formID >> 24) & 0xFF;
  const RE::TESFile* modFile = nullptr;
  if (modIndex == 0xFE) {
    uint32_t lightIndex = (formID >> 12) & 0xFFF;
    modFile = dataHandler->LookupLoadedLightModByIndex(lightIndex);
  } else {
    modFile = dataHandler->LookupLoadedModByIndex(modIndex);
  }

  if (modFile) {
    std::string modName = modFile->fileName;
    auto localID = formID & 0x00FFFFFF;
    auto form = dataHandler->LookupForm<T>(localID, modName);
    return form;
  }

  return nullptr;
}

void FogManager::Serialize(json& j) {
  j = nlohmann::json{
      {"fallbackAlpha", fallbackAlpha}, {"visibleDistance", visibleDistance}, {"invisibleDistance", invisibleDistance}};
}

bool FogManager::CellIsTracked(RE::FormID cellID) {
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

  if (!rootNode) {
    ShapeRef fallback;
    fallback.push_back({1, fallbackAlpha});
    results.push_back(fallback);
    return results;
  }

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
        shapeRef.push_back({i, matAlpha});
      }
    }

    results.push_back(shapeRef);
  }

  return results;
}

void FogManager::TrackRef(RE::TESObjectREFR* ref) {
  auto alphaList = GetShadersForRef(ref);
  trackedRefs.push_back({ref->GetHandle(), alphaList});
}

void FogManager::CleanupRefs() {
  std::lock_guard<std::mutex> lock(trackedRefLock);
  for (auto [handle, alphaMap] : trackedRefs) {
    alphaMap.clear();

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
  cell->ForEachReference([&](RE::TESObjectREFR& ref) -> RE::BSContainer::ForEachResult {
    auto baseObject = ref.GetBaseObject();
    if (!baseObject) return RE::BSContainer::ForEachResult::kContinue;

    if (isFog(baseObject)) {
      auto stat = baseObject->As<RE::TESObjectSTAT>();
      const char* modelPath = stat->model.c_str();

      TrackRef(&ref);
      log::info("Tracking: FormID={}, RefID={}, Path={}", baseObject->formID, ref.formID, modelPath);
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
    auto cell = LookupForm<RE::TESObjectCELL>(event->cellID);
    if (cell && !CellIsTracked(cell->formID)) ProcessCell(cell);
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
  fallbackAlpha = configManager->get<float>("fallbackAlpha", 0.36);
  invisibleDistance = configManager->get<float>("invisibleDistance", 200.f);
  visibleDistance = configManager->get<float>("visibleDistance", 400.f);
  log::info("Initialised Config");

  auto scriptEventHolder = RE::ScriptEventSourceHolder::GetSingleton();
  scriptEventHolder->AddEventSink<RE::TESCellFullyLoadedEvent>(this);
  scriptEventHolder->AddEventSink<RE::TESLoadGameEvent>(this);
  log::info("Installed Events");
}
}  // namespace NullMod