#pragma once
#include <Config.h>
namespace NullMod {
struct ShaderData {
  int shaderIndex;
  float alpha;
};

using ShapeRef = std::vector<ShaderData>;
struct FogRef {
  RE::ObjectRefHandle handle;
  std::vector<ShapeRef> shapes;
};

class FogManager : public RE::BSTEventSink<RE::TESCellFullyLoadedEvent>,
                   public RE::BSTEventSink<RE::TESLoadGameEvent>,
                   public RE::BSTEventSink<RE::BGSActorCellEvent> {
  FogManager() = default;
  FogManager(const FogManager&) = delete;
  FogManager(FogManager&&) = delete;
  FogManager& operator=(const FogManager&) = delete;
  FogManager& operator=(const FogManager&&) = delete;

 public:
  std::vector<FogRef> trackedRefs;
  std::mutex trackedRefsLock;
  
  float minAlpha;
  float fallbackAlpha;
  float invisibleDistance;
  float visibleDistance;
  bool useTint;
  RE::NiColor tint;

  static FogManager* GetSingleton() {
    static FogManager events;
    return &events;
  }

  void Init();
  bool CellIsTracked(RE::FormID formID);
  void Serialize(json& j);
  void CleanupRefs();
  void ProcessCell(RE::TESObjectCELL* cell);

  RE::BSEventNotifyControl ProcessEvent(
      const RE::TESCellFullyLoadedEvent* event,
      RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override;

  RE::BSEventNotifyControl ProcessEvent(
      const RE::TESLoadGameEvent* event,
      RE::BSTEventSource<RE::TESLoadGameEvent>*) override;

  RE::BSEventNotifyControl ProcessEvent(
      const RE::BGSActorCellEvent* event,
      RE::BSTEventSource<RE::BGSActorCellEvent>*) override;

 private:
  // Main functions
  void TrackRef(RE::TESObjectREFR* ref);
  void TrackRefDeferred(RE::TESObjectREFR* ref, int attempts);
  std::vector<ShapeRef> GetShadersForRef(RE::TESObjectREFR* ref);
};
}  // namespace NullMod