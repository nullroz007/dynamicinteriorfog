namespace NullMod {
class Hooks {
 public:
  static void Install();

 private:
  static void OnUpdate();
  static inline REL::Relocation<decltype(OnUpdate)> _OnUpdate;


};
}  // namespace NullMod