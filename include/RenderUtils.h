#include <SKSEMenuFramework.h>
#include <FogManager.h>
namespace NullMod {
class RayUtils {
public:
  // https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection.html
  static bool mollerTrumbore(RE::NiPoint3& orig, RE::NiPoint3& dir, RE::NiPoint3& v0, RE::NiPoint3& v1,
    RE::NiPoint3& v2, float& t, float& u, float& v);
  static float calculateFadeFromRay(FogManager* fogManager, RE::BSTriShape* triShape, RE::NiPoint3 target,
    RE::NiPoint3 rayOrig, RE::NiPoint3 rayDir, float edgeDist, FogRay& rayOut);
  static bool castRay(RE::BSTriShape* shape, RE::NiPoint3 rayOrig, RE::NiPoint3 rayDir, float& outDistance);
};

class RenderUtils {
public:
  static bool computePixelCoordinates(RE::NiPoint3 pWorld, RE::PlayerCamera* camera, ImGuiMCP::ImGuiIO* io,
    ImGuiMCP::ImVec2& pRasterOut);
};
} // namespace NullMod