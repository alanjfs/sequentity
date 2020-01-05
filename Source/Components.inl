// Components

struct BendollEvent {
    int time;
    int length;

    // Angles
    std::vector<float> data;
};

struct DragdollEvent {
};

using Position = Vector2i;
using Orientation = float;

struct InitialPosition : Vector2i { using Vector2i::Vector2i; };
struct StartPosition : Vector2i { using Vector2i::Vector2i; };
struct Size : Vector2i { using Vector2i::Vector2i; };
struct Index {
    int absolute { 0 };
    int relative { 0 };
};
struct Selected {};;
using BendollEvents = std::vector<BendollEvent>;
using DragdollEvents = std::vector<DragdollEvent>;

struct Color {
    ImVec4 fill;
    ImVec4 stroke;
};

struct Name {
    const char* text;
};

