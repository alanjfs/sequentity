// Components

using TimeType = int;

using Position = Vector2i;

using Orientation = float;

struct InitialPosition : Position { using Position::Position; };
struct StartPosition : Position { using Position::Position; };

struct Size : Position { using Position::Position; };
struct InitialSize : Size { using Size::Size; };

using Index = unsigned int;

struct Preselected {};
struct Selected {};
struct Hovered {};

using Color = ImVec4;

struct Name {
    const char* text;
};

// Input

// From e.g. Wacom tablet
struct InputPressure  { float strength; };
struct InputPitch  { float angle; };
struct InputYaw  { float angle; };


// From e.g. Mouse or WASD keys
struct InputPosition2D {
    Vector2i absolute;
    Vector2i relative;
    Vector2i delta;
    Vector2i normalised;
    unsigned int width;
    unsigned int height;
};

struct InputPosition3D : Vector3i {
    using Vector3i::Vector3i;
};

// From e.g. WASD keys or D-PAD on XBox controller
enum class Direction2D : std::uint8_t { Left = 0, Up, Right, Down };
enum class Direction3D : std::uint8_t { Left = 0, Up, Right, Down, Forward, Backward };
