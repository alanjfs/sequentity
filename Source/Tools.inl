struct Activated { int time; };
struct Active { int time; };
struct Deactivated { int time; };

enum class ToolType : std::uint8_t {
    Select,
    DragSelect,
    LassoSelect,

    Translate,
    Rotate,
    Scale,

    Scrub,
};

struct Tool {
    ToolType type;
    std::function<void()> system;
};

struct Input1DRange  { int x; };

struct Input2DRange  {
    Vector2i absolute;
    Vector2i relative;
};

struct Input3DRange : Vector3i { using Vector3i::Vector3i; };

enum class InputDirection2D : std::uint8_t { Left = 0, Up, Right, Down };
enum class InputDirection3D : std::uint8_t { Left = 0, Up, Right, Down, Forward, Backward };


struct TranslateEventData {
    Vector2i offset;
    std::vector<Vector2i> positions;
};


struct RotateEventData {
    Deg offset;
    std::vector<Deg> angles;
};

struct ScaleEventData {
    float offset;
    std::vector<float> scale;
};

struct ScrubEventData {
    std::vector<int> deltas;
};

// Possible event types
enum EventType : Sequentity::EventType {
    InvalidEvent = 0,  // Catch uninitialised types

    SelectEvent,
    LassoSelectEvent,
    DragSelectEvent,

    TranslateEvent,
    RotateEvent,
    ScaleEvent,

    ScrubEvent,
};


static void SelectTool() {}


static void TranslateTool() {
    // Handle press input of type: 2D range, relative anything with a position
    Registry.view<Activated, Input2DRange, Color, Position>().each([](auto entity,
                                                                      const auto& activated,
                                                                      const auto& input,
                                                                      const auto& color,
                                                                      const auto& position
                                                                      ) {
        auto* data = new TranslateEventData{}; {
            data->offset = input.absolute - position;
            data->positions.push_back(input.absolute);
        }

        Sequentity::Event event; {
            event.time = activated.time + 1;
            event.length = 1;
            event.color = { color.stroke, color.fill };

            // Store reference to our data
            event.type = TranslateEvent;
            event.data = static_cast<void*>(data);
        }

        // Write Sequentity data..
        auto& channel = Registry.get_or_assign<Sequentity::Channel>(entity);
        channel.push_back(event);
    });

    Registry.view<Name, Active, Input2DRange, Sequentity::Channel>().each([](const auto& name,
                                                                             const auto&,
                                                                             const auto& input,
                                                                             auto& channel
                                                                             ) {
        auto& event = channel.back();

        if (event.type != TranslateEvent) return;

        auto data = static_cast<TranslateEventData*>(event.data);
        data->positions.push_back(input.absolute);
        event.length += 1;
    });

    Registry.view<Deactivated>().each([](auto entity, const auto&) {});
}


static void RotateTool() {}
static void ScaleTool() {}


static void ScrubTool() {

    // Press
    Registry.view<Activated, Input2DRange>(entt::exclude<Position>).each([](const auto& activated,
                                                     const auto& input) {
        auto* data = new ScrubEventData{};

        Sequentity::Event event; {
            event.time = activated.time + 1;
            event.length = 1;
            event.type = ScrubEvent;
            event.data = static_cast<void*>(data);
        }

        auto global = Registry.ctx<entt::entity>();
        auto& channel = Registry.get_or_assign<Sequentity::Channel>(global);
        channel.push_back(event);
    });

    // Hold
    Registry.view<Active, Input2DRange, Sequentity::Channel>().each([](const auto&,
                                                                       const auto& input,
                                                                       auto& channel) {
        auto& event = channel.back();

        if (event.type != ScrubEvent) return;

        auto data = static_cast<ScrubEventData*>(event.data);
        data->deltas.push_back(input.relative.x());
        event.length += 1;
    });

    // Release
    Registry.view<Deactivated>().each([](auto entity, const auto&) {});
}