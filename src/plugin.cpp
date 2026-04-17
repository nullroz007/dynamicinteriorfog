#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>
#include <Config.h>
#include <FogManager.h>
#include <Hooks.h>
#include <UI.h>

using namespace SKSE;
using namespace SKSE::stl;
using namespace NullMod;

void InitLogging() {
  auto path = log::log_directory();
  spdlog::level::level_enum level;

  if (!path) report_and_fail("Unable to locate SKSE logs directory!");

  *path /= PluginDeclaration::GetSingleton()->GetName();
  *path += L".log";

  std::shared_ptr<spdlog::logger> log;

  if (IsDebuggerPresent()) {
    log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
    level = spdlog::level::trace;
  } else {
    log = std::make_shared<spdlog::logger>("Global",
      std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
    level = spdlog::level::info;
  }

  log->set_level(level);
  log->flush_on(level);

  spdlog::set_default_logger(std::move(log));
  spdlog::set_pattern("%s(%#): [%^%l%$] %v"s);
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
  SKSE::Init(skse);
  InitLogging();
  Config::GetSingleton()->Initialize("DynamicInteriorFog\\config.json");
  FogManager::GetSingleton()->Init();
  Hooks::Install();

  if (SKSEMenuFramework::IsInstalled()) { UI::GetSingleton()->Init(); }
  return true;
}