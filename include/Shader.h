#pragma once
#include <FogManager.h>
namespace NullMod {
class Shader {
public:
  static constexpr float rayEpsilon = 1e-7f;
  static constexpr float alphaEpsilon = 0.01;

  static bool applyFade(FogManager* fogManager, RE::BSEffectShaderProperty* effectShader, float fadePercent, float maxAlpha);
  static bool castRay(RE::BSTriShape* shape, RE::NiPoint3 rayOrig, RE::NiPoint3 rayDir, float& outDistance);
  static void applyEffects(FogManager* fogManager, RE::PlayerCharacter* player, FogRef& fogRef);
};

} // namespace NullMod