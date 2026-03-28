#include <Shader.h>

using namespace SKSE;
namespace NullMod {
bool Shader::applyFade(RE::BSEffectShaderProperty* effectShader,
                       float fadePercent, float maxAlpha, float minAlpha) {
  minAlpha = std::min(minAlpha, maxAlpha);
  float targetAlpha = std::clamp(fadePercent * maxAlpha, minAlpha, maxAlpha);
  if (std::abs(effectShader->QMaterialAlpha() - targetAlpha) > 0.01f) {
    effectShader->flags.set(
        RE::BSEffectShaderProperty::EShaderPropertyFlag::kVertexAlpha);
    effectShader->SetMaterialAlpha(targetAlpha);
    return true;
  }

  return false; 
}

bool Shader::applyTint(RE::BSEffectShaderProperty* effectShader,
                       RE::NiColor tint) {
  if (!effectShader->unk88) return false;
  RE::NiColor* currentColor = effectShader->unk88;
  if (std::abs(currentColor->red - tint.red) < 0.01f &&
      std::abs(currentColor->blue - tint.blue) < 0.01f &&
      std::abs(currentColor->green - tint.green) < 0.01f)
    return false;

  currentColor->red = tint.red;
  currentColor->green = tint.green;
  currentColor->blue = tint.blue;

  return true;
}

bool Shader::applyEffect(
    RE::BSGeometry* geom, const ShapeRef& shapeRef,
    std::function<bool(RE::BSEffectShaderProperty*, const ShaderData&)>
        effectFunc) {
  auto& runtimeData = geom->GetGeometryRuntimeData();

  bool needsUpdate = false;
  for (auto& shader : shapeRef) {
    auto prop = runtimeData.properties[shader.shaderIndex].get();
    if (auto effectShader = netimmerse_cast<RE::BSEffectShaderProperty*>(prop))
      needsUpdate |= effectFunc(effectShader, shader);
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
        bool needsUpdate = false;
        needsUpdate |= Shader::applyEffect(
            triShape, fogRef.shapes[shapeIndex],
            [&](auto effectShader, const ShaderData& shader) {
          bool effectApplied = false;
          effectApplied |= Shader::applyFade(
              effectShader, fadePercent, shader.alpha, fogManager->minAlpha);
          /* Ignoring for now, causing some flickering
          effectApplied |= Shader::applyTint(effectShader, fogManager->tint);*/
          return effectApplied;
        });

        shapeIndex++;

        if (needsUpdate) {
          RE::NiUpdateData ctx;
          triShape->SetMaterialNeedsUpdate(true);
          triShape->Update(ctx);
        }
      }
    }
  }
}
}  // namespace NullMod