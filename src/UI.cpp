#include <UI.h>
#include <FogManager.h>
#include <RenderUtils.h>

namespace ImGui = ImGuiMCP;
namespace NullMod {
using namespace SKSE;
using EventType = SKSEMenuFramework::Model::EventType;

void UI::Init() {
  auto fogManager = FogManager::GetSingleton();

  SKSEMenuFramework::SetSection("Dynamic Interior Fog");
  SKSEMenuFramework::AddSectionItem("Settings", UI::Render_Settings);
  SKSEMenuFramework::AddSectionItem("Debug", UI::Render_Debug);
  SKSEMenuFramework::AddHudElement(UI::Render_DebugRays);
  eventHandler = SKSEMenuFramework::AddEvent(OnEvent, 1.0f);

  fogManager->debugRays = false;
}

SKSEMenuFramework::Model::Event* UI::eventHandler = nullptr;
void UI::OnEvent(EventType eventType) {
  switch (eventType) {
  case EventType::kCloseMenu:
    auto configManager = Config::GetSingleton();
    auto fogManager = FogManager::GetSingleton();

    auto& config = configManager->GetConfig();
    fogManager->Serialize(config);

    if (!configManager->SaveConfig()) log::warn("Failed to save config. Changes will not be reflected.");
    break;
  }
}

void UI::Render_DebugRays() {
  auto _this = UI::GetSingleton();
  auto fogManager = FogManager::GetSingleton();
  if (SKSEMenuFramework::IsAnyBlockingWindowOpened() || !fogManager->debugRays) { return; }

  auto player = RE::PlayerCharacter::GetSingleton();
  if (!player) return;

  auto camera = RE::PlayerCamera::GetSingleton();
  if (!camera) return;

  auto cameraRoot = camera->cameraRoot.get();
  if (!cameraRoot) return;

  ImGui::ImGuiIO* io = ImGui::GetIO();
  ImGui::ImDrawList* drawList = ImGui::GetForegroundDrawList();

  std::lock_guard<std::mutex> lock(fogManager->trackedRefsLock);
  for (const auto& fogRef : fogManager->trackedRefs) {
    for (const auto& ray : fogRef.rays) {
      ImGui::ImU32 color = ray.hit ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255);
      ImGui::ImVec2 pOrigin{};
      ImGui::ImVec2 pEnd{};

      bool originValid = false;
      bool endValid = false;
      originValid = RenderUtils::computePixelCoordinates(ray.origin, camera, io, pOrigin);
      endValid = RenderUtils::computePixelCoordinates(ray.end, camera, io, pEnd);
      if (originValid || endValid) {
        if (!originValid) pOrigin = pEnd;
        if (!endValid) pEnd = pOrigin;
        ImGui::ImDrawListManager::AddLine(drawList, pOrigin, pEnd, color, 2);
      }
    }
  }
}

void UI::Render_Settings() {
  auto fogManager = FogManager::GetSingleton();
  ImGui::Text("Minimum Alpha");
  ImGui::TextColored(ImGui::ImVec4(1, 1, 0, 0.5f), "Must not be larger than the fallback alpha value.");
  ImGui::SliderFloat("##MinimumAlpha", &fogManager->minAlpha, 0.0f, 1.0f);

  ImGui::Text("Fallback Alpha");
  ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
    "The maximum alpha value used when it can not be automatically determined.");
  ImGui::SliderFloat("##FallbackAlpha", &fogManager->fallbackAlpha, 0.0f, 1.0f);

  ImGui::Text("Fade Distance");
  ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
    "Distance from the mesh edge at which fog begins fading out.");
  ImGui::SliderFloat("##FadeDistance", &fogManager->fadeDistance, 0.0f, 100.0f);

  ImGui::Text("Fade Responsiveness");
  ImGui::TextColored(ImGui::ImVec4(1, 1, 1, 0.5f),
    "Linear interpolation factor applied to the fade. 0.1 = slow fade, 1.0 = fast");
  ImGui::SliderFloat("##FadeResponsiveness", &fogManager->lerpFactor, 0.1f, 1);

  fogManager->minAlpha = *std::min(&fogManager->minAlpha, &fogManager->fallbackAlpha);
}

void UI::Render_Debug() {
  auto fogManager = FogManager::GetSingleton();
  std::lock_guard<std::mutex> lock(fogManager->trackedRefsLock);
  ImGui::Text("Tracked Refs");
  if (ImGui::BeginTable("##TrackedRefs", 4)) { 
    ImGui::TableSetupColumn("Ref ID");
    ImGui::TableSetupColumn("Ray Hit");
    ImGui::TableSetupColumn("Ray Count");
    ImGui::TableSetupColumn("Shape Details");
    ImGui::TableHeadersRow();

    int fogRefIdx = 0;
    for (const auto& fogRef : fogManager->trackedRefs) {
      auto handle = fogRef.handle;
      auto ref = handle.get();
      if (!ref) continue;
      ImGui::TableNextRow();

      ImGui::TableNextColumn();
      ImGui::Text("%08X", ref->GetFormID());
      
      ImGui::TableNextColumn();
      ImGui::Text(fogRef.rayHit ? "True" : "False");
      
      ImGui::TableNextColumn();
      ImGui::Text("%d", fogRef.rays.size());

      ImGui::TableNextColumn();
      ImGui::TableNextColumn();
      if (ImGui::TreeNode(std::format("Shapes##{:d}", fogRefIdx).c_str())) {
        ImGui::Indent();
        for (const auto& [name, shape] : fogRef.shapes) {
          ImGui::TextDisabled("%s", name.c_str());
          for (const auto& shader : shape) {
            ImGui::Text("  idx: %d  alpha: %.2f", shader.shaderIndex, shader.alpha);
          }
        }
        ImGui::Unindent();
        ImGui::TreePop();
      }

      fogRefIdx++;
    }
    ImGui::EndTable();
  }

  
  ImGui::Text("Draw Debug Rays");
  ImGui::Checkbox("##DrawDebugRays", &fogManager->debugRays);
}
} // namespace NullMod
