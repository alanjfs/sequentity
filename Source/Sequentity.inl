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

    /* Events are never really deleted, just hidden from view and iterators */
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

    bool solo { false };
    bool mute { false };

    std::unordered_map<EventType, Channel> channels;
    
    // Internal
    bool _notsoloed { false };
};


/**
 * Tag indicating whether a track is selected
 *
 * This can come in handy if you implement selection in your application
 * and wish to synchronise the currently selected track with its associated entity
 *
 */
struct Selected {};

/**
 * @brief Lambda signatures
 *
 */
using IntersectFunc = std::function<void(const Event&)>;
using IntersectWithPreviousFunc = std::function<void(const Event&, const Event&)>;
using IntersectWithPreviousAndNextFunc = std::function<void(const Event&, const Event&, const Event&)>;


/**
 * @brief All of Sequentities (mutable) state
 *
 * Accessible from your application via registry.ctx<Sequentity::State>();
 *
 */
struct State {

    // Functional
    int current_time { 0 };
    int range[2] { 0, 100 };

    // Selection
    Event*   selected_event { nullptr };
    Track*   selected_track { nullptr };
    Channel* selected_channel { nullptr };

    // Visual
    float zoom[2] { 250.0f, 20.0f };
    float pan[2] { 8.0f, 8.0f };
    int stride { 2 };

    // Transitions
    float target_zoom[2] { 200.0f, 20.0f };
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
 * @brief Stateless class
 *
 */
struct Sequentity {

    // Sequentity reads Event and Channel components from your EnTT registry
    // but the involvement of EnTT is minimal and easily replaced by your own
    // implementation. See any reference to _registry below.
    Sequentity(entt::registry& registry) : _registry(registry) {}

    void draw(bool* p_open = nullptr);

private:
    entt::registry& _registry;
};


/**
 * @brief A summary of all colours available to Sequentity
 *
 * Hint: These can be edited interactively via the `draw_theme_editor()` window
 *
 */

static struct GlobalTheme_ {
    ImVec4 dark         { ImColor::HSV(0.0f, 0.0f, 0.3f) };
    ImVec4 shadow       { ImColor::HSV(0.0f, 0.0f, 0.0f, 0.1f) };

    bool bling { true };  // Add some unnecessary but pretty flare, like shadows. Disable for performance
    float border_width { 1.0f };
    float track_height { 25.0f };
    float transition_speed { 0.2f };

} GlobalTheme;


static struct ListerTheme_ {
    ImVec4 background  { ImColor::HSV(0.0f, 0.0f, 0.188f) };
    ImVec4 alternate   { ImColor::HSV(0.0f, 0.0f, 1.0f, 0.02f) };
    ImVec4 text        { ImColor::HSV(0.0f, 0.0f, 0.850f) };
    ImVec4 dark        { ImColor::HSV(0.0f, 0.0f, 0.100f) };
    ImVec4 mid         { ImColor::HSV(0.0f, 0.0f, 0.314f) };
    ImVec4 accent      { ImColor::HSV(0.0f, 0.75f, 0.750f) };
    ImVec4 outline     { ImColor::HSV(0.0f, 0.0f, 0.1f) };

    float width { 180.0f };

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
    float spacing { 1.0f };

} EditorTheme;



/**
 * @brief Internal helper functions
 *
 */
static void _solo(entt::registry& registry, Track& track) {
    bool any_solo { false };
    registry.view<Track>().each([&any_solo](auto& track) {
        if (track.solo) any_solo = true;
        track._notsoloed = false;
    });

    if (any_solo) registry.view<Track>().each([](auto& track) {
        track._notsoloed = !track.solo;
    });
}


static bool _contains(const Event& event, int time) {
    return (
        event.time <= time &&
        event.time + event.length > time
    );
}


/**  Find intersecting events in track @ time
 *
 *               time
 *                 |
 *    _____________|__________   ______
 *   |_____________|__________| |______
 *          _______|__________       __
 *         |_______|__________|     |__
 *    _____________|___
 *   |_____________|___|
 *   ^             |
 * event           |
 *
 */
void Intersect(const Track& track, int time, IntersectFunc func) {
    for (auto& [type, channel] : track.channels) {
        if (track.mute) continue;
        if (track._notsoloed) continue;

        for (auto& event : channel.events) {
            if (event.removed) continue;
            if (!event.enabled) continue;
            if (_contains(event, time)) func(event);
        }
    }
}


void ThemeEditor(bool* p_open = nullptr) {
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

    auto& state = _registry.ctx_or_set<State>();
    transition(state.pan[0], state.target_pan[0], GlobalTheme.transition_speed, 1.0f);
    transition(state.pan[1], state.target_pan[1], GlobalTheme.transition_speed, 1.0f);
    transition(state.zoom[0], state.target_zoom[0], GlobalTheme.transition_speed);
    transition(state.zoom[1], state.target_zoom[1], GlobalTheme.transition_speed);

    ImGui::Begin("Editor", p_open, windowFlags);
    {
        auto* painter = ImGui::GetWindowDrawList();
        auto titlebarHeight = 24.0f; // TODO: Find an exact value for this
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const ImVec2 windowPos = ImGui::GetWindowPos() + ImVec2{ 0.0f, titlebarHeight };
        const ImVec2 padding { 7.0f, 2.0f };

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
        int minTime = state.range[0] / stride_;
        int maxTime = state.range[1] / stride_;

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
            if (GlobalTheme.bling) {
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
            }

            // Fill
            painter->AddRectFilled(
                A,
                A + ImVec2{ ListerTheme.width, windowSize.y },
                ImColor(ImGui::GetStyleColorVec4(ImGuiCol_TitleBg))
            );

            // Border
            painter->AddLine(A + ImVec2{ ListerTheme.width, 0.0f },
                             A + ImVec2{ ListerTheme.width, windowSize.y },
                             ImColor(GlobalTheme.dark),
                             GlobalTheme.border_width);
        };

        auto TimelineBackground = [&]() {
            if (GlobalTheme.bling) {
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
            }

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

            ImGui::PushID(indicator_count);
            ImGui::SetCursorPos(topPos - ImVec2{ size.x, size.y } - ImGui::GetWindowPos());
            ImGui::SetItemAllowOverlap(); // Prioritise the last drawn (shouldn't this be the default?)
            ImGui::InvisibleButton("##indicator", size * 2.0f);
            ImGui::PopID();

            static unsigned int initial_time { 0 };
            if (ImGui::IsItemActivated()) {
                initial_time = time;
            }

            ImVec4 color = cursor_color;
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
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
        auto Header = [&](const Track& track, ImVec2& cursor) {
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
            const ImVec2 size { windowSize.x, GlobalTheme.track_height };

            // Hide underlying vertical lines
            painter->AddRectFilled(
                ImVec2{ C.x, cursor.y },
                ImVec2{ C.x, cursor.y } + size,
                ImColor(EditorTheme.background)
            );

            // Tint empty area with the track color
            painter->AddRectFilled(
                ImVec2{ C.x, cursor.y },
                ImVec2{ C.x, cursor.y } + size,
                ImColor(track.color.x, track.color.y, track.color.z, 0.1f)
            );

            painter->AddRect(
                ImVec2{ C.x, cursor.y },
                ImVec2{ C.x, cursor.y } + size,
                ImColor(EditorTheme.mid)
            );

            cursor.y += size.y;
        };

        auto Events = [&]() {
            ImVec2 cursor { C.x + state.pan[0], C.y + state.pan[1] };

            _registry.view<Track>().each([&](auto& track) {
                Header(track, cursor);

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

                        ImVec2 pos { time_to_px(event.time), 0.0f };
                        ImVec2 size { time_to_px(event.length), state.zoom[1] };
                        ImVec2 head_size { 10.0f, size.y };
                        ImVec2 tail_size { 10.0f, size.y };

                        // Transitions
                        float target_height { 0.0f };
                        float target_thickness = 0.0f;

                        /** TODO: Implement crop interaction and visualisation
                             ____________________________________
                            |      |                      |      |
                            | head |         body         | tail |
                            |______|______________________|______|

                        */

                        ImGui::PushID(track.label);
                        ImGui::PushID(event_count);
                        ImGui::SetCursorPos(cursor + pos - ImGui::GetWindowPos());
                        ImGui::SetItemAllowOverlap();
                        ImGui::InvisibleButton("##event", size);
                        ImGui::PopID();
                        ImGui::PopID();

                        ImVec4 color = channel.color;

                        if (!event.enabled || track.mute || track._notsoloed) {
                            color = ImColor::HSV(0.0f, 0.0f, 0.5f);
                        }

                        else if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                            target_thickness = 2.0f;
                        }

                        // User Input
                        static int initial_time { 0 };

                        if (ImGui::IsItemActivated()) {
                            initial_time = event.time;
                            state.selected_event = &event;
                        }

                        if (!ImGui::GetIO().KeyAlt && ImGui::IsItemActive()) {
                            float delta = ImGui::GetMouseDragDelta().x;
                            event.time = initial_time + px_to_time(delta);
                            event.removed = (
                                event.time > state.range[1] ||
                                event.time + event.length < state.range[0]
                            );

                            event.enabled = !event.removed;
                            target_height = 5.0f;
                        }

                        transition(event.height, target_height, GlobalTheme.transition_speed);
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

                        // Add a dash to the bottom of each event.
                        painter->AddRectFilled(
                            cursor + pos + ImVec2{ 0.0f, size.y - 5.0f },
                            cursor + pos + size,
                            ImColor(color * 0.8f), EditorTheme.radius
                        );

                        if (ImGui::IsItemHovered() || ImGui::IsItemActive() || state.selected_event == &event) {
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
                                if (event.length > 5.0f) {
                                    painter->AddText(
                                        ImGui::GetFont(),
                                        ImGui::GetFontSize() * 0.85f,
                                        cursor + pos + ImVec2{ 3.0f + event.thickness, 0.0f },
                                        ImColor(EditorTheme.text),
                                        std::to_string(event.time).c_str()
                                    );
                                }
                                
                                if (event.length > 30.0f) {
                                    painter->AddText(
                                        ImGui::GetFont(),
                                        ImGui::GetFontSize() * 0.85f,
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
                    cursor.y += state.zoom[1] + EditorTheme.spacing;
                }

                // Next track
                cursor.y += padding.y;
            });
        };

        auto Range = [&]() {
            ImVec2 cursor { C.x, C.y };
            ImVec2 range_cursor_start { state.range[0] * zoom_ / stride_ + state.pan[0], TimelineTheme.height };
            ImVec2 range_cursor_end { state.range[1] * zoom_ / stride_ + state.pan[0], TimelineTheme.height };

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

        auto Button = [](const char* label, bool& checked, ImVec2 size = { 20.0f, 20.0f }) -> bool {
            if (checked) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0.25f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0.15f));
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0.1f));
            }

            const bool pressed = ImGui::Button(label, size);

            if (checked) ImGui::PopStyleColor(2);
            else         ImGui::PopStyleColor(1);

            if (pressed) checked ^= true;

            return pressed;
        };

        /**
         * @brief Outliner-style listing of available channels
         *
         *  ______________________________
         * |  _  _  _                     |
         * | |_||_||_|       track A      |
         * |                  |-o chan 1  |
         * |  _  _  _         |-o chan 2  |
         * | |_||_||_|       track B      |
         * |  _  _  _         |-o chan 1  |
         * | |_||_||_|       track C      |
         * |                  |-o chan 1  |
         * |                  |-o chan 2  |
         * |  _  _  _         |-o chan 3  |
         * | |_||_||_|       track D      |
         * |                 ...          |
         * |                              |
         * |______________________________|
         *
         *
         */
        auto Lister = [&]() {
            auto cursor = ImVec2{ A.x, A.y + state.pan[1] };

            _registry.view<Track>().each([&](auto& track) {

                // Draw track header
                //  __________________________________________
                // |  ______ ______                         | |
                // | | Mute | Solo |           Track        | |
                // |________________________________________|_|
                //

                const auto textSize = ImGui::CalcTextSize(track.label);
                const auto pos = ImVec2{
                    ListerTheme.width - textSize.x - padding.x - padding.x,
                    GlobalTheme.track_height / 2.0f - textSize.y / 2.0f
                };

                painter->AddRectFilled(
                    cursor + ImVec2{ ListerTheme.width - 5.0f, 0.0f },
                    cursor + ImVec2{ ListerTheme.width, GlobalTheme.track_height },
                    ImColor(track.color)
                );

                painter->AddText(
                    cursor + pos, ImColor(ListerTheme.text), track.label
                );

                ImGui::SetCursorPos(cursor + ImVec2{ padding.x, 0.0f } - ImGui::GetWindowPos());
                ImGui::PushID(track.label);
                Button("m", track.mute, { GlobalTheme.track_height, GlobalTheme.track_height });
                ImGui::SameLine();

                if (Button("s", track.solo, { GlobalTheme.track_height, GlobalTheme.track_height })) {
                    _solo(_registry, track);
                }

                ImGui::PopID();

                const auto track_corner = cursor;
                cursor.y += GlobalTheme.track_height;

                // Draw channels
                // |
                // |-- channel
                // |-- channel
                // |__ channel
                //
                for (auto& [type, channel] : track.channels) {
                    static const ImVec2 indicator_size { 9.0f, 9.0f };
                    const ImVec2 indicator_pos = {
                        ListerTheme.width - indicator_size.x - padding.x,
                        state.zoom[1] * 0.5f - indicator_size.y * 0.5f
                    };

                    painter->AddRectFilled(
                        cursor + indicator_pos,
                        cursor + indicator_pos + indicator_size,
                        ImColor(channel.color)
                    );

                    painter->AddRect(
                        cursor + indicator_pos,
                        cursor + indicator_pos + indicator_size,
                        ImColor(channel.color * 1.25f)
                    );

                    const auto textSize = ImGui::CalcTextSize(channel.label) * 0.85f;
                    const auto pos = ImVec2{
                        ListerTheme.width - textSize.x - padding.x - indicator_size.x - padding.x,
                        state.zoom[1] * 0.5f - textSize.y * 0.5f
                    };

                    painter->AddText(
                        ImGui::GetFont(),
                        textSize.y,
                        cursor + pos,
                        ImColor(ListerTheme.text), channel.label
                    );

                    // Next channel
                    cursor.y += state.zoom[1] + EditorTheme.spacing;
                }

                // Next track
                cursor.y += padding.y;

                // Visualise mute and solo state
                if (track.mute || track._notsoloed) {
                    ImVec4 faded = ListerTheme.background;
                    faded.w = 0.8f;

                    painter->AddRectFilled(
                        track_corner + ImVec2{ pos.x, 0.0f },
                        track_corner + ImVec2{ ListerTheme.width, cursor.y },
                        ImColor(faded)
                    );
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

        // Can intercept mouse events
        bool hovering_background { true };
        TimeIndicator(state.range[0], TimelineTheme.start_time, EditorTheme.start_time);
        if (ImGui::IsItemHovered()) hovering_background = false;
        TimeIndicator(state.range[1], TimelineTheme.end_time, EditorTheme.end_time);
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
            ImGui::SetCursorPos({ ListerTheme.width - 110.0f, TimelineTheme.height + titlebarHeight });
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