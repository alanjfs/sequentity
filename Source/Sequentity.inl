/**

---===  Sequentity  ===---

An immediate-mode Sequentity, written in C++ with ImGui, Magnum and EnTT

---=== === === === === ---

- ~2,000 vertices with 15 events, about 1,500 without any
- Magnum is an open-source OpenGL wrapper with window management and math library baked in
- EnTT is a entity-component-system (ECS) framework

Both of which are optional and unrelated to the sequencer itself, but needed
to get a window on screen and to manage relevant data.


### Legend

- Event:   An individual coloured bar, with a start, length and additional metadata
- Channel: A vector of Events


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
    int time;
    int length;

    // Map your custom data here, along with an optional type
    void* data { nullptr };
    EventType type { EventType_Move  };
};

/**
 * @brief A collection of events for a single channel
 *
 */
using Channel = std::vector<Event>;


/**
 * @brief Main window
 *
 */
struct Sequentity {

    // Sequentity reads Event and Channel components from your EnTT registry
    // but the involvement of EnTT is minimal and easily replaced by your own
    // implementation. See any reference to _registry below.
    Sequentity(entt::registry& registry) : _registry(registry) {}

    void draw(bool* p_open = nullptr);
    void drawThemeEditor(bool* p_open = nullptr);

    void update();

    void play();
    void step(int time);
    void stop();

    const Event* overlapping(const Channel& channel);
    void each_overlapping(const Channel& channel, std::function<void(const Event*)>);

    // State
    bool playing { false };
    int currentTime { 0 };
    Vector2i range { 0, 250 };

    // Options
    float zoom[2] { 200.0f, 30.0f };
    float scroll[2] { 8.0f, 8.0f };
    int stride { 3 };

    // If an event reaches beyond the end and loops around,
    // should we create a new event at the start to emulate
    // a long-running event? Default is to stop the event
    // at the end.
    bool splitEndEvent { false };

public:
    void before_stepped(std::function<void(int time)> func) { _before_stepped = func; }
    void after_stepped(std::function<void(int time)> func) { _after_stepped = func; }

private:
    entt::registry& _registry;
    int _previousTime { 0 };
    std::function<void(int time)> _before_stepped = [](int) {};
    std::function<void(int time)> _after_stepped = [](int) {};
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

} GlobalTheme;


static struct ListerTheme_ {
    ImVec4 background  { ImColor::HSV(0.0f, 0.0f, 0.200f) };
    ImVec4 alternate   { ImColor::HSV(0.0f, 0.00f, 1.0f, 0.02f) };
    ImVec4 text        { ImColor::HSV(0.0f, 0.0f, 0.850f) };
    ImVec4 dark        { ImColor::HSV(0.0f, 0.0f, 0.150f) };
    ImVec4 mid         { ImColor::HSV(0.0f, 0.0f, 0.314f) };
    ImVec4 accent      { ImColor::HSV(0.0f, 0.75f, 0.750f) };
    
    float width { 100.0f };

} ListerTheme;


static struct TimelineTheme_ {
    ImVec4 background  { ImColor::HSV(0.0f, 0.0f, 0.250f) };
    ImVec4 text        { ImColor::HSV(0.0f, 0.0f, 0.850f) };
    ImVec4 dark        { ImColor::HSV(0.0f, 0.0f, 0.322f) };
    ImVec4 mid         { ImColor::HSV(0.0f, 0.0f, 0.314f) };

    float height { 40.0f };

} TimelineTheme;


static struct EditorTheme_ {
    ImVec4 background  { ImColor::HSV(0.0f, 0.00f, 0.651f) };
    ImVec4 alternate   { ImColor::HSV(0.0f, 0.00f, 0.0f, 0.02f) };
    ImVec4 text        { ImColor::HSV(0.0f, 0.00f, 0.200f) };
    ImVec4 mid         { ImColor::HSV(0.0f, 0.00f, 0.600f) };
    ImVec4 dark        { ImColor::HSV(0.0f, 0.00f, 0.498f) };
    ImVec4 accent      { ImColor::HSV(0.0f, 0.75f, 0.750f) };
    ImVec4 accentLight { ImColor::HSV(0.0f, 0.5f, 1.0f, 0.1f) };
    ImVec4 accentDark  { ImColor::HSV(0.0f, 0.0f, 0.0f, 0.1f) };
    
    float radius { 0.0f };

} EditorTheme;


void Sequentity::play() {
    playing ^= true;
}


void Sequentity::step(int time) {

    // Prevent callbacks from being called unnecessarily
    if (currentTime + time == _previousTime) return;

    _before_stepped(currentTime);

    currentTime += time;

    if (currentTime > range.y()) {
        currentTime = range.x();
    }

    else if (currentTime < range.x()) {
        currentTime = range.y();
    }

    _previousTime = currentTime;

    _after_stepped(currentTime);
}


void Sequentity::stop() {
    step(range.x() - currentTime);
    playing = false;
}


void Sequentity::update() {
    if (playing) {
        step(1);
    }
}


const Event* Sequentity::overlapping(const Channel& channel) {
    const Event* intersecting { nullptr };

    for (auto& event : channel) {
        if (event.time <= currentTime && event.time + event.length > currentTime) {
            intersecting = &event;

            // Ignore overlapping
            // TODO: Remove underlapping
            break;
        }
    }
    return intersecting;
}

void Sequentity::drawThemeEditor(bool* p_open) {
    ImGui::Begin("Theme", p_open);
    {
        if (ImGui::CollapsingHeader("Timeline")) {
            ImGui::ColorEdit4("background", &TimelineTheme.background.x);
            ImGui::ColorEdit4("text", &TimelineTheme.text.x);
            ImGui::ColorEdit4("dark", &TimelineTheme.dark.x);
            ImGui::ColorEdit4("mid", &TimelineTheme.mid.x);
        }

        if (ImGui::CollapsingHeader("Editor")) {
            ImGui::ColorEdit4("background##editor", &EditorTheme.background.x);
            ImGui::ColorEdit4("alternate##editor", &EditorTheme.alternate.x);
            ImGui::ColorEdit4("text##editor", &EditorTheme.text.x);
            ImGui::ColorEdit4("mid##editor", &EditorTheme.mid.x);
            ImGui::ColorEdit4("dark##editor", &EditorTheme.dark.x);
            ImGui::ColorEdit4("accent##editor", &EditorTheme.accent.x);
        }
    }
    ImGui::End();
}


void Sequentity::draw(bool* p_open) {
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar
                                 | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::Begin("Editor", p_open, windowFlags);
    {
        auto* painter = ImGui::GetWindowDrawList();
        const auto& style = ImGui::GetStyle();
        auto titlebarHeight = 24.0f; // TODO: Find an exact value for this
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 windowPos = ImGui::GetWindowPos() + ImVec2{ 0.0f, titlebarHeight };
        float lineHeight = ImGui::GetTextLineHeight() + style.ItemSpacing.y;

        /**
         * @brief Sequentity divided into 4 panels
         *
         *          ___________________________
         *         |       |                   |
         *   Cross |   X   |        B          | Timeline
         *         |_______|___________________|
         *         |       |                   |
         *         |       |                   |
         *         |       |                   |
         *  Lister |   A   |        C          | Editor
         *         |       |                   |
         *         |       |                   |
         *         |_______|___________________|
         *
         */

        auto X = windowPos;
        auto A = windowPos + ImVec2{ 0.0f, TimelineTheme.height };
        auto B = windowPos + ImVec2{ ListerTheme.width, 0.0f };
        auto C = windowPos + ImVec2{ ListerTheme.width, TimelineTheme.height };

        float zoom_ = zoom[0] / stride;
        int stride_ = stride * 5;  // How many frames to skip drawing
        int minTime = range.x() / stride_;
        int maxTime = range.y() / stride_;

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

            // Stipple
            bool isOdd { true };
            for (float y = 0; y < windowSize.y; y += zoom[1]) {
                isOdd ^= true;

                if (isOdd) painter->AddRectFilled(
                    A + ImVec2{ 0, y + scroll[1] },
                    A + ImVec2{ ListerTheme.width, y + scroll[1] + zoom[1] - 1 },
                    ImColor(ListerTheme.alternate)
                );
            }

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

                xMin += B.x + scroll[0];
                xMax += B.x + scroll[0];
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

        auto drawHorizontalBar = [&]() {
            float xMin = 0.0f;
            float xMax = windowSize.x;
            float yMin = TimelineTheme.height;
            float yMax = TimelineTheme.height;

            xMin += B.x + scroll[0];
            xMax += B.x + scroll[0];
            yMin += B.y;
            yMax += B.y;

            painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMax, yMax), ImColor(EditorTheme.dark), 1.0f);
        };

        auto drawVerticalGrid = [&]() {
            for (int time = minTime; time < maxTime + 1; time++) {
                float xMin = time * zoom_;
                float xMax = 0.0f;
                float yMin = 0.0f;
                float yMax = windowSize.y;

                xMin += C.x + scroll[0];
                xMax += C.x + scroll[0];
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
         *
         *         __
         *        |  |
         *  _______\/___________________________________________________
         *         |
         *         |
         *         |
         *         |
         *         |
         *         |
         *         |
         *         |
         *
         *
         */
        auto drawCurrentTime = [&]() {
            auto xMin = currentTime * zoom_ / stride_;
            auto xMax = 0.0f;
            auto yMin = TimelineTheme.height;
            auto yMax = windowSize.y;

            xMin += B.x + scroll[0];
            xMax += B.x + scroll[0];
            yMin += B.y;
            yMax += B.y;

            painter->AddLine(ImVec2{ xMin, yMin }, ImVec2{ xMin, yMax }, ImColor(EditorTheme.accent), 2.0f);

            /**
             *  Cursor
             *    __
             *   |  |
             *    \/
             *
             */
            auto width = 10.0f;
            auto height = 20.0f;
            auto topPos = ImVec2{ xMin, yMin };

            ImVec2 points[5] = {
                topPos,
                topPos - ImVec2{ -width, height / 2.0f },
                topPos - ImVec2{ -width, height },
                topPos - ImVec2{ width, height },
                topPos - ImVec2{ width, height / 2.0f }
            };

            ImVec2 shadow1[5];
            ImVec2 shadow2[5];
            for (int i=0; i < 5; i++) { shadow1[i] = points[i] + ImVec2{ 1.0f, 1.0f }; }
            for (int i=0; i < 5; i++) { shadow2[i] = points[i] + ImVec2{ 3.0f, 3.0f }; }

            painter->AddConvexPolyFilled(shadow1, 5, ImColor(GlobalTheme.shadow));
            painter->AddConvexPolyFilled(shadow2, 5, ImColor(GlobalTheme.shadow));
            painter->AddConvexPolyFilled(points, 5, ImColor(EditorTheme.accent));
            painter->AddPolyline(points, 5, ImColor(EditorTheme.accentDark), true, 1.0f);
            painter->AddLine(topPos - ImVec2{  2.0f, height * 0.3f },
                             topPos - ImVec2{  2.0f, height * 0.8f },
                             ImColor(EditorTheme.accentDark));
            painter->AddLine(topPos - ImVec2{ -2.0f, height * 0.3f },
                             topPos - ImVec2{ -2.0f, height * 0.8f },
                             ImColor(EditorTheme.accentDark));
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
            yMin += A.y + scroll[1];
            yMax += A.y + scroll[1];

            bool isOdd = false;
            for (float y = yMin; y < yMax; y += zoom[1]) {
                isOdd ^= true;

                if (isOdd) painter->AddRectFilled(
                    { xMin, y },
                    { xMax, y + zoom[1] - 1 },
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
            _registry.view<Index, Name, Channel, Color>().each([&](const auto index,
                                                                  const auto& name,
                                                                  auto& channel,
                                                                  const auto& color) {
                unsigned int count { 0 };

                for (auto& event : channel) {
                    static int initialTime { 0 };
                    int startTime = event.time;
                    int endTime = startTime + event.length;

                    float xMin = static_cast<float>(startTime) * zoom_ / stride_;
                    float xMax = static_cast<float>(endTime) * zoom_ / stride_;
                    float yMin = zoom[1] * index.relative;
                    float yMax = yMin + zoom[1];

                    xMin += C.x + scroll[0];
                    xMax += C.x + scroll[0];
                    yMin += C.y + scroll[1];
                    yMax += C.y + scroll[1];

                    ImGui::SetCursorPos(ImVec2{ xMin, yMin } - ImGui::GetWindowPos());

                    const char* label = (std::string("##event") + name.text + std::to_string(count)).c_str();
                    ImVec2 size { static_cast<float>(event.length * zoom_ / stride_), zoom[1]};
                    ImGui::InvisibleButton(label, size);

                    float thickness = 0.0f;
                    ImVec4 currentFill = color.fill;

                    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                        thickness = 3.0f;
                        currentFill = color.fill * 1.1f;
                    }

                    if (ImGui::IsItemActivated()) {
                        initialTime = startTime;
                    }

                    // User Input
                    // TODO: Move this out of the drawing code
                    if (!ImGui::GetIO().KeyAlt && ImGui::IsItemActive()) {
                        float delta = ImGui::GetMouseDragDelta().x;
                        event.time = initialTime + static_cast<int>(delta / (zoom_ / stride_));
                    }

                    const int shadow = 2;
                    painter->AddRectFilled(
                        { xMin + shadow, yMin + shadow },
                        { xMax + shadow, yMax + shadow - 1 },
                        ImColor::HSV(0.0f, 0.0f, 0.0f, 0.3f), EditorTheme.radius
                    );

                    painter->AddRectFilled({ xMin, yMin }, { xMax, yMax - 1 }, ImColor(currentFill), EditorTheme.radius);

                    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                        painter->AddRect(
                            { xMin + (thickness - 1) / 2, yMin + (thickness - 1) / 2 },
                            { xMax - (thickness - 1) / 2, yMax - (thickness - 1) / 2 - 1 },
                            ImColor::HSV(0.15f, 0.7f, 1.0f), EditorTheme.radius, ImDrawCornerFlags_All, thickness
                        );
                    }
                    else {
                        painter->AddRect(
                            { xMin + thickness, yMin + thickness },
                            { xMax - thickness, yMax - thickness - 1 },
                            ImColor(color.stroke), EditorTheme.radius
                        );
                    }

                    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                        painter->AddText(ImVec2{ xMin + 5.0f, yMin }, ImColor(1.0f, 1.0f, 1.0f, 1.0f), std::to_string(startTime).c_str());
                        
                        if (event.length > 20.0f) {
                            painter->AddText(ImVec2{ xMin + event.length * (zoom_ / stride_) - 20.0f, yMin }, ImColor(1.0f, 1.0f, 1.0f, 1.0f), std::to_string(event.length).c_str());
                        }
                    }

                    count++;
                }
            });
        };

        /**
         * @brief Outliner-style listing of available channels
         *
         *  ______________
         * |              |
         * | Channel A    |
         * | Channel B    |
         * | Channel C    |
         * | Channel D    |
         * | ...          |
         * |              |
         * |______________|
         *
         *
         */
        auto drawLister = [&]() {
            const ImVec2 size { ListerTheme.width, zoom[1] };

            _registry.view<Name, Index, Channel>().each([&](auto entity, const auto& name, const auto& index, const auto&) {
                float xMin = 0.0f;
                float xMax = ListerTheme.width;
                float yMin = zoom[1] * index.relative;
                float yMax = 0.0f;

                xMin += A.x;
                xMax += A.x;
                yMin += A.y + scroll[1];
                yMax += A.y + scroll[1];

                ImGuiSelectableFlags flags = 0;
                auto topLeftGlobal = ImVec2{ xMin, yMin };
                auto topLeftLocal = topLeftGlobal - ImGui::GetWindowPos();

                const auto padding = 5.0f;
                const auto textSize = ImGui::CalcTextSize(name.text);
                painter->AddText(topLeftGlobal + ImVec2{ ListerTheme.width - textSize.x - padding, zoom[1] / 2.0f - textSize.y / 2.0f }, ImColor(ListerTheme.text), name.text);
            });
        };

        painter->PushClipRect(B + ImVec2{ 1.0f, 0.0f }, ImGui::GetWindowPos() + ImGui::GetWindowSize());
        {
            drawEditorBackground();
            drawHorizontalGrid();
            drawHorizontalBar();
            drawVerticalGrid();
            drawEvents();
            drawTimelineBackground();
            drawTimeline();
            drawCurrentTime();
        }
        painter->PopClipRect();

        drawListerBackground();
        drawLister();
        drawCrossBackground();

        //
        // User Input
        //

        /** Dual Scroll
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

        /** Vertical Scroll
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

        /** Vertical Scroll
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
            scroll[0] += ImGui::GetIO().MouseDelta.x;
            scroll[1] += ImGui::GetIO().MouseDelta.y;
        }
        else if (scrollV) scroll[1] += ImGui::GetIO().MouseDelta.y;
        else if (scrollH) scroll[0] += ImGui::GetIO().MouseDelta.x;
    }
    ImGui::End();
}

}