#include <Config.h>
#include <SKSEMenuFramework.h>

using json = nlohmann::json;
using namespace SKSE;

namespace NullMod {
json Config::j = nullptr;
filesystem::path Config::_configPath;
template <typename T>
T Config::getFrom(const string& from, const string& name,
                  const T& fallbackValue) {
  try {
    if (j != nullptr) {
      return j.at(from).at(name).get<T>();
    } else {
      log::error("Config is not initialized! (Reading {})", name);
    }
  } catch (const exception& e) {
    log::error("Error fetching {} (child of {}) from splash.json: {}", name,
               from, e.what());
  }

  return fallbackValue;
}

template <typename T>
T Config::get(const string& name, const T& fallbackValue) {
  try {
    if (j != nullptr) {
      return j.at(name).get<T>();
    } else {
      log::error("Config is not initialized! (Reading {})", name);
    }
  } catch (const exception& e) {
    log::error("Error fetching {} from splash.json: {}", name, e.what());
  }

  return fallbackValue;
}

template <>
wstring Config::get<wstring>(const string& name, const wstring& fallbackValue) {
  try {
    if (j != nullptr) {
      std::string value = j.at(name).get<std::string>();
      int size_needed =
          MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
      std::wstring wValue(size_needed, 0);
      MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, wValue.data(),
                          size_needed);
      return wValue;

    } else {
      log::error("Config is not initialized! (Reading {})", name);
    }
  } catch (const exception& e) {
    log::error("Error fetching wstring {} from splash.json: {}", name,
               e.what());
  }
  return fallbackValue;
}

json& Config::GetConfig() { return j; }

bool Config::SaveConfig() {
  if (!IsInitialized()) return false;

  string jString = j.dump();
  std::ofstream output(_configPath);
  if (output.is_open()) {
    try {
      output.write(jString.c_str(), jString.size());
      return true;
    } catch (exception& ex) {
      log::error("Filesystem error: {}", ex.what());
    }

    output.close();
  } else {
    log::error("Failed to open: {}", _configPath.string());
  }

  return false;
}

void Config::Initialize(string path) {
  HINSTANCE hModule = GetModuleHandle(nullptr);
  wchar_t dllPath[MAX_PATH];
  GetModuleFileNameW(hModule, dllPath, MAX_PATH);
  filesystem::path configPath =
      filesystem::path(dllPath).parent_path() / L"Data\\SKSE\\Plugins\\";
  configPath /= path;

  log::info("Config Path: {}", configPath.string());

  std::ifstream f(configPath);
  if (!f.is_open()) {
    log::error("Failed to open config file: {}", configPath.string());
    return;
  }

  try {
    j = json::parse(f);
    log::info("Config loaded successfully");
    _configPath = configPath;
  } catch (const json::parse_error& e) {
    log::error("Failed to parse splash.json: {}", e.what());
  } catch (const exception& e) {
    log::error("Unexpected error parsing splash.json: {}", e.what());
  }
}

bool Config::IsInitialized() { return (Config::j != NULL); }

template int Config::get<int>(const std::string&, const int&);
template float Config::get<float>(const std::string&, const float&);
template double Config::get<double>(const std::string&, const double&);
template bool Config::get<bool>(const std::string&, const bool&);
template std::string Config::get<std::string>(const std::string&,
                                              const std::string&);
template std::wstring Config::get<std::wstring>(const std::string&,
                                                const std::wstring&);

template int Config::getFrom<int>(const std::string&, const std::string&,
                                  const int&);
template float Config::getFrom<float>(const std::string&, const std::string&,
                                      const float&);
template double Config::getFrom<double>(const std::string&, const std::string&,
                                        const double&);
template bool Config::getFrom<bool>(const std::string&, const std::string&,
                                    const bool&);
template std::string Config::getFrom<std::string>(const std::string&,
                                                  const std::string&,
                                                  const std::string&);
}  // namespace NullMod