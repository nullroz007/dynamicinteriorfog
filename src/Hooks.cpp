#include <Hooks.h>
#include <FogManager.h>
using namespace SKSE;

namespace NullMod {
void Hooks::Install() {
  auto& trampoline = SKSE::GetTrampoline();
  trampoline.create(14);
  REL::Relocation<uintptr_t> OnUpdate_Target{RELOCATION_ID(35565, 36564)};
  _OnUpdate = trampoline.write_call<5>(
      OnUpdate_Target.address() + REL::Relocate(0x748, 0xC26), Hooks::OnUpdate);
  log::info("Installed Hooks");
}

void Hooks::OnUpdate() {
  _OnUpdate();

  FogManager* fogManager = FogManager::GetSingleton();
  std::lock_guard<std::mutex> lock(fogManager->trackedRefLock);
  if (fogManager->trackedRefs.empty()) return;

  auto player = RE::PlayerCharacter::GetSingleton();
  if (!player) return;

  for (auto& fogRef : fogManager->trackedRefs) {
    auto ref = fogRef.handle.get();
    if (!ref || !ref->Is3DLoaded()) continue;

    auto container = ref->Get3D();
    auto posFog = ref->GetPosition();
    auto posPlayer = player->GetPosition();
    auto dist = posPlayer.GetDistance(posFog);

    float fadePercent = std::clamp(
        (dist - fogManager->invisibleDistance) /
            (fogManager->visibleDistance - fogManager->invisibleDistance),
        0.0f, 1.0f);

    auto applyFade = [&](RE::BSGeometry* geom, ShapeRef& shapeRef) {
      auto& runtimeData = geom->GetGeometryRuntimeData();
      bool needsUpdate = false;
      for (auto& shader : shapeRef) {
        auto prop = runtimeData.properties[shader.shaderIndex].get();
        if (auto effectShader =
                netimmerse_cast<RE::BSEffectShaderProperty*>(prop)) {
          float maxAlpha = shader.alpha;
          float targetAlpha = fadePercent * maxAlpha;
          if (std::abs(effectShader->QMaterialAlpha() - targetAlpha) > 0.01f) {
            effectShader->flags.set(
                RE::BSEffectShaderProperty::EShaderPropertyFlag::kVertexAlpha);

            effectShader->SetMaterialAlpha(targetAlpha);
            effectShader->alpha = targetAlpha;
            needsUpdate = true;
          }
        }
      }

      if (needsUpdate) {
          RE::NiUpdateData ctx;
          geom->SetMaterialNeedsUpdate(true);
          geom->Update(ctx);
      }
    };

    if (auto nodes = container->AsNode()) {
      int shapeIndex = 0;
      for (auto child : nodes->GetChildren()) {
        if (!child) continue;
        if (shapeIndex >= fogRef.shapes.size()) break;
        auto triShape = child->AsTriShape();

        if (triShape) {
          applyFade(triShape, fogRef.shapes[shapeIndex]);
          shapeIndex++;
        }
      }
    }
  }
}

}  // namespace NullMod