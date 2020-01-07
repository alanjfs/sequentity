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

    bool removed { false };

    /* Extend or reduce the length of an event */
    float scale { 1.0f };

    ImVec4 color { ImColor::HSV(0.0f, 0.0f, 1.0f) };

    // Map your custom data here, along with an optional type
    void* data { nullptr };
    EventType type { EventType_Move };
};

/**
 * @brief A collection of events
 *
 */
struct Channel {
    const char* label { "Untitled" };
    ImVec4 color { ImColor::HSV(0.33f, 0.5f, 1.0f) };
    std::vector<Event> events;
};

/**
 * @brief A collection of channels
 *
 */
using Track = std::unordered_map<EventType, Channel>;


/**
 * @brief Callbacks
 *
 */
using OverlappingCallback = std::function<void(const Event&)>;
using TimeChangedCallback = std::function<void()>;


/**
 * @brief All of Sequentities (mutable) state, accessible from your application
 *
 */
struct State {

    // Functional
    bool playing { false };
    int currentTime { 0 };
    Vector2i range { 0, 100 };

    // Visual
    float zoom[2] { 200.0f, 30.0f };
    float scroll[2] { 8.0f, 8.0f };
    int stride { 3 };

    // Transitions
    float target_zoom[2] { 200.0f, 30.0f };
    float target_scroll[2] { 8.0f, 8.0f };
};

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
    void set_current_time(int time);
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
    int _previousTime { 0 };

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
    ImVec4 borderInner  { ImColor::HSV(0.0f, 0.0f, 0.4f) };
    ImVec4 borderOuter  { ImColor::HSV(0.0f, 0.0f, 0.3f) };

    // Shadows are overlayed, for a gradient-like effect
    ImVec4 shadow  { ImColor::HSV(0.0f, 0.0f, 0.0f, 0.1f) };

    float borderWidth { 2.0f };
    float trackHeight { 25.0f };

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
    ImVec4 background  { ImColor::HSV(0.0f, 0.0f, 0.250f) };
    ImVec4 text        { ImColor::HSV(0.0f, 0.0f, 0.850f) };
    ImVec4 dark        { ImColor::HSV(0.0f, 0.0f, 0.322f) };
    ImVec4 mid         { ImColor::HSV(0.0f, 0.0f, 0.314f) };
    ImVec4 startTime   { ImColor::HSV(0.33f, 0.0f, 0.50f) };
    ImVec4 currentTime { ImColor::HSV(0.57f, 0.62f, 0.99f) };
    ImVec4 endTime     { ImColor::HSV(0.0f, 0.0f, 0.40f) };

    float height { 40.0f };

} TimelineTheme;


static struct EditorTheme_ {
    ImVec4 background  { ImColor::HSV(0.0f, 0.00f, 0.651f) };
    ImVec4 alternate   { ImColor::HSV(0.0f, 0.00f, 0.0f, 0.02f) };
    ImVec4 text        { ImColor::HSV(0.0f, 0.00f, 0.950f) };
    ImVec4 mid         { ImColor::HSV(0.0f, 0.00f, 0.600f) };
    ImVec4 dark        { ImColor::HSV(0.0f, 0.00f, 0.498f) };
    ImVec4 accent      { ImColor::HSV(0.0f, 0.75f, 0.750f) };
    ImVec4 accentLight { ImColor::HSV(0.0f, 0.5f, 1.0f, 0.1f) };
    ImVec4 accentDark  { ImColor::HSV(0.0f, 0.0f, 0.0f, 0.1f) };
    ImVec4 selection   { ImColor::HSV(0.15f, 0.7f, 1.0f) };
    ImVec4 outline     { ImColor::HSV(0.0f, 0.0f, 0.1f) };
    ImVec4 track       { ImColor::HSV(0.0f, 0.0f, 0.6f) };

    ImVec4 startTime   { ImColor::HSV(0.33f, 0.0f, 0.25f) };
    ImVec4 currentTime { ImColor::HSV(0.6f, 0.5f, 0.5f) };
    ImVec4 endTime     { ImColor::HSV(0.0f, 0.0f, 0.25f) };
    
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
    Registry.reset<Track>();
}

void Sequentity::play() {
    _registry.ctx<State>().playing ^= true;
}


void Sequentity::set_current_time(int time) {
    auto& state = _registry.ctx<State>();
    auto& range = _registry.ctx<State>().range;

    // Prevent callbacks from being called unnecessarily
    if (time == _previousTime) return;

    _before_time_changed();
    _previousTime = time;
    state.currentTime = time;
    _after_time_changed();
}


void Sequentity::stop() {
    auto& state = _registry.ctx<State>();

    set_current_time(state.range.x());
    state.playing = false;
}


void Sequentity::update() {
    auto& state = _registry.ctx<State>();
    if (state.playing) step(1);
}


void Sequentity::step(int delta) {
    auto& state = _registry.ctx<State>();
    auto time = state.currentTime + delta;

    if (time > state.range.y()) {
        time = state.range.x();
    }

    else if (time < state.range.x()) {
        time = state.range.y();
    }

    set_current_time(time);
}


bool contains(const Event& event, int time) {
    return (
        event.time <= time &&
        event.time + event.length > time
    );
}

const Event* Sequentity::overlapping(const Track& track) {
    const Event* intersecting { nullptr };

    for (auto& [type, channel] : track) {
        for (auto& event : channel.events) {
            if (contains(event, _registry.ctx<State>().currentTime)) {
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
    for (auto& [type, channel] : track) {
        for (auto& event : channel.events) {
            if (contains(event, _registry.ctx<State>().currentTime)) func(event);
        }
    }
}


void Sequentity::drawThemeEditor(bool* p_open) {
    ImGui::Begin("Theme", p_open);
    {
        if (ImGui::CollapsingHeader("Timeline")) {
            ImGui::ColorEdit4("background", &TimelineTheme.background.x);
            ImGui::ColorEdit4("text", &TimelineTheme.text.x);
            ImGui::ColorEdit4("dark", &TimelineTheme.dark.x);
            ImGui::ColorEdit4("mid", &TimelineTheme.mid.x);
            ImGui::ColorEdit4("startTime", &TimelineTheme.startTime.x);
            ImGui::ColorEdit4("currentTime", &TimelineTheme.currentTime.x);
            ImGui::ColorEdit4("endTime", &TimelineTheme.endTime.x);
            ImGui::DragFloat("height", &TimelineTheme.height);
        }

        if (ImGui::CollapsingHeader("Editor")) {
            ImGui::ColorEdit4("background##editor", &EditorTheme.background.x);
            ImGui::ColorEdit4("alternate##editor", &EditorTheme.alternate.x);
            ImGui::ColorEdit4("text##editor", &EditorTheme.text.x);
            ImGui::ColorEdit4("mid##editor", &EditorTheme.mid.x);
            ImGui::ColorEdit4("dark##editor", &EditorTheme.dark.x);
            ImGui::ColorEdit4("accent##editor", &EditorTheme.accent.x);

            ImGui::ColorEdit4("startTime##editor", &EditorTheme.startTime.x);
            ImGui::ColorEdit4("currentTime##editor", &EditorTheme.currentTime.x);
            ImGui::ColorEdit4("endTime##editor", &EditorTheme.endTime.x);
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

    auto& state = _registry.ctx<State>();
    auto speed = 0.25f;
    state.scroll[0] += (state.target_scroll[0] - state.scroll[0]) * speed;
    state.scroll[1] += (state.target_scroll[1] - state.scroll[1]) * speed;
    state.zoom[0] += (state.target_zoom[0] - state.zoom[0]) * speed;
    state.zoom[1] += (state.target_zoom[1] - state.zoom[1]) * speed;

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

        auto drawCrossBackground = [&]() {
            painter->AddRectFilled(
                X,
                X + ImVec2{ ListerTheme.width + 1, TimelineTheme.height },
                ImColor(ListerTheme.background)
            );

            // Border
            painter->AddLine(X + ImVec2{ ListerTheme.width, 0.0f },
                             X + ImVec2{ ListerTheme.width, TimelineTheme.height },
                             ImColor(GlobalTheme.dark),
                             GlobalTheme.borderWidth);

            painter->AddLine(X + ImVec2{ 0.0f, TimelineTheme.height },
                             X + ImVec2{ ListerTheme.width + 1, TimelineTheme.height },
                             ImColor(GlobalTheme.dark),
                             GlobalTheme.borderWidth);
        };

        auto drawListerBackground = [&]() {
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
                             GlobalTheme.borderWidth);
        };

        auto drawTimelineBackground = [&]() {
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
                             GlobalTheme.borderWidth);
        };

        auto drawEditorBackground = [&]() {
            painter->AddRectFilled(
                C,
                C + windowPos + windowSize,
                ImColor(EditorTheme.background)
            );
        };

        auto drawTimeline = [&]() {
            for (int time = minTime; time < maxTime + 1; time++) {
                float xMin = time * zoom_;
                float xMax = 0.0f;
                float yMin = 0.0f;
                float yMax = TimelineTheme.height - 1;

                xMin += B.x + state.scroll[0];
                xMax += B.x + state.scroll[0];
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

        auto drawVerticalGrid = [&]() {
            for (int time = minTime; time < maxTime + 1; time++) {
                float xMin = time * zoom_;
                float xMax = 0.0f;
                float yMin = 0.0f;
                float yMax = windowSize.y;

                xMin += C.x + state.scroll[0];
                xMax += C.x + state.scroll[0];
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
         * @brief Visualise current time
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
        auto drawTimeIndicator = [&](int& time, ImVec4 cursor_color, ImVec4 line_color) -> bool {
            auto xMin = time * zoom_ / stride_;
            auto xMax = 0.0f;
            auto yMin = TimelineTheme.height;
            auto yMax = windowSize.y;

            xMin += B.x + state.scroll[0];
            xMax += B.x + state.scroll[0];
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
                             ImColor(EditorTheme.accentDark));
            painter->AddLine(topPos - ImVec2{ -2.0f, size.y * 0.3f },
                             topPos - ImVec2{ -2.0f, size.y * 0.8f },
                             ImColor(EditorTheme.accentDark));

            indicator_count++;

            return false;
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
        auto drawHorizontalGrid = [&]() {
            float yMin = 0.0f;
            float yMax = windowSize.y;
            float xMin = 0.0f;
            float xMax = windowSize.x;

            xMin += A.x;
            xMax += A.x;
            yMin += A.y + state.scroll[1];
            yMax += A.y + state.scroll[1];

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
        auto drawEvents = [&]() {
            ImVec2 cursor { C.x + state.scroll[0], C.y + state.scroll[1] };

            _registry.view<Index, Name, Track>().each<Index>([&](auto entity, const auto, const auto& name, auto& track) {

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
                static ImVec2 center { 0.0f, GlobalTheme.trackHeight / 2.0f };

                ImVec2 size { windowSize.x, GlobalTheme.trackHeight };

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
                for (auto& [type, channel] : track) {
                    for (auto& event : channel.events) {
                        ImVec2 pos { time_to_px(event.time), 0.0f };
                        ImVec2 size { time_to_px(event.length), state.zoom[1] };

                        std::string label { "##event" };
                        label += name.text;
                        label += std::to_string(event_count);
                        ImGui::SetCursorPos(cursor + pos - ImGui::GetWindowPos());
                        ImGui::InvisibleButton(label.c_str(), size);

                        float thickness = 0.0f;
                        ImVec4 color = event.color;

                        if (!event.enabled) {
                            color = ImColor::HSV(0.0f, 0.0f, 0.5f);
                        }

                        else if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                            thickness = 3.0f;
                        }

                        // User Input
                        static int initialTime { 0 };
                        if (ImGui::IsItemActivated()) initialTime = event.time;
                        if (!ImGui::GetIO().KeyAlt && ImGui::IsItemActive()) {
                            float delta = ImGui::GetMouseDragDelta().x;
                            event.time = initialTime + px_to_time(delta);
                            event.removed = (
                                event.time > state.range.y() ||
                                event.time + event.length < state.range.x()
                            );

                            event.enabled = !event.removed;
                        }

                        const int shadow = 2;
                        painter->AddRectFilled(
                            cursor + pos + shadow,
                            cursor + pos + size + shadow,
                            ImColor::HSV(0.0f, 0.0f, 0.0f, 0.3f), EditorTheme.radius
                        );

                        painter->AddRectFilled(
                            cursor + pos,
                            cursor + pos + size,
                            ImColor(color), EditorTheme.radius
                        );

                        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                            painter->AddRect(
                                cursor + pos        + thickness * 0.25f,
                                cursor + pos + size - thickness * 0.25f,
                                ImColor(EditorTheme.selection), EditorTheme.radius, ImDrawCornerFlags_All, thickness
                            );
                        }
                        else {
                            painter->AddRect(
                                cursor + pos        + thickness,
                                cursor + pos + size - thickness,
                                ImColor(EditorTheme.outline), EditorTheme.radius
                            );
                        }

                        if (event.enabled) {
                            if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                                if (event.length > 10.0f) {
                                    painter->AddText(
                                        cursor + pos + ImVec2{ 3.0f, 0.0f },
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

        auto drawRange = [&]() {
            ImVec2 cursor { C.x, C.y };
            ImVec2 range_cursor_start { state.range.x() * zoom_ / stride_ + state.scroll[0], TimelineTheme.height };
            ImVec2 range_cursor_end { state.range.y() * zoom_ / stride_ + state.scroll[0], TimelineTheme.height };

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
        auto drawLister = [&]() {
            auto cursor = ImVec2{ A.x, A.y + state.scroll[1] };
            const auto padding = 5.0f;

            _registry.view<Index, Name, Track>().each<Index>([&](const auto&, const auto& name, auto& track) {

                // Draw channel header
                //  _________________________________________
                // |_________________________________________|
                //
                const auto textSize = ImGui::CalcTextSize(name.text);
                const auto pos = ImVec2{
                    ListerTheme.width - textSize.x - padding,
                    GlobalTheme.trackHeight / 2.0f - textSize.y / 2.0f
                };

                painter->AddRectFilled(
                    cursor,
                    cursor + ImVec2{ ListerTheme.width, GlobalTheme.trackHeight },
                    ImColor(ListerTheme.alternate)
                );

                painter->AddText(
                    cursor + pos, ImColor(ListerTheme.text), name.text
                );

                cursor.y += GlobalTheme.trackHeight;

                // Draw children
                //
                // |
                // |-- child
                // |-- child
                // |__ child
                //
                for (auto& [type, channel] : track) {
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

        bool hovering_background { true };
        painter->PushClipRect(B + ImVec2{ 1.0f, 0.0f }, ImGui::GetWindowPos() + ImGui::GetWindowSize());
        {
            drawEditorBackground();
            drawVerticalGrid();
            drawEvents();
            drawTimelineBackground();
            drawTimeline();
            drawRange();
            drawTimeIndicator(state.range.x(), TimelineTheme.startTime, EditorTheme.startTime);
            if (ImGui::IsItemHovered()) hovering_background = false;
            drawTimeIndicator(state.range.y(), TimelineTheme.endTime, EditorTheme.endTime);
            if (ImGui::IsItemHovered()) hovering_background = false;
            drawTimeIndicator(state.currentTime, TimelineTheme.currentTime, EditorTheme.currentTime);
            if (ImGui::IsItemHovered()) hovering_background = false;
        }
        painter->PopClipRect();

        drawListerBackground();
        drawLister();
        drawCrossBackground();

        //
        // User Input
        //
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
            ImGui::InvisibleButton("##mscroll", ImVec2{ ListerTheme.width, TimelineTheme.height });

            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            const bool scrollM = (
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
            ImGui::InvisibleButton("##scroll[0]", ImVec2{ windowSize.x, TimelineTheme.height });

            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            const bool scrollH = ImGui::IsItemActive();

            /** Vertical Pan
             *
             *        ^
             *        |
             *        |
             *        |
             *        v
             */
            ImGui::SetCursorPos({ 0.0f, TimelineTheme.height + titlebarHeight });
            ImGui::InvisibleButton("##scroll[1]", ImVec2{ ListerTheme.width, windowSize.y });

            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            const bool scrollV = ImGui::IsItemActive();

            if (scrollM) {
                state.target_scroll[0] += ImGui::GetIO().MouseDelta.x;
                state.target_scroll[1] += ImGui::GetIO().MouseDelta.y;
            }
            else if (scrollV) state.target_scroll[1] += ImGui::GetIO().MouseDelta.y;
            else if (scrollH) state.target_scroll[0] += ImGui::GetIO().MouseDelta.x;
        }
    }
    ImGui::End();
}

}