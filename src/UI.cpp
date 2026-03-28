#include <UI.h>
#include <FogManager.h>
namespace ImGui = ImGuiMCP;
namespace NullMod {
void UI::Init() {
  SKSEMenuFramework::SetSection("Dynamic Interior Fog");
  SKSEMenuFramework::AddSectionItem("Settings", UI::Render_Settings);
  SKSEMenuFramework::AddSectionItem("Debug", UI::Render_Debug);
}

void UI::Render_Settings() {
  ImGui::SliderFloat("Invisible Distance",
                     &FogManager::GetSingleton()->invisibleDistance, 20, 200);
  ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
                     "Distance at which fog will be fully transparent");
  ImGui::SliderFloat("Visible Distance",
                     &FogManager::GetSingleton()->visibleDistance, 100, 800);
  ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
                     "Distance at which fog will be it's original opacity");
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
        ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f), "shape %d, shader %d :%.2f",
                           i, shader.shaderIndex, shader.alpha);
      }
    }   
  }
}
}  // namespace NullMod
