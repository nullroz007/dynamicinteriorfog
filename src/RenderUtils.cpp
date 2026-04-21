#include <RenderUtils.h>
#include <Shader.h>

#define _USE_MATH_DEFINES
#include <math.h>

using namespace NullMod;
using namespace SKSE;
namespace ImGui = ImGuiMCP;
bool RenderUtils::computePixelCoordinates(RE::NiPoint3 pWorld, RE::PlayerCamera* camera,
  ImGui::ImGuiIO* io, ImGui::ImVec2& pRasterOut) {
  auto cameraRoot = camera->cameraRoot.get();
  float aspect = io->DisplaySize.x / io->DisplaySize.y;
  float vFov = camera->worldFOV;
  float radAngle = vFov * M_PI / 180.f;

  float canvasWidth = 2.0f * std::tan(radAngle / 2.0f);
  float canvasHeight = canvasWidth / aspect;

  RE::NiTransform worldToCamera = cameraRoot->world.Invert();
  RE::NiPoint3 pCamera = worldToCamera * pWorld;

  ImGui::ImVec2 pScreen{};
  if (pCamera.y <= 0) return false;

  float perspective = 1.0f / pCamera.y;
  pScreen.x = pCamera.x * perspective;
  pScreen.y = pCamera.z * perspective;

  float halfWidth = canvasWidth * 0.5f;
  float halfHeight = canvasHeight * 0.5f;
  if (std::abs(pScreen.x) > halfWidth || std::abs(pScreen.y) > halfHeight) return false;

  ImGui::ImVec2 pNDC{};
  pNDC.x = (pScreen.x + canvasWidth / 2) / canvasWidth;
  pNDC.y = (pScreen.y + canvasHeight / 2) / canvasHeight;

  pRasterOut.x = std::floor(pNDC.x * io->DisplaySize.x);
  pRasterOut.y = std::floor((1 - pNDC.y) * io->DisplaySize.y);
  return true;
}

bool RayUtils::mollerTrumbore(RE::NiPoint3& orig, RE::NiPoint3& dir, RE::NiPoint3& v0, RE::NiPoint3& v1,
  RE::NiPoint3& v2, float& t, float& u, float& v) {
  RE::NiPoint3 v0v1 = v1 - v0;
  RE::NiPoint3 v0v2 = v2 - v0;
  RE::NiPoint3 pvec = dir.Cross(v0v2);
  float det = v0v1.Dot(pvec);

  if (std::abs(det) < Shader::rayEpsilon) return false;
  float invDet = 1 / det;

  RE::NiPoint3 tvec = orig - v0;
  u = tvec.Dot(pvec) * invDet;
  if (u < 0 || u > 1) return false;

  RE::NiPoint3 qvec = tvec.Cross(v0v1);
  v = dir.Dot(qvec) * invDet;
  if (v < 0 || u + v > 1) return false;

  t = v0v2.Dot(qvec) * invDet;
  return (t > Shader::rayEpsilon);
}

bool RayUtils::castRay(RE::BSTriShape* shape, RE::NiPoint3 rayOrig, RE::NiPoint3 rayDir,
  float& outDistance) {
  auto& runtimeData = shape->GetGeometryRuntimeData();
  auto& triRuntimeData = shape->GetTrishapeRuntimeData();
  auto& rendererData = runtimeData.rendererData;

  if (!rendererData || !rendererData->rawIndexData || !rendererData->rawVertexData) {
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

    auto v0 = *reinterpret_cast<RE::NiPoint3*>(rawVertexData + (idx0 * vertSize));
    auto v1 = *reinterpret_cast<RE::NiPoint3*>(rawVertexData + (idx1 * vertSize));
    auto v2 = *reinterpret_cast<RE::NiPoint3*>(rawVertexData + (idx2 * vertSize));

    float t, u, v;
    if (mollerTrumbore(locOrig, locDir, v0, v1, v2, t, u, v)) {
      if (t < minT) {
        minT = t;
        hit = true;
      }
    }
  }

  if (hit) {
    outDistance = minT;
    return true;
  }

  return false;
}

float RayUtils::calculateFadeFromRay(FogManager* fogManager, FogRef* fogRef, RE::BSTriShape* triShape, RE::NiPoint3 target,
  RE::NiPoint3 rayOrig, RE::NiPoint3 rayDir, float edgeDist, FogRay& rayOut) {
  float fadePercent = 1.0f;
  float rayDist = 0.0f;
  if (edgeDist <= 0.0f) return 0.0f;

  rayOut.origin = rayOrig;
  
  bool hit = castRay(triShape, rayOrig, rayDir, rayDist);
  float diff = std::abs(rayDist - edgeDist);
  float newDiff = std::abs(edgeDist - rayDist);

  if (newDiff > fogRef->edgeRayDiff) fogRef->edgeRayDiff = newDiff;

  float finalDist = hit ? rayDist : edgeDist - fogRef->edgeRayDiff;
  if (finalDist < fogManager->fadeDistance) {
    fadePercent = finalDist / fogManager->fadeDistance;
  } else {
    fadePercent = 1.0f;
  }

  rayOut.end = rayOrig + rayDir * (hit ? rayDist : edgeDist);
  rayOut.hit = hit;

  return fadePercent;
}