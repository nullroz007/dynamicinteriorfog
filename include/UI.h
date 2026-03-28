#include <SKSEMenuFramework.h>
namespace NullMod {
class UI {
 public:
  static UI* GetSingleton() {
    static UI ui;
    return &ui;
  }

  void Init();
  static void __stdcall Render_Settings();
  static void __stdcall Render_Debug();
};
}  // namespace NullMod