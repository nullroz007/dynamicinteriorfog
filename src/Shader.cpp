#include <Shader.h>
#include <RenderUtils.h>
#include <SKSEMenuFramework.h>

using namespace SKSE;
namespace NullMod {
bool Shader::applyFade(FogManager* fogManager, RE::BSEffectShaderProperty* effectShader, float fadePercent, float maxAlpha) {
  float minAlpha = std::min(fogManager->minAlpha, maxAlpha);
  float currentAlpha = effectShader->QMaterialAlpha();
  float targetAlpha = std::clamp(fadePercent * maxAlpha, minAlpha, maxAlpha);
  float smoothedAlpha = std::lerp(currentAlpha, targetAlpha, fogManager->lerpFactor * 0.1f);

  if (std::abs(currentAlpha - targetAlpha) > alphaEpsilon) {
    effectShader->flags.set(RE::BSEffectShaderProperty::EShaderPropertyFlag::kVertexAlpha);
    effectShader->SetMaterialAlpha(smoothedAlpha);
    return true;
  }

  return false;
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
    RE::BSVisit::TraverseScenegraphGeometries(container,
      [&](RE::BSGeometry* geom) -> RE::BSVisit::BSVisitControl {
        bool needsUpdate = false;

        auto triShape = geom->AsTriShape();
        auto& runtimeData = geom->GetGeometryRuntimeData();
        if (!triShape) return RE::BSVisit::BSVisitControl::kContinue;
        
        auto it = fogRef.shapes.find(triShape->name.c_str());
        if (it == fogRef.shapes.end()) return RE::BSVisit::BSVisitControl::kContinue;
        const ShapeRef& shapeRef = it->second;

        FogRay ray{};
        float fadePercent = RayUtils::calculateFadeFromRay(fogManager, &fogRef, triShape, fogCenter,
          posPlayer, dirRay, edgeDist, ray);

        if (ray.hit) foundHit = true;
        fogRef.rays.push_back(std::move(ray));

        for (const auto& shader : shapeRef) {
          auto prop = runtimeData.properties[shader.shaderIndex].get();
          auto effectShader = netimmerse_cast<RE::BSEffectShaderProperty*>(prop);
          if (!effectShader) continue;

          needsUpdate |= Shader::applyFade(fogManager, effectShader, fadePercent, shader.alpha);
        }

        if (needsUpdate) {
          RE::NiUpdateData ctx;
          triShape->SetMaterialNeedsUpdate(true);
          triShape->Update(ctx);
        }

        return RE::BSVisit::BSVisitControl::kContinue;
    });
  }

  fogRef.rayHit = foundHit;
}
} // namespace NullMod