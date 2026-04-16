#include <UI.h>
#include <FogManager.h>
namespace ImGui = ImGuiMCP;
namespace NullMod {
using namespace SKSE;
using EventType = SKSEMenuFramework::Model::EventType;
void UI::Init() {
  SKSEMenuFramework::SetSection("Dynamic Interior Fog");
  SKSEMenuFramework::AddSectionItem("Settings", UI::Render_Settings);
  SKSEMenuFramework::AddSectionItem("Debug", UI::Render_Debug);
  eventHandler = SKSEMenuFramework::AddEvent(OnEvent, 1.0f);
}

SKSEMenuFramework::Model::Event* UI::eventHandler = nullptr;
void UI::OnEvent(EventType eventType) {
  switch (eventType) {
    case EventType::kCloseMenu:
      auto configManager = Config::GetSingleton();
      auto fogManager = FogManager::GetSingleton();

      auto& config = configManager->GetConfig();
      fogManager->Serialize(config);

      if (!configManager->SaveConfig())
        log::warn("Failed to save config. Changes will not be reflected.");
      break;
  }
}

void UI::Render_Settings() {
  auto fogManager = FogManager::GetSingleton();

  ImGui::SliderFloat("Minimum Alpha", &fogManager->minAlpha, 0.0f, 1.0f);
  ImGui::TextColored(ImGui::ImVec4(1, 1, 0, 0.5f),
                     "Must be smaller than the fallback alpha value.");

  ImGui::SliderFloat("Fallback Alpha", &fogManager->fallbackAlpha, 0.0f, 1.0f);
  ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
                     "The maximum alpha value used when it can not be "
                     "automatically determined.");

  ImGui::SliderFloat("Fade Distance", &fogManager->fadeDistance, 0.0f, 100.0f);
  ImGui::TextColored(
      ImGui::ImVec4(1, 1, 1, 0.5f),
      "Distance from the mesh edge at which fog begins fading out.");

  ImGui::SliderFloat("Fade Responsiveness", &fogManager->lerpFactor, 0.1f, 1);
  ImGui::TextColored(
      ImGui::ImVec4(1, 1, 1, 0.5f),
      "Linear interpolation factor applied to the fade. 0.1 = slow fade, 1.0 = fast");
}

void UI::Render_Debug() {
  auto fogManager = FogManager::GetSingleton();
  std::lock_guard<std::mutex> lock(fogManager->trackedRefsLock);
  ImGui::Text("Tracked Refs");
  for (auto fogRef : fogManager->trackedRefs) {
    auto handle = fogRef.handle;
    auto ref = handle.get();
    if (!ref) continue;
    ImGui::Text("RefID: %08X", ref->GetFormID());
    ImGui::Text("Ray: %s", fogRef.rayHit ? "Hit" : "Miss");
    for (int i = 0; i < fogRef.shapes.size(); i++) {
      const auto& shape = fogRef.shapes[i];
      for (auto& shader : shape) {
        ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
                           "shape %d, shader %d original alpha:%.2f", i,
                           shader.shaderIndex, shader.alpha);
      }
    }
  }
}
}  // namespace NullMod
