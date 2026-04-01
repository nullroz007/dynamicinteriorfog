#include <Shader.h>

using namespace SKSE;
namespace NullMod {
bool Shader::applyFade(RE::BSEffectShaderProperty* effectShader,
                       float fadePercent, float maxAlpha, float minAlpha) {
  minAlpha = std::min(minAlpha, maxAlpha);
  float currentAlpha = effectShader->QMaterialAlpha();
  float targetAlpha = std::clamp(fadePercent * maxAlpha, minAlpha, maxAlpha);

  if (std::abs(currentAlpha - targetAlpha) > 0.01f) {
    effectShader->flags.set(
        RE::BSEffectShaderProperty::EShaderPropertyFlag::kVertexAlpha);
    effectShader->SetMaterialAlpha(targetAlpha);
    return true;
  }

  return false;
}

bool Shader::applyEffect(
    RE::BSGeometry* geom, const ShapeRef& shapeRef,
    std::function<bool(RE::BSEffectShaderProperty*, const ShaderData&)>
        effectFunc) {
  auto& runtimeData = geom->GetGeometryRuntimeData();

  bool needsUpdate = false;
  for (const auto& shader : shapeRef) {
    auto prop = runtimeData.properties[shader.shaderIndex].get();
    if (const auto& effectShader = netimmerse_cast<RE::BSEffectShaderProperty*>(prop)) {
      needsUpdate |= effectFunc(effectShader, shader);
    }
  }

  return needsUpdate;
}

void Shader::applyEffects(FogManager* fogManager, RE::PlayerCharacter* player,
                          const FogRef& fogRef) {
  auto ref = fogRef.handle.get();
  if (!ref || !ref->Is3DLoaded()) return;

  auto container = ref->Get3D();
  auto posFog = ref->GetPosition();
  auto posPlayer = player->GetPosition();
  auto dist = posPlayer.GetDistance(posFog);
  auto time = RE::Calendar::GetSingleton()->GetCurrentGameTime();

  float fadePercent = std::clamp(
      (dist - fogManager->invisibleDistance) /
          (fogManager->visibleDistance - fogManager->invisibleDistance),
      0.0f, 1.0f);

  RE::NiColor tint = fogManager->tint;
  if (auto nodes = container->AsNode()) {
    int shapeIndex = 0;
    for (auto child : nodes->GetChildren()) {
      if (!child) continue;
      if (shapeIndex >= fogRef.shapes.size()) break;
      auto triShape = child->AsTriShape();

      if (triShape) {
        if (Shader::applyEffect(
                triShape, fogRef.shapes[shapeIndex],
                [&](auto effectShader, const ShaderData& shader) {
          bool effectsApplied = false;
          effectsApplied |= Shader::applyFade(effectShader, fadePercent, shader.alpha,
                                   fogManager->minAlpha);
          return effectsApplied;
        })) {
          RE::NiUpdateData ctx;
          triShape->SetMaterialNeedsUpdate(true);
          triShape->Update(ctx);
        }

        shapeIndex++;
      }
    }
  }
}
}  // namespace NullMod