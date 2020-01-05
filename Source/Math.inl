
inline ImVec4 operator*(const ImVec4& vec, const float mult) {
    return ImVec4{ vec.x * mult, vec.y * mult, vec.z * mult, vec.w };
}

inline ImVec2 operator+(const ImVec2& vec, const float value) {
    return ImVec2{ vec.x + value, vec.y + value };
}

inline ImVec2 operator*(const ImVec2& vec, const float value) {
    return ImVec2{ vec.x * value, vec.y * value };
}

inline ImVec2 operator*(const ImVec2& vec, const ImVec2 value) {
    return ImVec2{ vec.x * value.x, vec.y * value.y };
}

inline ImVec2 operator+(const ImVec2& vec, const ImVec2 value) {
    return ImVec2{ vec.x + value.x, vec.y + value.y };
}

inline ImVec2 operator-(const ImVec2& vec, const ImVec2 value) {
    return ImVec2{ vec.x - value.x, vec.y - value.y };
}