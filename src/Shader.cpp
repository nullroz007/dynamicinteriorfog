#include <Shader.h>
#include <RenderUtils.h>
#include <SKSEMenuFramework.h>

using namespace SKSE;
namespace NullMod {
bool Shader::applyFade(RE::BSEffectShaderProperty* effectShader, float fadePercent, float maxAlpha,
  float minAlpha, float lerpFactor) {
  minAlpha = std::min(minAlpha, maxAlpha);
  float currentAlpha = effectShader->QMaterialAlpha();
  float targetAlpha = std::clamp(fadePercent * maxAlpha, minAlpha, maxAlpha);
  float smoothedAlpha = std::lerp(currentAlpha, targetAlpha, lerpFactor * 0.1f);

  if (std::abs(currentAlpha - targetAlpha) > alphaEpsilon) {
    effectShader->flags.set(RE::BSEffectShaderProperty::EShaderPropertyFlag::kVertexAlpha);
    effectShader->SetMaterialAlpha(smoothedAlpha);
    return true;
  }

  return false;
}

bool Shader::applyEffect(RE::BSGeometry* geom, const ShapeRef& shapeRef,
  std::function<bool(RE::BSEffectShaderProperty*, const ShaderData&)> effectFunc) {
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

void Shader::applyEffects(FogManager* fogManager, RE::PlayerCharacter* player, FogRef& fogRef) {
  auto ref = fogRef.handle.get();
  if (!ref || !ref->Is3DLoaded()) return;

  fogRef.rays.clear();

  auto container = ref->Get3D();
  auto posFog = ref->GetPosition();
  auto posPlayer = player->GetPosition();

  auto worldBound = container->worldBound;
  auto fogCenter = worldBound.center;
  auto distToCenter = posPlayer.GetDistance(fogCenter);
  auto edgeDist = distToCenter - worldBound.radius;

  auto dirToCenter = fogCenter - posPlayer;
  dirToCenter.Unitize();

  auto closestPoint = fogCenter - (dirToCenter * worldBound.radius);
  auto dirRay = closestPoint - posPlayer;
  dirRay.Unitize();

  bool foundHit = false;
  if (auto nodes = container->AsNode()) {
    int shapeIndex = 0;
    for (auto child : nodes->GetChildren()) {
      if (!child) continue;
      if (shapeIndex >= fogRef.shapes.size()) break;
      
      if (auto triShape = child->AsTriShape()) {
        FogRay ray{};
        float fadePercent = RayUtils::calculateFadeFromRay(fogManager, triShape, fogCenter, posPlayer, dirRay,
          edgeDist, ray);
        
        if (ray.hit) foundHit = true;
        fogRef.rays.push_back(std::move(ray));

        if (Shader::applyEffect(triShape, fogRef.shapes[shapeIndex],
              [&](RE::BSEffectShaderProperty* effectShader, const ShaderData& shader) {
                return Shader::applyFade(effectShader, fadePercent, shader.alpha, fogManager->minAlpha,
                  fogManager->lerpFactor);
              })) {
          RE::NiUpdateData ctx;
          triShape->SetMaterialNeedsUpdate(true);
          triShape->Update(ctx);
        }

        shapeIndex++;
      }
    }
  }

  fogRef.rayHit = foundHit;
}
} // namespace NullMod