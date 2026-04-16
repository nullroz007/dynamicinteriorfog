#pragma once
#include <FogManager.h>
namespace NullMod {
class Shader {
 public:
  static bool applyEffect(
      RE::BSGeometry* geom, const ShapeRef& shapeRef,
      std::function<bool(RE::BSEffectShaderProperty*, const ShaderData&)>
          effectFunc);

  static bool applyFade(RE::BSEffectShaderProperty* effectShader,
                        float fadePercent, float maxAlpha, float minAlpha,
                        float lerpFactor);
  static bool castRay(RE::BSTriShape* shape, RE::NiPoint3 rayOrig,
                      RE::NiPoint3 rayDir, float& outDistance);
  static void applyEffects(FogManager* fogManager, RE::PlayerCharacter* player,
                           FogRef& fogRef);
};

}  // namespace NullMod