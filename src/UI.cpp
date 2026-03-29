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
      auto& config = configManager->GetConfig();
      FogManager::GetSingleton()->Serialize(config);
      if (!configManager->SaveConfig()) log::warn("Failed to save config. Changes will not be reflected.");
      break;
  }
}

void UI::Render_Settings() {
  ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
                     "Maximum alpha value used when it can not be determined:");
  ImGui::SliderFloat("Fallback Alpha",
                     &FogManager::GetSingleton()->fallbackAlpha, 0.0f, 1.0f);
  ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
                     "Distance at which fog will be fully transparent:");
  ImGui::SliderFloat("Invisible Distance",
                     &FogManager::GetSingleton()->invisibleDistance, 20, 200);
  ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
                     "Distance at which fog will be it's original opacity");
  ImGui::SliderFloat("Visible Distance",
                     &FogManager::GetSingleton()->visibleDistance, 100, 800);
}

void UI::Render_Debug() {
  ImGui::Text("Tracked Refs");
  for (auto fogRef : FogManager::GetSingleton()->trackedRefs) {
    auto handle = fogRef.handle;
    auto ref = handle.get();
    if (!ref) continue;
    ImGui::Text("RefID: %08X", ref->GetFormID());
    for (int i = 0; i < fogRef.shapes.size(); i++) {
      const auto& shape = fogRef.shapes[i];
      for (auto& shader : shape) {
        ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
                           "shape %d, shader %d :%.2f", i, shader.shaderIndex,
                           shader.alpha);
      }
    }
  }
}
}  // namespace NullMod
