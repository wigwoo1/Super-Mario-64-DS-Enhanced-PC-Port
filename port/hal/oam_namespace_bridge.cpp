// The recovered minigame base declares this overload as a free function in
// namespace OAM, while the recovered OAM manager defines it as a static method
// on the global OAM class. Preserve both observed ABI spellings.
struct OamAttr;

extern "C" void _ZN3OAM9RenderSubEP7OamAttriiii(
    OamAttr *data, int x, int y, int palette, int priority);

namespace OAM {
void RenderSub(OamAttr *data, int x, int y, int palette, int priority) {
    _ZN3OAM9RenderSubEP7OamAttriiii(data, x, y, palette, priority);
}
}
