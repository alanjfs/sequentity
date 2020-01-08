// Components

struct Position {
    int x { 0 };
    int y { 0 };
};

using Orientation = float;

struct InitialPosition : Position {};
struct StartPosition : Position {};
struct Size : Position {};

struct Index {
    int absolute { 0 };
    int relative { 0 };
};

struct Selected {};

using Color = ImVec4;

struct Name {
    const char* text;
};

// Math

inline Position operator*(const Position& pos, const Position other) {
    return Position{ pos.x * other.x, pos.y * other.y };
}

inline Position operator+(const Position& pos, const Position other) {
    return Position{ pos.x + other.x, pos.y + other.y };
}

inline Position operator-(const Position& pos, const Position other) {
    return Position{ pos.x - other.x, pos.y - other.y };
}