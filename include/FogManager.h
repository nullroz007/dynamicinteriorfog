#include <Config.h>
namespace NullMod {
struct ShaderAlpha {
  int shaderIndex;
  float alpha;
};

using ShapeRef = std::vector<ShaderAlpha>;
struct FogReference {
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
  std::vector<FogReference> trackedRefs;
  std::mutex trackedRefLock;

  float fallbackAlpha = 0.56f;
  float invisibleDistance = 200.f;
  float visibleDistance = 400.f;

  static FogManager* GetSingleton() {
    static FogManager events;
    return &events;
  }

  void Init();
  bool CellIsTracked(RE::FormID formID);
  void Serialize(json& j);
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
  void CleanupRefs();
  void ProcessCell(RE::TESObjectCELL* cell);
  std::vector<ShapeRef> GetShadersForRef(RE::TESObjectREFR* ref);
  void SetGeomFlags(RE::TESObjectREFR* ref);
};
}  // namespace NullMod