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

  ImGui::SliderFloat("Invisible Distance", &fogManager->invisibleDistance, 20,
                     200);
  ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
                     "Distance at which fog will be fully transparent.");

  ImGui::SliderFloat("Visible Distance", &fogManager->visibleDistance, 100,
                     800);
  ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
                     "Distance at which fog will be it's maximum opacity.");
  fogManager->minAlpha =
      std::min(fogManager->minAlpha, fogManager->fallbackAlpha);

  fogManager->invisibleDistance = std::min(fogManager->invisibleDistance,
                                           fogManager->visibleDistance - 1.0f);
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
