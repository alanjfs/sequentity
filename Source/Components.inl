// Components

using Position = Vector2i;
using Orientation = float;

struct InitialPosition : Vector2i { using Vector2i::Vector2i; };
struct StartPosition : Vector2i { using Vector2i::Vector2i; };
struct Size : Vector2i { using Vector2i::Vector2i; };
struct Index {
    int absolute { 0 };
    int relative { 0 };
};
struct Selected {};

using Color = ImVec4;

struct Name {
    const char* text;
};

