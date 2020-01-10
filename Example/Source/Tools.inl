#include <vector>
#include <array>

/**
 * @brief Components and Data generated by these tools
 *
 * Tools translate application events into Sequentity events.
 * For example, a mouse click on a given entity results in
 * the `Activated` component being assigned. This then causes
 * the relevant portion of a tool to generate a new track, new
 * channel and new event.
 *
 * During press-and-hold, this newly created event is then mutated
 * with additional data, such as where the mouse is over time.
 * Incrementing the length of the event to line up with the amount
 * of data generated by the input.
 *
 */

// An entity has just been made active
struct Activated { int time; };

// An entity is being interacted with, e.g. dragged with mouse
struct Active { int time; };

// An entity transitioned from active to deactive
struct Deactivated { int time; };

// Halt an ongoing iteration of enities with an `Active` component
struct Abort {};

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
    std::function<void()> write;
    std::function<void(entt::entity entity, const Sequentity::Event& event, const int time)> read;
};

// From e.g. Wacom tablet
struct InputPressure  { float strength; };
struct InputPitch  { float angle; };
struct InputYaw  { float angle; };

// From e.g. Mouse or WASD keys
struct InputPosition2D  {
    Position absolute { 0, 0 };
    Position relative { 0, 0 };
};

struct InputPosition3D {
    Position absolute { 0, 0 };
    Position relative { 0, 0 };
};

// From e.g. WASD keys or D-PAD on XBox controller
enum class InputDirection2D : std::uint8_t { Left = 0, Up, Right, Down };
enum class InputDirection3D : std::uint8_t { Left = 0, Up, Right, Down, Forward, Backward };


/**
 * @brief Application data generated by these tools
 *
 * In addition to each Sequentity event, there is also our internal
 * application data, carried by events via a void*
 *
 */
struct TranslateEventData {
    Position offset;
    std::vector<Position> positions;
};

struct RotateEventData {
    float offset;
    std::vector<float> orientations;
};

struct ScaleEventData {
    std::vector<float> scales;
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


/**
 * @brief The simplest possible tool
 *
 *
 */
static void SelectTool() {
    Registry.view<Name, Deactivated>().each([](auto entity, const auto& name, const auto&) {
        
        // Ensure there is only ever 1 selected entity
        Registry.reset<Selected>();

        Registry.assign<Selected>(entity);
    });
}


/**
 * @brief Translate an entity
 *
 *      __________ 
 *     |          |
 *     |          | ----------->   
 *     |          |
 *     |__________|
 *
 *
 */
static void TranslateTool() {
    // Handle press input of type: 2D range, relative anything with a position

    Registry.view<Name, Activated, InputPosition2D, Color, Position>().each([](
                                                                      auto entity,
                                                                      const auto& name,
                                                                      const auto& activated,
                                                                      const auto& input,
                                                                      const auto& color,
                                                                      const auto& position) {

        // The default name for any new track is coming from the owning entity
        if (!Registry.has<Sequentity::Track>(entity)) {
            Registry.assign<Sequentity::Track>(entity, name.text, color);
        }

        auto* data = new TranslateEventData{}; {
            data->offset = input.absolute - position;
            data->positions.emplace_back(input.absolute);
        }

        auto& track = Registry.get<Sequentity::Track>(entity);
        bool has_channel = Sequentity::HasChannel(track, TranslateEvent);
        auto& channel = Sequentity::PushChannel(track, TranslateEvent);

        if (!has_channel) {
            channel.label = "Translate";
            channel.color = ImColor::HSV(0.0f, 0.75f, 0.75f);
        }

        Sequentity::PushEvent(channel, {
            activated.time + 1,                 /* time= */
            1,                                  /* length= */
            color,                              /* color= */

            // Store reference to our data
            TranslateEvent,                     /* type= */
            static_cast<void*>(data)            /* data= */
        });

        Registry.reset<Selected>();
        Registry.assign<Selected>(entity);
    });

    Registry.view<Active, InputPosition2D, Sequentity::Track>(entt::exclude<Abort>).each([](
                                                                const auto&,
                                                                const auto& input,
                                                                auto& track) {
        if (!track.channels.count(TranslateEvent)) {
            Warning() << "TranslateTool on" << track.label << "didn't have a TranslateEvent";
            return;
        }

        auto& channel = track.channels[TranslateEvent];
        auto& event = channel.events.back();

        auto data = static_cast<TranslateEventData*>(event.data);
        data->positions.emplace_back(input.absolute);
        event.length += 1;
    });

    Registry.view<Deactivated>().each([](auto entity, const auto&) {});
}


/**
 * @brief Rotate an entity
 *                  __
 *      __________     \
 *     |          |     v
 *     |          |   
 *     |          |
 *     |__________|
 *  ^
 *   \___
 *
 */
static void RotateTool() {
    Registry.view<Name, Activated, InputPosition2D, Color, Orientation>().each([](
                                                                         auto entity,
                                                                         const auto& name,
                                                                         const auto& activated,
                                                                         const auto& input,
                                                                         const auto& color,
                                                                         const auto& orientation) {
        auto* data = new RotateEventData{}; {
            data->offset = orientation;
            data->orientations.push_back(orientation);
        }

        if (!Registry.has<Sequentity::Track>(entity)) {
            Registry.assign<Sequentity::Track>(entity, name.text, color);
        }

        auto& track = Registry.get<Sequentity::Track>(entity);
        bool has_channel = track.channels.count(RotateEvent);
        auto& channel = track.channels[RotateEvent];

        if (!has_channel) {
            channel.label = "Rotate";
            channel.color = ImColor::HSV(0.33f, 0.75f, 0.75f);
        }

        Sequentity::PushEvent(channel, {
            activated.time + 1,
            1,
            color,

            RotateEvent,
            static_cast<void*>(data)
        });

        Registry.reset<Selected>();
        Registry.assign<Selected>(entity);
    });

    Registry.view<Name, Active, InputPosition2D, Sequentity::Track>(entt::exclude<Abort>).each([](const auto& name,
                                                                           const auto&,
                                                                           const auto& input,
                                                                           auto& track) {
        if (!track.channels.count(RotateEvent)) { Warning() << "RotateTool: This should never happen"; return; }

        auto& channel = track.channels[RotateEvent];
        auto& event = channel.events.back();

        auto data = static_cast<RotateEventData*>(event.data);
        data->orientations.push_back(static_cast<float>(data->offset + input.relative.x));
        event.length += 1;
    });

    Registry.view<Deactivated>().each([](auto entity, const auto&) {});
}


/**
 * @brief Scale an entity
 *
 *   \              /
 *    \ __________ /
 *     |          |
 *     |          |
 *     |          |
 *     |__________|
 *    /            \
 *   /              \
 *
 */
static void ScaleTool() {
    Registry.view<Name, Activated, InputPosition2D, Color, Size>().each([](
                                                                         auto entity,
                                                                         const auto& name,
                                                                         const auto& activated,
                                                                         const auto& input,
                                                                         const auto& color,
                                                                         const auto& size) {
        auto* data = new ScaleEventData{}; {
            data->scales.push_back(1.0f);
        }

        Sequentity::Event event; {
            event.time = activated.time + 1;
            event.length = 1;
            event.color = color;

            // Store reference to our data
            event.type = ScaleEvent;
            event.data = static_cast<void*>(data);
        }

        if (!Registry.has<Sequentity::Track>(entity)) {
            Registry.assign<Sequentity::Track>(entity, name.text, color);
        }

        auto& track = Registry.get<Sequentity::Track>(entity);
        bool has_channel = track.channels.count(ScaleEvent);
        auto& channel = track.channels[ScaleEvent];

        if (!has_channel) {
            channel.label = "Scale";
            channel.color = ImColor::HSV(0.52f, 0.75f, 0.50f);
        }

        Sequentity::PushEvent(channel, {
            activated.time + 1,
            1,
            color,

            ScaleEvent,
            static_cast<void*>(data)
        });

        Registry.reset<Selected>();
        Registry.assign<Selected>(entity);
    });

    Registry.view<Name, Active, InputPosition2D, Sequentity::Track>(entt::exclude<Abort>).each([](const auto& name,
                                                                           const auto&,
                                                                           const auto& input,
                                                                           auto& track) {
        if (!track.channels.count(ScaleEvent)) { Warning() << "ScaleTool: This should never happen"; return; }

        auto& channel = track.channels[ScaleEvent];
        auto& event = channel.events.back();

        auto data = static_cast<ScaleEventData*>(event.data);
        data->scales.push_back(1.0f + input.relative.x * 0.01f);
        event.length += 1;
    });

    Registry.view<Deactivated>().each([](auto entity, const auto&) {});
}


/**
 * @brief Relatively move the timeline
 *
 * This tool differs from the others, in that it doesn't actually apply to the
 * active entity. Instead, it applies to the global state of Sequentity. But,
 * it currently can't do that, unless an entity is active. So that's a bug.
 *
 */
static void ScrubTool() {
    // Press
    static int previous_time { 0 };
    Registry.view<Activated, InputPosition2D>().each([](const auto& activated, const auto& input) {
        auto& state = Registry.ctx<Sequentity::State>();
        previous_time = state.current_time;
    });

    // Hold
    Registry.view<Active, InputPosition2D>().each([](const auto&, const auto& input) {
        auto& state = Registry.ctx<Sequentity::State>();
        state.current_time = previous_time + input.relative.x / 10;
    });

    // Release
    Registry.view<Deactivated>().each([](auto entity, const auto&) {});
}
