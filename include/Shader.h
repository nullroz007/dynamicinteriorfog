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
                        float fadePercent, float maxAlpha, float minAlpha);
  static void applyEffects(FogManager* fogManager, RE::PlayerCharacter* player,
                           const FogRef& fogRef);
};

}  // namespace NullMod