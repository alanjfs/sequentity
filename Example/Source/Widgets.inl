
namespace Widgets {


inline ImVec4 operator*(const ImVec4& vec, const float mult) {
    return ImVec4{ vec.x * mult, vec.y * mult, vec.z * mult, vec.w };
}

inline ImVec2 operator+(const ImVec2& vec, const float value) {
    return ImVec2{ vec.x + value, vec.y + value };
}

inline ImVec2 operator+(const ImVec2& vec, const ImVec2 value) {
    return ImVec2{ vec.x + value.x, vec.y + value.y };
}

inline void operator-=(ImVec2& vec, const float value) {
    vec.x -= value;
    vec.y -= value;
}

inline ImVec2 operator-(const ImVec2& vec, const float value) {
    return ImVec2{ vec.x - value, vec.y - value };
}

inline ImVec2 operator-(const ImVec2& vec, const ImVec2 value) {
    return ImVec2{ vec.x - value.x, vec.y - value.y };
}

inline ImVec2 operator*(const ImVec2& vec, const float value) {
    return ImVec2{ vec.x * value, vec.y * value };
}

inline ImVec2 operator*(const ImVec2& vec, const ImVec2 value) {
    return ImVec2{ vec.x * value.x, vec.y * value.y };
}


static bool Graphic(const char* label,
                    ImVec2 pos,
                    ImVec2 size = { 50, 50 },
                    float angle = 0.0f,
                    ImVec4 color = { 1.0f, 1.0f, 1.0f, 1.0f },
                    bool selected = false) {

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

    if (selected) {
        painter.AddQuad(
            corner + pos + a,
            corner + pos + b,
            corner + pos + d,
            corner + pos + c,
            ImColor::HSV(0.0f, 0.0f, 1.0f),
            6.0f
        );
    }

    // Shadow
    painter.AddQuadFilled(
        corner + pos + a + 5.0,
        corner + pos + b + 5.0,
        corner + pos + d + 5.0,
        corner + pos + c + 5.0,
        ImColor(0.0f, 0.0f, 0.0f, 0.1f)
    );

    painter.AddQuadFilled(
        corner + pos + a,
        corner + pos + b,
        corner + pos + d,
        corner + pos + c,
        ImColor(color)
    );

    return ImGui::IsItemActive();
}


void Cursor(ImVec2 corner, ImColor color) {
    auto root = ImGui::GetWindowPos();
    auto painter = ImGui::GetWindowDrawList();

    auto abscorner = root + corner;

    painter->AddCircleFilled(
        abscorner,
        10.0f,
        ImColor::HSV(0.0f, 0.0f, 1.0f)
    );
}


auto Button(const char* label, bool checked, float width = 100.0f) -> bool {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 10.0f, 20.0f });

    if (checked) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0.25f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0.15f));
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0.1f));
    }

    const bool pressed = ImGui::Button(label, {width, 0});

    if (checked) ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    return pressed;
}


auto RecordButton(const char* label, bool checked, float width = 100.0f) -> bool {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 10.0f, 20.0f });

    if (checked) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor::HSV(0.0f, 0.7f, 0.7f)));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor::HSV(0.0f, 0.7f, 0.8f)));
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0.1f));
    }

    const bool pressed = ImGui::Button(label, {width, 0});

    if (checked) ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    return pressed;
}

}