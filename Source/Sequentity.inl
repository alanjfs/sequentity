/**

---===  Sequentity  ===---

An immediate-mode sequencer, written in C++ with ImGui, Magnum and EnTT

---=== === === === === ---

- ~2,000 vertices with 15 events, about 1,500 without any
- Magnum is an open-source OpenGL wrapper with window management and math library baked in
- EnTT is a entity-component-system (ECS) framework

Both of which are optional and unrelated to the sequencer itself, but needed
to get a window on screen and to manage relevant data.


### Legend

- Event:   An individual coloured bar, with a start, length and additional metadata
- Channel: A vector of Events
- Track: A vector of Channels

 ___________ __________________________________________________
|           |                                                  |
|-----------|--------------------------------------------------|
| Track     |                                                  |
|   Channel |  Event Event Event                               |
|   Channel |  Event Event Event                               |
|   Channel |  Event Event Event                               |
|   ...     |  ...                                             |
|___________|__________________________________________________|


### Usage

Data is passed between Sequentity and your host application via the Event
and Channel structs below.

*/

#include <functional>
#include <vector>

namespace Sequentity {

using EventType = int;

// Some example events
enum EventType_ : std::uint8_t {
    EventType_Move = 0,
    EventType_Rotate,
    EventType_Scale
};

/**
 * @brief A Sequentity Event
 *
 */
struct Event {
    int time { 0 };
    int length { 0 };

    /**
     * @brief Ignore start and end of event
     *
     * E.g. crop = { 2, 4 };
     *       ______________________________________
     *      |//|                              |////|
     *      |//|______________________________|////|
     *      |  |                              |    |
     *      |--|                              |----|
     *  2 cropped from start             4 cropped from end
     *
     */
    int crop[2] { 0, 0 };

    /* Whether or not to consider this event */
    bool enabled { true };

    /* Events are never really deleted, just hidden from view */
    bool removed { false };

    /* Extend or reduce the length of an event */
    float scale { 1.0f };

    ImVec4 color { ImColor::HSV(0.0f, 0.0f, 1.0f) };

    // Visuals, animation
    float height { 0.0f };
    float thickness { 0.0f };

    // Map your custom data here, along with an optional type
    void* data { nullptr };
    EventType type { EventType_Move };
};

/**
 * @brief A collection of events
 *
 */
struct Channel {
    const char* label { "Untitled channel" };
    ImVec4 color { ImColor::HSV(0.33f, 0.5f, 1.0f) };
    std::vector<Event> events;
};

/**
 * @brief A collection of channels
 *
 */
struct Track {
    const char* label { "Untitled track" };
    ImVec4 color { ImColor::HSV(0.66f, 0.5f, 1.0f) };
    std::unordered_map<EventType, Channel> channels;
};


/**
 * @brief Callbacks
 *
 */
using OverlappingCallback = std::function<void(const Event&)>;
using TimeChangedCallback = std::function<void()>;

struct Selected {};


/**
 * @brief All of Sequentities (mutable) state, accessible from your application
 *
 */
struct State {

    // Functional
    bool playing { false };
    int current_time { 0 };
    Vector2i range { 0, 100 };

    Event* selection { nullptr };

    // Visual
    float zoom[2] { 200.0f, 30.0f };
    float pan[2] { 8.0f, 8.0f };
    int stride { 2 };

    // Transitions
    float target_zoom[2] { 250.0f, 30.0f };
    float target_pan[2] { 15.0f, 20.0f };
};


/**
 * @brief Handy math overloads, for readability
 *
 */

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


/**
 * @brief Main window
 *
 */
struct Sequentity {

    // Sequentity reads Event and Channel components from your EnTT registry
    // but the involvement of EnTT is minimal and easily replaced by your own
    // implementation. See any reference to _registry below.
    Sequentity(entt::registry& registry);

    void draw(bool* p_open = nullptr);
    void drawThemeEditor(bool* p_open = nullptr);

    void update();
    void clear();

    void play();
    void step(int delta);
    void stop();

    /**  Find intersecting events
     *
     *               time
     *                 |
     *    _____________|__________
     *   |_____________|__________|
     *   ^             |
     * event           |
     *                 
     */
    const Event* overlapping(const Track& channel);
    void each_overlapping(const Track& channel, OverlappingCallback func);


public:
    // Callbacks
    void before_time_changed(TimeChangedCallback func) { _before_time_changed = func; }
    void after_time_changed(TimeChangedCallback func) { _after_time_changed = func; }

private:
    entt::registry& _registry;
    int _previous_time { 0 };

    TimeChangedCallback _before_time_changed = []() {};
    TimeChangedCallback _after_time_changed = []() {};

    void _sort();
    void _on_new_track();
};


/**
 * @brief A summary of all colours available to Sequentity
 *
 * Hint: These can be edited interactively via the `drawThemeEditor()` window
 *
 */

static struct GlobalTheme_ {
    ImVec4 dark         { ImColor::HSV(0.0f, 0.0f, 0.3f) };
    ImVec4 shadow       { ImColor::HSV(0.0f, 0.0f, 0.0f, 0.1f) };

    float border_width { 2.0f };
    float track_height { 25.0f };
    float transition_speed { 0.2f };

} GlobalTheme;


static struct ListerTheme_ {
    ImVec4 background  { ImColor::HSV(0.0f, 0.0f, 0.200f) };
    ImVec4 alternate   { ImColor::HSV(0.0f, 0.00f, 1.0f, 0.02f) };
    ImVec4 text        { ImColor::HSV(0.0f, 0.0f, 0.850f) };
    ImVec4 dark        { ImColor::HSV(0.0f, 0.0f, 0.150f) };
    ImVec4 mid         { ImColor::HSV(0.0f, 0.0f, 0.314f) };
    ImVec4 accent      { ImColor::HSV(0.0f, 0.75f, 0.750f) };
    
    float width { 150.0f };

} ListerTheme;


static struct TimelineTheme_ {
    ImVec4 background   { ImColor::HSV(0.0f, 0.0f, 0.250f) };
    ImVec4 text         { ImColor::HSV(0.0f, 0.0f, 0.850f) };
    ImVec4 dark         { ImColor::HSV(0.0f, 0.0f, 0.322f) };
    ImVec4 mid          { ImColor::HSV(0.0f, 0.0f, 0.314f) };
    ImVec4 start_time   { ImColor::HSV(0.33f, 0.0f, 0.50f) };
    ImVec4 current_time { ImColor::HSV(0.57f, 0.62f, 0.99f) };
    ImVec4 end_time     { ImColor::HSV(0.0f, 0.0f, 0.40f) };

    float height { 40.0f };

} TimelineTheme;


static struct EditorTheme_ {
    ImVec4 background    { ImColor::HSV(0.0f, 0.00f, 0.651f) };
    ImVec4 alternate     { ImColor::HSV(0.0f, 0.00f, 0.0f, 0.02f) };
    ImVec4 text          { ImColor::HSV(0.0f, 0.00f, 0.950f) };
    ImVec4 mid           { ImColor::HSV(0.0f, 0.00f, 0.600f) };
    ImVec4 dark          { ImColor::HSV(0.0f, 0.00f, 0.498f) };
    ImVec4 accent        { ImColor::HSV(0.0f, 0.75f, 0.750f) };
    ImVec4 accent_light  { ImColor::HSV(0.0f, 0.5f, 1.0f, 0.1f) };
    ImVec4 accent_dark   { ImColor::HSV(0.0f, 0.0f, 0.0f, 0.1f) };
    ImVec4 selection     { ImColor::HSV(0.0f, 0.0f, 1.0f) };
    ImVec4 outline       { ImColor::HSV(0.0f, 0.0f, 0.1f) };
    ImVec4 track         { ImColor::HSV(0.0f, 0.0f, 0.6f) };

    ImVec4 start_time    { ImColor::HSV(0.33f, 0.0f, 0.25f) };
    ImVec4 current_time  { ImColor::HSV(0.6f, 0.5f, 0.5f) };
    ImVec4 end_time      { ImColor::HSV(0.0f, 0.0f, 0.25f) };
    
    float radius { 0.0f };

} EditorTheme;


Sequentity::Sequentity(entt::registry& registry) : _registry(registry) {
    
    // Store state such that it is accessible externally
    registry.ctx_or_set<State>();

    registry.on_construct<Track>().connect<&Sequentity::_on_new_track>(*this);
}


void Sequentity::_on_new_track() {
    _sort();
}


void Sequentity::_sort() {
    _registry.sort<Index>([this](const entt::entity lhs, const entt::entity rhs) {
        return _registry.get<Index>(lhs).absolute < _registry.get<Index>(rhs).absolute;
    });
}


void Sequentity::clear() {
    Registry.reset<Track>();
}


void Sequentity::play() {
    _registry.ctx<State>().playing ^= true;
}


void Sequentity::stop() {
    auto& state = _registry.ctx<State>();

    state.current_time = state.range.x();
    state.playing = false;
}


void Sequentity::update() {
    auto& state = _registry.ctx<State>();

    if (state.playing) {
        step(1);
    }

    if (state.current_time != _previous_time) {
        _after_time_changed();
        _previous_time = state.current_time;
    }
}


void Sequentity::step(int delta) {
    auto& state = _registry.ctx<State>();
    auto time = state.current_time + delta;

    if (time > state.range.y()) {
        time = state.range.x();
    }

    else if (time < state.range.x()) {
        time = state.range.y();
    }

    state.current_time = time;
}


bool contains(const Event& event, int time) {
    return (
        event.time <= time &&
        event.time + event.length > time
    );
}

const Event* Sequentity::overlapping(const Track& track) {
    const Event* intersecting { nullptr };

    for (auto& [type, channel] : track.channels) {
        for (auto& event : channel.events) {
            if (event.removed) continue;
            if (!event.enabled) continue;

            if (contains(event, _registry.ctx<State>().current_time)) {
                intersecting = &event;

                // Ignore overlapping
                // TODO: Remove underlapping
                break;
            }
        }
    }
    return intersecting;
}


void Sequentity::each_overlapping(const Track& track, OverlappingCallback func) {
    for (auto& [type, channel] : track.channels) {
        for (auto& event : channel.events) {
            if (event.removed) continue;
            if (!event.enabled) continue;
            if (contains(event, _registry.ctx<State>().current_time)) func(event);
        }
    }
}


void Sequentity::drawThemeEditor(bool* p_open) {
    ImGui::Begin("Theme", p_open);
    {
        if (ImGui::CollapsingHeader("Global")) {
            ImGui::ColorEdit4("dark##global", &GlobalTheme.dark.x);
            ImGui::ColorEdit4("shadow##global", &GlobalTheme.shadow.x);
            ImGui::DragFloat("transition_speed##global", &GlobalTheme.transition_speed);
            ImGui::DragFloat("track_height##global", &GlobalTheme.track_height);
            ImGui::DragFloat("border_width##global", &GlobalTheme.border_width);
        }

        if (ImGui::CollapsingHeader("Timeline")) {
            ImGui::ColorEdit4("background", &TimelineTheme.background.x);
            ImGui::ColorEdit4("text", &TimelineTheme.text.x);
            ImGui::ColorEdit4("dark", &TimelineTheme.dark.x);
            ImGui::ColorEdit4("mid", &TimelineTheme.mid.x);
            ImGui::ColorEdit4("start_time", &TimelineTheme.start_time.x);
            ImGui::ColorEdit4("current_time", &TimelineTheme.current_time.x);
            ImGui::ColorEdit4("end_time", &TimelineTheme.end_time.x);
            ImGui::DragFloat("height", &TimelineTheme.height);
        }

        if (ImGui::CollapsingHeader("Editor")) {
            ImGui::ColorEdit4("background##editor", &EditorTheme.background.x);
            ImGui::ColorEdit4("alternate##editor", &EditorTheme.alternate.x);
            ImGui::ColorEdit4("text##editor", &EditorTheme.text.x);
            ImGui::ColorEdit4("mid##editor", &EditorTheme.mid.x);
            ImGui::ColorEdit4("dark##editor", &EditorTheme.dark.x);
            ImGui::ColorEdit4("accent##editor", &EditorTheme.accent.x);

            ImGui::ColorEdit4("start_time##editor", &EditorTheme.start_time.x);
            ImGui::ColorEdit4("current_time##editor", &EditorTheme.current_time.x);
            ImGui::ColorEdit4("end_time##editor", &EditorTheme.end_time.x);
        }

        if (ImGui::CollapsingHeader("Lister")) {
            ImGui::ColorEdit4("background##lister", &ListerTheme.background.x);
            ImGui::ColorEdit4("text##lister", &ListerTheme.text.x);
            ImGui::ColorEdit4("dark##lister", &ListerTheme.dark.x);
            ImGui::ColorEdit4("mid##lister", &ListerTheme.mid.x);
            ImGui::DragFloat("width##lister", &ListerTheme.width);
        }
    }
    ImGui::End();
}


void Sequentity::draw(bool* p_open) {
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar
                                 | ImGuiWindowFlags_NoScrollWithMouse;

    // Animate between current and target value, at a given velocity (ignoring deltas smaller than epsilon)
    auto transition = [](float& current, float target, float velocity, float epsilon = 0.1f) {
        const float delta = target - current;

        // Prevent transitions between too small values
        // (Especially damaging to fonts)
        if (abs(delta) < epsilon) current = target;

        current += delta * velocity;
    };

    auto& state = _registry.ctx<State>();
    transition(state.pan[0], state.target_pan[0], GlobalTheme.transition_speed);
    transition(state.pan[1], state.target_pan[1], GlobalTheme.transition_speed);
    transition(state.zoom[0], state.target_zoom[0], GlobalTheme.transition_speed);
    transition(state.zoom[1], state.target_zoom[1], GlobalTheme.transition_speed);

    ImGui::Begin("Editor", p_open, windowFlags);
    {
        auto* painter = ImGui::GetWindowDrawList();
        auto titlebarHeight = 24.0f; // TODO: Find an exact value for this
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 windowPos = ImGui::GetWindowPos() + ImVec2{ 0.0f, titlebarHeight };

        /**
         * @brief Sequentity divided into 4 panels
         *
         *          _________________________________________________
         *         |       |                                         |
         *   Cross |   X   |                  B                      | Timeline
         *         |_______|_________________________________________|
         *         |       |                                         |
         *         |       |                                         |
         *         |       |                                         |
         *  Lister |   A   |                  C                      | Editor
         *         |       |                                         |
         *         |       |                                         |
         *         |_______|_________________________________________|
         *
         */

        auto X = windowPos;
        auto A = windowPos + ImVec2{ 0.0f, TimelineTheme.height };
        auto B = windowPos + ImVec2{ ListerTheme.width, 0.0f };
        auto C = windowPos + ImVec2{ ListerTheme.width, TimelineTheme.height };

        float zoom_ = state.zoom[0] / state.stride;
        int stride_ = state.stride * 5;  // How many frames to skip drawing
        int minTime = state.range.x() / stride_;
        int maxTime = state.range.y() / stride_;

        // TODO: What is this value?
        auto multiplier = zoom_ / stride_;

        auto time_to_px = [&multiplier](int time)  -> float { return time * multiplier; };
        auto px_to_time = [&multiplier](float px)  -> int   { return static_cast<int>(px / multiplier); };

        auto CrossBackground = [&]() {
            painter->AddRectFilled(
                X,
                X + ImVec2{ ListerTheme.width + 1, TimelineTheme.height },
                ImColor(ListerTheme.background)
            );

            // Border
            painter->AddLine(X + ImVec2{ ListerTheme.width, 0.0f },
                             X + ImVec2{ ListerTheme.width, TimelineTheme.height },
                             ImColor(GlobalTheme.dark),
                             GlobalTheme.border_width);

            painter->AddLine(X + ImVec2{ 0.0f, TimelineTheme.height },
                             X + ImVec2{ ListerTheme.width + 1, TimelineTheme.height },
                             ImColor(GlobalTheme.dark),
                             GlobalTheme.border_width);
        };

        auto ListerBackground = [&]() {
            // Drop Shadow
            painter->AddRectFilled(
                A,
                A + ImVec2{ ListerTheme.width + 3.0f, windowSize.y },
                ImColor(0.0f, 0.0f, 0.0f, 0.1f)
            );
            painter->AddRectFilled(
                A,
                A + ImVec2{ ListerTheme.width + 2.0f, windowSize.y },
                ImColor(0.0f, 0.0f, 0.0f, 0.2f)
            );

            // Fill
            painter->AddRectFilled(
                A,
                A + ImVec2{ ListerTheme.width, windowSize.y },
                ImColor(ListerTheme.background)
            );

            // Border
            painter->AddLine(A + ImVec2{ ListerTheme.width, 0.0f },
                             A + ImVec2{ ListerTheme.width, windowSize.y },
                             ImColor(GlobalTheme.dark),
                             GlobalTheme.border_width);
        };

        auto TimelineBackground = [&]() {
            // Drop Shadow
            painter->AddRectFilled(
                B,
                B + ImVec2{ windowSize.x, TimelineTheme.height + 3.0f },
                ImColor(0.0f, 0.0f, 0.0f, 0.1f)
            );
            painter->AddRectFilled(
                B,
                B + ImVec2{ windowSize.x, TimelineTheme.height + 2.0f },
                ImColor(0.0f, 0.0f, 0.0f, 0.2f)
            );

            // Fill
            painter->AddRectFilled(
                B,
                B + ImVec2{ windowSize.x, TimelineTheme.height },
                ImColor(TimelineTheme.background)
            );

            // Border
            painter->AddLine(B + ImVec2{ 0.0f, TimelineTheme.height },
                             B + ImVec2{ windowSize.x, TimelineTheme.height },
                             ImColor(GlobalTheme.dark),
                             GlobalTheme.border_width);
        };

        auto EditorBackground = [&]() {
            painter->AddRectFilled(
                C,
                C + windowPos + windowSize,
                ImColor(EditorTheme.background)
            );
        };

        auto Timeline = [&]() {
            for (int time = minTime; time < maxTime + 1; time++) {
                float xMin = time * zoom_;
                float xMax = 0.0f;
                float yMin = 0.0f;
                float yMax = TimelineTheme.height - 1;

                xMin += B.x + state.pan[0];
                xMax += B.x + state.pan[0];
                yMin += B.y;
                yMax += B.y;

                painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), ImColor(TimelineTheme.dark));
                painter->AddText(
                    ImGui::GetFont(),
                    ImGui::GetFontSize() * 0.85f,
                    ImVec2{ xMin + 5.0f, yMin },
                    ImColor(TimelineTheme.text),
                    std::to_string(time * stride_).c_str()
                );

                if (time == maxTime) break;

                for (int z = 0; z < 5 - 1; z++) {
                    const auto innerSpacing = zoom_ / 5;
                    auto subline = innerSpacing * (z + 1);
                    painter->AddLine(
                        ImVec2{ xMin + subline, yMin + (TimelineTheme.height * 0.5f) },
                        ImVec2{ xMin + subline, yMax },
                        ImColor(TimelineTheme.mid));
                }
            }
        };
        /**
         * @brief Vertical grid lines
         *        
         *   ______________________________________________________
         *  |    |    |    |    |    |    |    |    |    |    |    |
         *  |    |    |    |    |    |    |    |    |    |    |    |
         *  |    |    |    |    |    |    |    |    |    |    |    |
         *  |    |    |    |    |    |    |    |    |    |    |    |
         *  |    |    |    |    |    |    |    |    |    |    |    |
         *  |____|____|____|____|____|____|____|____|____|____|____|
         *
         */
        auto VerticalGrid = [&]() {
            for (int time = minTime; time < maxTime + 1; time++) {
                float xMin = time * zoom_;
                float xMax = 0.0f;
                float yMin = 0.0f;
                float yMax = windowSize.y;

                xMin += C.x + state.pan[0];
                xMax += C.x + state.pan[0];
                yMin += C.y;
                yMax += C.y;

                painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), ImColor(EditorTheme.dark));

                if (time == maxTime) break;

                for (int z = 0; z < 5 - 1; z++) {
                    const auto innerSpacing = zoom_ / 5;
                    auto subline = innerSpacing * (z + 1);
                    painter->AddLine(ImVec2(xMin + subline, yMin), ImVec2(xMin + subline, yMax), ImColor(EditorTheme.mid));
                }
            }
        };

        /**
         * @brief Draw time indicator
         *         __
         *        |  |
         *  _______\/___________________________________________________
         *         |
         *         |
         *         |
         *         |
         *         |
         *
         */
        unsigned int indicator_count { 0 };
        auto TimeIndicator = [&](int& time, ImVec4 cursor_color, ImVec4 line_color) {
            auto xMin = time * zoom_ / stride_;
            auto xMax = 0.0f;
            auto yMin = TimelineTheme.height;
            auto yMax = windowSize.y;

            xMin += B.x + state.pan[0];
            xMax += B.x + state.pan[0];
            yMin += B.y;
            yMax += B.y;

            painter->AddLine(ImVec2{ xMin, yMin }, ImVec2{ xMin, yMax }, ImColor(line_color), 2.0f);

            /**
             *  Cursor
             *    __
             *   |  |
             *    \/
             *
             */
            ImVec2 size { 10.0f, 20.0f };
            auto topPos = ImVec2{ xMin, yMin };

            std::string id { "##indicator" };
            id += std::to_string(indicator_count);

            ImGui::SetCursorPos(topPos - ImVec2{ size.x, size.y } - ImGui::GetWindowPos());
            ImGui::InvisibleButton(id.c_str(), size * 2.0f);

            static unsigned int initial_time { 0 };
            if (ImGui::IsItemActivated()) {
                initial_time = time;
            }

            ImVec4 color = cursor_color;
            if (ImGui::IsItemHovered()) {
                color = color * 1.2f;
            }

            if (ImGui::IsItemActive()) {
                time = initial_time + static_cast<int>(ImGui::GetMouseDragDelta().x / (zoom_ / stride_));
            }

            ImVec2 points[5] = {
                topPos,
                topPos - ImVec2{ -size.x, size.y / 2.0f },
                topPos - ImVec2{ -size.x, size.y },
                topPos - ImVec2{ size.x, size.y },
                topPos - ImVec2{ size.x, size.y / 2.0f }
            };

            ImVec2 shadow1[5];
            ImVec2 shadow2[5];
            for (int i=0; i < 5; i++) { shadow1[i] = points[i] + ImVec2{ 1.0f, 1.0f }; }
            for (int i=0; i < 5; i++) { shadow2[i] = points[i] + ImVec2{ 3.0f, 3.0f }; }

            painter->AddConvexPolyFilled(shadow1, 5, ImColor(GlobalTheme.shadow));
            painter->AddConvexPolyFilled(shadow2, 5, ImColor(GlobalTheme.shadow));
            painter->AddConvexPolyFilled(points, 5, ImColor(color));
            painter->AddPolyline(points, 5, ImColor(color * 1.25f), true, 1.0f);
            painter->AddLine(topPos - ImVec2{  2.0f, size.y * 0.3f },
                             topPos - ImVec2{  2.0f, size.y * 0.8f },
                             ImColor(EditorTheme.accent_dark));
            painter->AddLine(topPos - ImVec2{ -2.0f, size.y * 0.3f },
                             topPos - ImVec2{ -2.0f, size.y * 0.8f },
                             ImColor(EditorTheme.accent_dark));

            indicator_count++;
        };

        /**
         * @brief Filled bars in the background
         *
         *  __________________________________________________________
         * ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
         *  __________________________________________________________
         * ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
         *  __________________________________________________________
         * ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
         *  __________________________________________________________
         * ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
         *
         */
        auto HorizontalGrid = [&]() {
            float yMin = 0.0f;
            float yMax = windowSize.y;
            float xMin = 0.0f;
            float xMax = windowSize.x;

            xMin += A.x;
            xMax += A.x;
            yMin += A.y + state.pan[1];
            yMax += A.y + state.pan[1];

            bool isOdd = false;
            for (float y = yMin; y < yMax; y += state.zoom[1]) {
                isOdd ^= true;

                if (isOdd) painter->AddRectFilled(
                    { xMin, y },
                    { xMax, y + state.zoom[1] - 1 },
                    ImColor(EditorTheme.alternate)
                );
            }
        };

        /**
         * @brief MIDI-like representation of on/off events
         *
         *   _______       _______________
         *  |_______|     |_______________|
         *       _______________
         *      |_______________|
         *                          _________________________
         *                         |_________________________|
         *
         */
        auto Events = [&]() {
            ImVec2 cursor { C.x + state.pan[0], C.y + state.pan[1] };

            _registry.view<Index, Track>().each<Index>([&](const auto, auto& track) {

                // Draw track header, a separator-like empty space
                // 
                // |__|__|__|__|__|__|__|__|__|__|__|__|__|__|
                // |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
                // |__|__|__|__|__|__|__|__|__|__|__|__|__|__|
                // |                                         |
                // |_________________________________________|
                // |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
                // |__|__|__|__|__|__|__|__|__|__|__|__|__|__|
                // |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
                //
                static ImVec2 button_size { 15.0f, 15.0f };
                static ImVec2 padding { 10.0f, 2.0f };
                static ImVec2 center { 0.0f, GlobalTheme.track_height / 2.0f };

                ImVec2 size { windowSize.x, GlobalTheme.track_height };

                painter->AddRectFilled(
                    cursor,
                    cursor + size,
                    ImColor(EditorTheme.background)
                );

                painter->AddRect(
                    cursor,
                    cursor + size,
                    ImColor(EditorTheme.mid)
                );

                cursor.y += size.y;

                // Give each event a unique ImGui ID
                unsigned int type_count { 0 };
                unsigned int event_count { 0 };

                // Draw events
                //            ________
                //           |________|
                //  _________________________
                // |_________________________|
                //     ________      ____________________________
                //    |________|    |____________________________|
                //
                for (auto& [type, channel] : track.channels) {
                    for (auto& event : channel.events) {

                        // Transitions
                        float target_height { 0.0f };
                        float target_thickness = 0.0f;

                        ImVec2 pos { time_to_px(event.time), 0.0f };
                        ImVec2 size { time_to_px(event.length), state.zoom[1] };

                        std::string id { "##event" };
                        id += track.label;
                        id += std::to_string(event_count);
                        ImGui::SetCursorPos(cursor + pos - ImGui::GetWindowPos());
                        ImGui::SetItemAllowOverlap();
                        ImGui::InvisibleButton(id.c_str(), size);

                        ImVec4 color = event.color;

                        if (!event.enabled) {
                            color = ImColor::HSV(0.0f, 0.0f, 0.5f);
                        }

                        else if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                            target_thickness = 2.0f;
                        }

                        // User Input
                        static int initial_time { 0 };

                        if (ImGui::IsItemActivated()) {
                            initial_time = event.time;
                            state.selection = &event;
                        }

                        if (!ImGui::GetIO().KeyAlt && ImGui::IsItemActive()) {
                            float delta = ImGui::GetMouseDragDelta().x;
                            event.time = initial_time + px_to_time(delta);
                            event.removed = (
                                event.time > state.range.y() ||
                                event.time + event.length < state.range.x()
                            );

                            event.enabled = !event.removed;
                            target_height = 5.0f;
                        }

                        const float height_delta = target_height - event.height;
                        transition(event.height, target_height, GlobalTheme.transition_speed);
                        // transition(event.thickness, target_height, GlobalTheme.transition_speed);

                        pos -= event.height;

                        const int shadow = 2;
                        painter->AddRectFilled(
                            cursor + pos + shadow + event.height * 1.25f,
                            cursor + pos + size + shadow + event.height * 1.25f,
                            ImColor::HSV(0.0f, 0.0f, 0.0f, 0.3f), EditorTheme.radius
                        );

                        painter->AddRectFilled(
                            cursor + pos,
                            cursor + pos + size,
                            ImColor(color), EditorTheme.radius
                        );

                        if (ImGui::IsItemHovered() || ImGui::IsItemActive() || state.selection == &event) {
                            painter->AddRect(
                                cursor + pos        + event.thickness * 0.25f,
                                cursor + pos + size - event.thickness * 0.25f,
                                ImColor(EditorTheme.selection), EditorTheme.radius, ImDrawCornerFlags_All, event.thickness
                            );
                        }
                        else {
                            painter->AddRect(
                                cursor + pos        + event.thickness,
                                cursor + pos + size - event.thickness,
                                ImColor(EditorTheme.outline), EditorTheme.radius
                            );
                        }

                        if (event.enabled) {
                            if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                                if (event.length > 10.0f) {
                                    painter->AddText(
                                        cursor + pos + ImVec2{ 3.0f + event.thickness, 0.0f },
                                        ImColor(EditorTheme.text),
                                        std::to_string(event.time).c_str()
                                    );
                                }
                                
                                if (event.length > 30.0f) {
                                    painter->AddText(
                                        cursor + pos + ImVec2{ size.x - 20.0f, 0.0f },
                                        ImColor(EditorTheme.text),
                                        std::to_string(event.length).c_str()
                                    );
                                }
                            }
                        }

                        event_count++;
                    }

                    // Next event type
                    cursor.y += state.zoom[1];
                }

                // Next track
                cursor.y += padding.y;
            });
        };

        auto Range = [&]() {
            ImVec2 cursor { C.x, C.y };
            ImVec2 range_cursor_start { state.range.x() * zoom_ / stride_ + state.pan[0], TimelineTheme.height };
            ImVec2 range_cursor_end { state.range.y() * zoom_ / stride_ + state.pan[0], TimelineTheme.height };

            painter->AddRectFilled(
                cursor + ImVec2{ 0.0f, 0.0f },
                cursor + ImVec2{ range_cursor_start.x, windowSize.y },
                ImColor(0.0f, 0.0f, 0.0f, 0.3f)
            );

            painter->AddRectFilled(
                cursor + ImVec2{ range_cursor_end.x, 0.0f },
                cursor + ImVec2{ windowSize.x, windowSize.y },
                ImColor(0.0f, 0.0f, 0.0f, 0.3f)
            );
        };

        /**
         * @brief Outliner-style listing of available channels
         *
         *  ______________
         * |              |
         * | chan A       |
         * |  |-o type 1  |
         * |  |-o type 2  |
         * | chan B       |
         * |  |-o type 1  |
         * | chan C       |
         * |  |-o type 1  |
         * |  |-o type 2  |
         * |  |-o type 3  |
         * | chan D       |
         * | ...          |
         * |              |
         * |______________|
         *
         *
         */
        auto Lister = [&]() {
            auto cursor = ImVec2{ A.x, A.y + state.pan[1] };
            const auto padding = 5.0f;

            _registry.view<Index, Track>().each<Index>([&](const auto&, auto& track) {

                // Draw channel header
                //  _________________________________________
                // |_________________________________________|
                //
                const auto textSize = ImGui::CalcTextSize(track.label);
                const auto pos = ImVec2{
                    ListerTheme.width - textSize.x - padding,
                    GlobalTheme.track_height / 2.0f - textSize.y / 2.0f
                };

                painter->AddRectFilled(
                    cursor,
                    cursor + ImVec2{ ListerTheme.width, GlobalTheme.track_height },
                    ImColor(ListerTheme.alternate)
                );

                painter->AddText(
                    cursor + pos, ImColor(ListerTheme.text), track.label
                );

                cursor.y += GlobalTheme.track_height;

                // Draw children
                //
                // |
                // |-- child
                // |-- child
                // |__ child
                //
                for (auto& [type, channel] : track.channels) {
                    const auto textSize = ImGui::CalcTextSize(channel.label) * 0.85f;
                    const auto pos = ImVec2{
                        ListerTheme.width - textSize.x - padding,
                        state.zoom[1] / 2.0f - textSize.y / 2.0f
                    };

                    painter->AddText(
                        ImGui::GetFont(),
                        textSize.y,
                        cursor + pos,
                        ImColor(ListerTheme.text), channel.label
                    );

                    cursor.y += state.zoom[1];
                }
            });
        };

        /**
         * Draw
         *
         */
        EditorBackground();
        VerticalGrid();
        Events();
        TimelineBackground();
        Timeline();
        Range();
        TimeIndicator(state.range.x(), TimelineTheme.start_time, EditorTheme.start_time);
        
        // Can intercept mouse events
        bool hovering_background { true };
        if (ImGui::IsItemHovered()) hovering_background = false;
        TimeIndicator(state.range.y(), TimelineTheme.end_time, EditorTheme.end_time);
        if (ImGui::IsItemHovered()) hovering_background = false;
        TimeIndicator(state.current_time, TimelineTheme.current_time, EditorTheme.current_time);
        if (ImGui::IsItemHovered()) hovering_background = false;

        ListerBackground();
        Lister();
        CrossBackground();

        /**
         * User Input
         *
         */
        if (hovering_background) {

            /** Pan
             *
             *        ^
             *        |
             *        |
             *  <-----|----->
             *        |
             *        |
             *        v
             */
            ImGui::SetCursorPos({ 0.0f, titlebarHeight });
            ImGui::InvisibleButton("##mpan", ImVec2{ ListerTheme.width, TimelineTheme.height });

            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            const bool panM = (
                ImGui::IsItemActive() ||
                (ImGui::IsWindowFocused() && ImGui::GetIO().KeyAlt && ImGui::GetIO().MouseDown[0])
            );

            /** Vertical Pan
             *
             *
             *
             *  <--------------->
             *
             *
             */
            ImGui::SetCursorPos({ ListerTheme.width, titlebarHeight });
            ImGui::InvisibleButton("##pan[0]", ImVec2{ windowSize.x, TimelineTheme.height });

            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            const bool panH = ImGui::IsItemActive();

            /** Vertical Pan
             *
             *        ^
             *        |
             *        |
             *        |
             *        v
             */
            ImGui::SetCursorPos({ 0.0f, TimelineTheme.height + titlebarHeight });
            ImGui::InvisibleButton("##pan[1]", ImVec2{ ListerTheme.width, windowSize.y });

            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            const bool panV = ImGui::IsItemActive();

            if (panM) {
                state.target_pan[0] += ImGui::GetIO().MouseDelta.x;
                state.target_pan[1] += ImGui::GetIO().MouseDelta.y;
            }
            else if (panV) state.target_pan[1] += ImGui::GetIO().MouseDelta.y;
            else if (panH) state.target_pan[0] += ImGui::GetIO().MouseDelta.x;
        }
    }
    ImGui::End();
}

}