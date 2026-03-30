#pragma once
#include "nlohmann/json.hpp"
#include <fstream>

using json = nlohmann::json;
using namespace std;
namespace NullMod {
class Config {
 public:
  static Config* GetSingleton() {
    static Config c;
    return &c;
  }

  template <typename T>
  static T get(const std::string& name, const T& fallbackValue = T{});

  template <typename T>
  static T getFrom(const std::string& from, const std::string& name,
                   const T& fallbackValue = T{});
  static void Initialize(std::string path);
  static bool IsInitialized();
  static bool SaveConfig();
  static json& GetConfig();

 private:
  static filesystem::path _configPath;
  static json j;
};

}  // namespace NullMod