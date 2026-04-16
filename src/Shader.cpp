#include <Shader.h>

using namespace SKSE;
namespace NullMod {
// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection.html
bool mollerTrumbore(RE::NiPoint3& orig, RE::NiPoint3& dir, RE::NiPoint3& v0,
                    RE::NiPoint3& v1, RE::NiPoint3& v2, float& t, float& u,
                    float& v) {
  const float kEpsilon = 1e-7f;
  RE::NiPoint3 v0v1 = v1 - v0;
  RE::NiPoint3 v0v2 = v2 - v0;
  RE::NiPoint3 pvec = dir.Cross(v0v2);
  float det = v0v1.Dot(pvec);

  if (std::abs(det) < kEpsilon) return false;
  float invDet = 1 / det;

  RE::NiPoint3 tvec = orig - v0;
  u = tvec.Dot(pvec) * invDet;
  if (u < 0 || u > 1) return false;

  RE::NiPoint3 qvec = tvec.Cross(v0v1);
  v = dir.Dot(qvec) * invDet;
  if (v < 0 || u + v > 1) return false;

  t = v0v2.Dot(qvec) * invDet;
  return (t > kEpsilon);
}

bool Shader::applyFade(RE::BSEffectShaderProperty* effectShader,
                       float fadePercent, float maxAlpha, float minAlpha,
                       float lerpFactor) {
  minAlpha = std::min(minAlpha, maxAlpha);
  float currentAlpha = effectShader->QMaterialAlpha();
  float targetAlpha = std::clamp(fadePercent * maxAlpha, minAlpha, maxAlpha);
  float smoothedAlpha = std::lerp(currentAlpha, targetAlpha, lerpFactor * 0.1);
  if (std::abs(currentAlpha - smoothedAlpha) > 0.001f) {
    effectShader->flags.set(
        RE::BSEffectShaderProperty::EShaderPropertyFlag::kVertexAlpha);
    effectShader->SetMaterialAlpha(smoothedAlpha);
    return true;
  }

  return false;
}

bool Shader::castRay(RE::BSTriShape* shape, RE::NiPoint3 rayOrig,
                     RE::NiPoint3 rayDir, float& outDistance) {
  if (!shape) return false;
  auto& runtimeData = shape->GetGeometryRuntimeData();
  auto& triRuntimeData = shape->GetTrishapeRuntimeData();
  auto& rendererData = runtimeData.rendererData;

  if (!rendererData || !rendererData->rawIndexData ||
      !rendererData->rawVertexData) {
    log::error("Invalid buffers, or Render data was nullptr");
    return false;
  }

  uint8_t* rawVertexData = rendererData->rawVertexData;
  uint16_t* rawIndexData = rendererData->rawIndexData;
  uint16_t triCount = triRuntimeData.triangleCount;
  uint32_t vertSize = rendererData->vertexDesc.GetSize();

  auto invTransform = shape->world.Invert();
  auto locOrig = invTransform.rotate * rayOrig + invTransform.translate;
  auto locDir = invTransform.rotate * rayDir;

  float minT = FLT_MAX;
  bool hit = false;
  for (int i = 0; i < triCount; i++) {
    int idx0 = rawIndexData[i * 3];
    int idx1 = rawIndexData[i * 3 + 1];
    int idx2 = rawIndexData[i * 3 + 2];

    auto v0 =
        *reinterpret_cast<RE::NiPoint3*>(rawVertexData + (idx0 * vertSize));
    auto v1 =
        *reinterpret_cast<RE::NiPoint3*>(rawVertexData + (idx1 * vertSize));
    auto v2 =
        *reinterpret_cast<RE::NiPoint3*>(rawVertexData + (idx2 * vertSize));

    float t, u, v;
    if (mollerTrumbore(locOrig, locDir, v0, v1, v2, t, u, v)) {
      float maxDistance = FogManager::GetSingleton()->fadeDistance;
      if (t < minT && t <= maxDistance) {
        minT = t;
        hit = true;
      }
    }
  }

  if (hit) {
    log::info("HITHITHIT");
    outDistance = minT;
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
    if (const auto& effectShader =
            netimmerse_cast<RE::BSEffectShaderProperty*>(prop)) {
      needsUpdate |= effectFunc(effectShader, shader);
    }
  }

  return needsUpdate;
}

void Shader::applyEffects(FogManager* fogManager, RE::PlayerCharacter* player,
                          FogRef& fogRef) {
  auto ref = fogRef.handle.get();
  if (!ref || !ref->Is3DLoaded()) return;

  auto container = ref->Get3D();
  auto posFog = ref->GetPosition();
  auto posPlayer = player->GetPosition();

  auto worldBound = container->worldBound;
  auto fogCenter = worldBound.center;
  auto distToCenter = posPlayer.GetDistance(fogCenter);
  auto edgeDist = distToCenter - worldBound.radius;
  auto direction = fogCenter - posPlayer;
  direction.Unitize();

  if (auto nodes = container->AsNode()) {
    int shapeIndex = 0;
    for (auto child : nodes->GetChildren()) {
      if (!child) continue;
      if (shapeIndex >= fogRef.shapes.size()) break;
      if (auto triShape = child->AsTriShape()) {
        float rayDist = 0.0f;
        bool hit = Shader::castRay(triShape, posPlayer, direction, rayDist);

        float finalDist = 0.0f;
        float fadePercent = 1.0f;

        if (hit)
          fadePercent =
              std::clamp(finalDist / fogManager->fadeDistance, 0.0f, 1.0f);
        else {
          if (edgeDist <= 0.0f)
            fadePercent = 0.0f;
          else
            fadePercent =
                std::clamp(edgeDist / fogManager->fadeDistance, 0.0f, 1.0f);
        }

        if (Shader::applyEffect(triShape, fogRef.shapes[shapeIndex],
                                [&](RE::BSEffectShaderProperty* effectShader,
                                    const ShaderData& shader) {
          fogRef.rayHit = hit;

          return Shader::applyFade(effectShader, fadePercent, shader.alpha,
                                   fogManager->minAlpha,
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
}
}  // namespace NullMod