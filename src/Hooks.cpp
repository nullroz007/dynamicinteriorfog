#include <Hooks.h>
#include <FogManager.h>
#include <Shader.h>
using namespace SKSE;

namespace NullMod {
void Hooks::Install() {
  auto& trampoline = SKSE::GetTrampoline();
  trampoline.create(14);
  REL::Relocation<uintptr_t> OnUpdate_Target{RELOCATION_ID(35565, 36564)};
  _OnUpdate = trampoline.write_call<5>(
      OnUpdate_Target.address() + REL::Relocate(0x748, 0xC26), Hooks::OnUpdate);
  log::info("Installed Hooks");
}

void Hooks::OnUpdate() {
  _OnUpdate();

  FogManager* fogManager = FogManager::GetSingleton();
  std::lock_guard<std::mutex> lock(fogManager->trackedRefsLock);
  auto player = RE::PlayerCharacter::GetSingleton();
  if (fogManager->trackedRefs.empty() || !player) return;

  for (auto& fogRef : fogManager->trackedRefs) {
    auto ref = fogRef.handle.get();
    if (!ref || !ref->Is3DLoaded()) continue;
    Shader::applyEffects(fogManager, player, fogRef);
  }
}
}  // namespace NullMod