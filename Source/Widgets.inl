
using SmartButtonState = int;
enum SmartButtonState_ {
    SmartButtonState_None = 0,
    SmartButtonState_Hovered = 1 << 1,
    SmartButtonState_Pressed = 1 << 2,
    SmartButtonState_Dragged = 1 << 3,
    SmartButtonState_Released = 1 << 4
};


static SmartButtonState SmartButton(const char* label, ImVec2 size = {0, 0}) {
    bool useless = ImGui::Button(label, size);

    // Return value doesn't take into account release
    // when cursor is outside the bounds of the button.

    SmartButtonState state { 0 };
    if (ImGui::IsItemHovered()) state |= SmartButtonState_Hovered;

    if (ImGui::IsItemActivated()) {
        state |= SmartButtonState_Pressed;
    } else if (ImGui::IsItemActive()) {
        state |= SmartButtonState_Dragged;
    } else if (ImGui::IsItemDeactivated()) {
        state |= SmartButtonState_Released;
    }

    return state;
}
