
using SmartButtonState = int;
enum SmartButtonState_ {
    SmartButtonState_None = 0,
    SmartButtonState_Hovered = 1 << 1,
    SmartButtonState_Pressed = 1 << 2,
    SmartButtonState_Dragged = 1 << 3,
    SmartButtonState_Released = 1 << 4
};


static SmartButtonState SmartButton(const char* label,
                                    ImVec2 pos,
                                    ImVec2 size = { 50, 50 },
                                    float angle = 0.0f,
                                    ImVec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }) {
    auto corner = ImGui::GetWindowPos();
    auto& painter = *ImGui::GetWindowDrawList();

    auto coord = ImVec2{ size.x / 2.0f, size.y / 2.0f };

    ImGui::SetCursorPos(pos - coord);
    ImGui::InvisibleButton(label, size);

    /**
     *
     *  a           b
     *    _________ 
     *   |         |
     *   |         |
     *   |         |
     *   |         |
     *   |_________|
     *               
     *  c           d
     * 
     *
     */

    auto rotate = [](ImVec2 v, float radians) -> ImVec2 {
        return {
            v.x * cos(radians) - v.y * sin(radians),
            v.y * cos(radians) + v.x * sin(radians)
        };
    };

    const auto rad = static_cast<float>(Rad(Deg(angle)));
    auto a = rotate(ImVec2{ -coord.x, -coord.y }, rad);
    auto b = rotate(ImVec2{  coord.x, -coord.y }, rad);
    auto c = rotate(ImVec2{ -coord.x,  coord.y }, rad);
    auto d = rotate(ImVec2{  coord.x,  coord.y }, rad);

    painter.AddQuadFilled(
        corner + pos + a,
        corner + pos + b,
        corner + pos + d,
        corner + pos + c,
        ImColor(color)
    );

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
