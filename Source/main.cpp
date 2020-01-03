#include <string>
#include <iostream>
#include <unordered_map>

#include <Magnum/Math/Color.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Platform/GlfwApplication.h>

#include <imgui_internal.h>
#include <entt/entity/registry.hpp>

#include "Theme.h"

using namespace Magnum;
using namespace Math::Literals;

static entt::registry Registry;

// Components
struct BendollEvent {};

struct DragdollEvent {
    int time;
    int length;

    /** Relative the target center, where did the press occur?
     *
     * (0, 0)
     *    .____________
     *    |            |
     *    |            |
     *    |            |
     *    |    (30, 40)|
     *    |      .     |
     *    |______|\____|
     *           |_\    
     *             \    
     *               
     */
    Vector2i offset;

    /* In absolute coordinates, where the press occur? */
    Vector2i pressPosition;
    Vector2i releasePosition;

    /* Absolute move positions */
    std::vector<Vector2i> data;
};

using Position = Vector2i;
struct InitialPosition : Vector2i { using Vector2i::Vector2i; };
struct StartPosition : Vector2i { using Vector2i::Vector2i; };
struct Size : Vector2i { using Vector2i::Vector2i; };
using BenddollEvents = std::vector<BendollEvent>;
using DragdollEvents = std::vector<DragdollEvent>;

struct Color {
    ImVec4 fill;
    ImVec4 stroke;
};

struct Name {
    const char* text;
};


static struct OutlinerTheme_ {
    ImVec4 background  { ImColor::HSV(0.0f, 0.0f, 0.200f) };
    ImVec4 text        { ImColor::HSV(0.0f, 0.0f, 0.850f) };
    ImVec4 dark        { ImColor::HSV(0.0f, 0.0f, 0.150f) };
    ImVec4 mid         { ImColor::HSV(0.0f, 0.0f, 0.314f) };
    ImVec4 accent      { ImColor::HSV(0.0f, 0.75f, 0.750f) };
    
    float width { 200.0f };

} OutlinerTheme;


static struct TimelineTheme_ {
    ImVec4 background  { ImColor::HSV(0.0f, 0.0f, 0.250f) };
    ImVec4 text        { ImColor::HSV(0.0f, 0.0f, 0.850f) };
    ImVec4 dark        { ImColor::HSV(0.0f, 0.0f, 0.322f) };
    ImVec4 mid         { ImColor::HSV(0.0f, 0.0f, 0.314f) };

    float height { 60.0f };

} TimelineTheme;


static struct EditorTheme_ {
    ImVec4 background  { ImColor::HSV(0.0f, 0.00f, 0.651f) };
    ImVec4 alternate   { ImColor::HSV(0.0f, 0.00f, 0.627f) };
    ImVec4 text        { ImColor::HSV(0.0f, 0.00f, 0.200f) };
    ImVec4 mid         { ImColor::HSV(0.0f, 0.00f, 0.600f) };
    ImVec4 dark        { ImColor::HSV(0.0f, 0.00f, 0.498f) };
    ImVec4 accent      { ImColor::HSV(0.0f, 0.75f, 0.750f) };
    
    float radius { 0.0f };

} EditorTheme;


using SmartButtonState = int;
enum SmartButtonState_ {
    SmartButtonState_None = 0,
    SmartButtonState_Hovered = 1 << 1,
    SmartButtonState_Pressed = 1 << 2,
    SmartButtonState_Dragged = 1 << 3,
    SmartButtonState_Released = 1 << 4
};


static SmartButtonState SmartButton(const char* label, ImVec2 size = {0, 0}) {
    bool useless = ImGui::Button(label, size);

    // Return value doesn't take into account release
    // when cursor is outside the bounds of the button.

    SmartButtonState state { 0 };
    if (ImGui::IsItemHovered()) state |= SmartButtonState_Hovered;

    if (ImGui::IsItemActivated()) {
        state |= SmartButtonState_Pressed;
    } else if (ImGui::IsItemActive()) {
        state |= SmartButtonState_Dragged;
    } else if (ImGui::IsItemDeactivated()) {
        state |= SmartButtonState_Released;
    }

    return state;
}


struct Sequencer {
    void draw();
    void update();
    void step(int time);

    const DragdollEvent* overlapping(const DragdollEvents& events);

    Vector2i range { 0, 250 };
    int currentTime { 0 };
    int previousTime { 0 };
    float zoom { 200.0f };
    float scroll { 0.0f };
    int stride { 3 };

    // If an event reaches beyond the end and loops around,
    // should we create a new event at the start to emulate
    // a long-running event? Default is to stop the event
    // at the end.
    bool splitEndEvent { false };
};


void Sequencer::update() {
    if (previousTime == currentTime) return;

    auto startTime = range.x();

    if (currentTime == startTime) {
        Registry.view<Position, InitialPosition>().each([&](auto& position, const auto& initial) {
            position = initial;
        });
    }

    else {
        Registry.view<Position, DragdollEvents, Color>().each([&](auto entity, auto& position, const auto& events, const auto& color) {
            /**  Find intersecting event, backwards
             *
             *               time
             *                 |
             *    _____________|__________
             *   |_____________|__________|
             *   ^             |
             * event           |
             *                 
             */

            const DragdollEvent* current = overlapping(events);

            // Inbetween events, that's ok
            if (current == nullptr) return;

            const int index = currentTime - current->time;
            const auto value = current->data[index] - current->offset;
            position = value;
        });

        Registry.reset<StartPosition>();
    }

    previousTime = currentTime;
}


const DragdollEvent* Sequencer::overlapping(const DragdollEvents& events) {
    const DragdollEvent* intersecting { nullptr };

    for (auto& event : events) {
        if (event.time < currentTime && event.time + event.length > currentTime) {
            intersecting = &event;

            // Ignore overlapping
            // TODO: Remove underlapping
            break;
        }
    }
    return intersecting;
}


void Sequencer::step(int time) {
    update();

    currentTime += time;

    if (currentTime > range.y()) {
        currentTime = range.x();
    }
}


void Sequencer::draw() {
    ImGui::Begin("Editor", nullptr);
    {
        auto* painter = ImGui::GetWindowDrawList();
        const auto& style = ImGui::GetStyle();
        auto titlebarHeight = 24.0f; // TODO: Find an exact value for this
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 windowPos = ImGui::GetWindowPos() + ImVec2{ 0.0f, titlebarHeight };
        float lineHeight = ImGui::GetTextLineHeight() + style.ItemSpacing.y;

        /**
         * @brief Sequencer divided into 3 panels
         *
         *     ___________________________
         *    |       |                   |
         *    |       |        B          |
         *    |       |___________________|
         *    |       |                   |
         *    |   A   |                   |
         *    |       |                   |
         *    |       |        C          |
         *    |       |                   |
         *    |       |                   |
         *    |_______|___________________|
         *
         */

        auto A = windowPos;
        auto B = windowPos + ImVec2{ OutlinerTheme.width, 0.0f };
        auto C = windowPos + ImVec2{ OutlinerTheme.width, TimelineTheme.height };

        int stride_ = stride * 5;  // How many frames to skip drawing
        float zoom_ = zoom / stride;
        int minTime = range.x() / stride_;
        int maxTime = range.y() / stride_;

        auto drawOutlinerBackground = [&]() {
            painter->AddRectFilled(
                A,
                A + ImVec2{ OutlinerTheme.width, windowSize.y },
                ImColor(OutlinerTheme.background)
            );

            painter->AddLine(A + ImVec2{ OutlinerTheme.width, 0.0f },
                             A + ImVec2{ OutlinerTheme.width, windowSize.y },
                             ImColor(OutlinerTheme.dark));
        };

        auto drawTimelineBackground = [&]() {
            painter->AddRectFilled(
                B,
                B + ImVec2{ windowSize.x, TimelineTheme.height },
                ImColor(TimelineTheme.background)
            );
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
                float yMax = TimelineTheme.height;

                xMin += B.x + scroll;
                xMax += B.x + scroll;
                yMin += B.y;
                yMax += B.y;

                painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), ImColor(TimelineTheme.dark));
                painter->AddText(
                    ImGui::GetFont(),
                    ImGui::GetFontSize() * 0.85f,
                    ImVec2(xMin + 5.0f, yMin + lineHeight),
                    ImColor(TimelineTheme.text),
                    std::to_string(time * stride_).c_str()
                );

                if (time == maxTime) break;

                for (int z = 0; z < 5 - 1; z++) {
                    const auto innerSpacing = zoom_ / 5;
                    auto subline = innerSpacing * (z + 1);
                    painter->AddLine(ImVec2(xMin + subline, yMin + 35), ImVec2(xMin + subline, yMax), ImColor(TimelineTheme.mid));
                }
            }
        };

        auto drawHorizontalBar = [&]() {
            float xMin = 0.0f;
            float xMax = windowSize.x;
            float yMin = TimelineTheme.height;
            float yMax = TimelineTheme.height;

            xMin += B.x + scroll;
            xMax += B.x + scroll;
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

                xMin += C.x + scroll;
                xMax += C.x + scroll;
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

        auto drawCurrentTime = [&]() {
            auto xMin = currentTime * zoom_ / stride_;
            auto xMax = 0.0f;
            auto yMin = 0.0f;
            auto yMax = windowSize.y;

            xMin += B.x + scroll;
            xMax += B.x + scroll;
            yMin += B.y;
            yMax += B.y;

            painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), ImColor(EditorTheme.accent), 2.0f);

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

            xMin += C.x + scroll;
            xMax += C.x + scroll;
            yMin += C.y;
            yMax += C.y;

            bool isOdd = false;
            for (float y = yMin; y < yMax; y += lineHeight) {
                isOdd ^= true;

                if (isOdd) painter->AddRectFilled(
                    { xMin, y },
                    { xMax, y + lineHeight - 1 },
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
            int yOffset = 0;

            Registry.view<Name, DragdollEvents, Color>().each([&](const auto& name, auto& events, const auto& color) {
                int count { 0 };

                for (auto& event : events) {
                    static int initialTime { 0 };
                    int startTime = event.time;
                    int endTime = startTime + event.length;

                    float xMin = static_cast<float>(startTime) * zoom_ / stride_;
                    float xMax = static_cast<float>(endTime) * zoom_ / stride_;
                    float yMin = lineHeight * yOffset;
                    float yMax = yMin + lineHeight;

                    xMin += C.x + scroll;
                    xMax += C.x + scroll;
                    yMin += C.y;
                    yMax += C.y;

                    ImGui::SetCursorPos(ImVec2{ xMin, yMin } - ImGui::GetWindowPos());

                    const char* label = (std::string("##event") + name.text + std::to_string(count)).c_str();
                    ImVec2 size { static_cast<float>(event.length * zoom_ / stride_), lineHeight};
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

                    if (ImGui::IsItemActive()) {
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

                yOffset += 1;
            });

        };

        painter->PushClipRect(B + ImVec2{ 1.0f, 0.0f }, ImGui::GetWindowPos() + ImGui::GetWindowSize());
        {
            drawEditorBackground();
            drawTimelineBackground();
            drawTimeline();
            drawHorizontalGrid();
            drawHorizontalBar();
            drawVerticalGrid();
            drawEvents();
            drawCurrentTime();
        }
        painter->PopClipRect();

        drawOutlinerBackground();

        ImGui::SetCursorPos({ OutlinerTheme.width, titlebarHeight });
        ImGui::InvisibleButton("##scroll", ImVec2{ windowSize.x, TimelineTheme.height });

        if (ImGui::IsItemActive()) {
            scroll += ImGui::GetIO().MouseDelta.x;
        }
    }
    ImGui::End();
}


class Application : public Platform::Application {
public:
    explicit Application(const Arguments& arguments);

    void drawEvent() override;
    void drawRigids();
    void drawThemeEditor();
    void drawTransport();
    void drawTimeline();
    void drawCentralWidget();

    void setup();
    void clear();

    auto dpiScaling() const -> Vector2;
    void viewportEvent(ViewportEvent& event) override;

    void keyPressEvent(KeyEvent& event) override;
    void keyReleaseEvent(KeyEvent& event) override;

    void mousePressEvent(MouseEvent& event) override;
    void mouseReleaseEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void mouseScrollEvent(MouseScrollEvent& event) override;
    void textInputEvent(TextInputEvent& event) override;

private:
    ImGuiIntegration::Context _imgui{ NoCreate };
    Sequencer _sequencer;

    Vector2 _dpiScaling { 1.0f, 1.0f };

    bool _running { true };
    bool _playing { true };
    bool _relative { true };

    bool _showMetrics { false };
    bool _showStyleEditor { false };

    Vector2i _redPos { 300, 50 };
    Vector2i _greenPos { 550, 50 };
    Vector2i _bluePos { 800, 50 };
    Vector2i _redInitialPos { 300, 50 };
    Vector2i _greenInitialPos { 550, 50 };
    Vector2i _blueInitialPos { 800, 50 };
};


Application::Application(const Arguments& arguments): Platform::Application{arguments,
    Configuration{}.setTitle("Application")
                   .setSize({1600, 900})
                   .setWindowFlags(Configuration::WindowFlag::Resizable)}
{
    // Use virtual scale, rather than physical
    glfwGetWindowContentScale(this->window(), &_dpiScaling.x(), &_dpiScaling.y());

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(this->window(),
        (mode->width / 2) - (windowSize().x() / 2),
        (mode->height / 2) - (windowSize().y() / 2)
    );

    this->setSwapInterval(1);  // VSync

    // IMPORTANT: In order to add a new font, we need to
    // first create an ImGui context. But, it has to happen
    // **before** creating the ImGuiIntegration::Context.
    ImGui::CreateContext();

    auto& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("OpenSans-Regular.ttf", 16.0f * dpiScaling().x());

    _imgui = ImGuiIntegration::Context(

        // Note, in order for the newly added font to remain, we'll need to
        // pass the current context we just created back into the integration.
        // Your code would run without this, but would also throw your font away.
        *ImGui::GetCurrentContext(),

        Vector2{ windowSize() } / dpiScaling(),
        windowSize(), framebufferSize()
    );

    // Shouldn't be, but is, necessary
    ImGui::SetCurrentContext(_imgui.context());

    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
    ImGui::GetIO().ConfigWindowsResizeFromEdges = true;
    
    // Optional
    Theme();

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
    ImGui::GetIO().ConfigDockingWithShift = true;

    /* Set up proper blending to be used by ImGui. There's a great chance
       you'll need this exact behavior for the rest of your scene. If not, set
       this only for the drawFrame() call. */
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    setup();
}


void Application::setup() {
    auto red = Registry.create();
    auto green = Registry.create();
    auto blue = Registry.create();

    Registry.assign<Name>(red, "Red Rigid");
    Registry.assign<Size>(red, Vector2i{ 100, 100 });
    Registry.assign<Color>(red, ImColor::HSV(0.0f, 0.75f, 0.75f), ImColor::HSV(0.0f, 0.75f, 0.1f));
    Registry.assign<Position>(red, Vector2i{ 300, 50 });
    Registry.assign<InitialPosition>(red, Vector2i{ 300, 50 });

    Registry.assign<Name>(green, "Green Rigid");
    Registry.assign<Size>(green, Vector2i{ 100, 100 });
    Registry.assign<Color>(green, ImColor::HSV(0.33f, 0.75f, 0.75f), ImColor::HSV(0.33f, 0.75f, 0.1f));
    Registry.assign<Position>(green, Vector2i{ 500, 50 });
    Registry.assign<InitialPosition>(green, Vector2i{ 500, 50 });

    Registry.assign<Name>(blue, "Blue Rigid");
    Registry.assign<Size>(blue, Vector2i{ 100, 100 });
    Registry.assign<Color>(blue, ImColor::HSV(0.55f, 0.75f, 0.75f), ImColor::HSV(0.55f, 0.75f, 0.1f));
    Registry.assign<Position>(blue, Vector2i{ 700, 50 });
    Registry.assign<InitialPosition>(blue, Vector2i{ 700, 50 });
}


auto Application::dpiScaling() const -> Vector2 { return _dpiScaling; }


void Application::clear() {
    Registry.view<DragdollEvents>().each([](auto& events) {
        events.clear();
    });
}


void Application::drawCentralWidget() {
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking
                                 | ImGuiWindowFlags_NoTitleBar
                                 | ImGuiWindowFlags_NoCollapse
                                 | ImGuiWindowFlags_NoResize
                                 | ImGuiWindowFlags_NoMove
                                 | ImGuiWindowFlags_NoBringToFrontOnFocus
                                 | ImGuiWindowFlags_NoNavFocus;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    // This is basically the background window that contains all the dockable windows
    ImGui::Begin("InvisibleWindow", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockSpaceId = ImGui::GetID("InvisibleWindowDockSpace");

    if(!ImGui::DockBuilderGetNode(dockSpaceId)) {
        ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockSpaceId, viewport->Size);

        auto timelineHeight = 40.0f; // px, unscaled
        auto shelfHeight = 40.0f;
        auto outlinerWidth = 200.0f;
        auto channelBoxWidth = 400.0f;

        ImGuiID center = dockSpaceId;
        ImGuiID top = ImGui::DockBuilderSplitNode(center, ImGuiDir_Up, 0.1f, nullptr, &center);
        ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.25f, nullptr, &center);
        ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
        ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.5f, nullptr, &center);

        ImGui::DockBuilderDockWindow("Transport", left);
        ImGui::DockBuilderDockWindow("Buttons", bottom);
        ImGui::DockBuilderDockWindow("Editor", bottom);

        ImGui::DockBuilderFinish(center);
    }

    ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f));
    ImGui::End();
}


void Application::drawTransport() {
    ImGui::Begin("Transport", nullptr);
    {
        if (ImGui::Button("Play")) _playing ^= true;
        ImGui::SameLine();
        if (ImGui::Button("<")) _sequencer.step(-1);
        ImGui::SameLine();
        if (ImGui::Button(">")) _sequencer.step(1);

        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            _sequencer.currentTime = _sequencer.range.x();
            _playing = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            clear();
        }

        if (ImGui::DragInt("Time", &_sequencer.currentTime, 1.0f, _sequencer.range.x(), _sequencer.range.y())) {
            _sequencer.update();
        }

        if (ImGui::DragInt2("Range", _sequencer.range.data())) {
            if (_sequencer.range.x() < 0) _sequencer.range.x() = 0;
            if (_sequencer.range.y() < 5) _sequencer.range.y() = 5;

            if (_sequencer.currentTime < _sequencer.range.x()) {
                _sequencer.currentTime = _sequencer.range.x();
            }

            if (_sequencer.currentTime > _sequencer.range.y()) {
                _sequencer.currentTime = _sequencer.range.y();
            }
        }

        ImGui::SliderFloat("Zoom", &_sequencer.zoom, 20.0f, 400.0f);
        ImGui::DragFloat("Scroll", &_sequencer.scroll);
        ImGui::SliderInt("Stride", &_sequencer.stride, 1, 5);

        ImGui::Checkbox("Relative", &_relative);
    }
    ImGui::End();
}


void Application::drawRigids() {
    ImVec2 size = ImGui::GetWindowSize();
    Vector2i shapeSize { 100, 100 };

    auto drawCursor = [&](ImVec2 corner, ImColor color) {
        auto root = ImGui::GetWindowPos();
        auto painter = ImGui::GetWindowDrawList();

        auto abscorner = root + corner;

        painter->AddCircleFilled(
            abscorner,
            10.0f,
            ImColor::HSV(0.0f, 0.0f, 1.0f)
        );
    };

    auto currentTime = _sequencer.currentTime;
    ImGui::Begin("Shapes", nullptr);
    {
        Registry.view<Name, Position, Size, Color>().each([&](auto entity,
                                                              const auto& name,
                                                              const auto& position,
                                                              const auto& size,
                                                              const auto& color) {
            auto& events = Registry.get_or_assign<DragdollEvents>(entity);

            static DragdollEvent* current { nullptr };

            auto imsize = ImVec2((float)size.x(), (float)size.y());
            auto impos = ImVec2((float)position.x(), (float)position.y());

            auto mousePosition = Vector2i(Vector2(ImGui::GetIO().MousePos - ImGui::GetWindowPos()));

            ImGui::SetCursorPos(impos);
            auto state = SmartButton(name.text, imsize);

            if (state & SmartButtonState_Pressed) {
                DragdollEvent event;

                event.time = currentTime;
                event.length = 1;
                event.offset = mousePosition - position;
                event.pressPosition = position;
                event.releasePosition = position;
                event.data.push_back(mousePosition);

                events.push_back(event);
                current = &events.back();
            }

            else if (state & SmartButtonState_Dragged) {
                if (current != nullptr && current->time + current->length > currentTime) {
                    current = nullptr;
                }

                if (current != nullptr) {
                    current->data.push_back(mousePosition);
                    current->releasePosition = position;
                    current->length += 1;
                }
            }

            else if (state & SmartButtonState_Released) {
                current = nullptr;
            }

            if (auto ovl = _sequencer.overlapping(events)) {
                impos.x += ovl->offset.x();
                impos.y += ovl->offset.y();
                drawCursor(impos, color.fill);
            }
        });
    }
    ImGui::End();
}

void Application::drawThemeEditor() {
    static bool open = true;

    ImGui::Begin("Theme");
    {
        if (ImGui::CollapsingHeader("Timeline", &open)) {
            ImGui::ColorEdit4("background", &TimelineTheme.background.x);
            ImGui::ColorEdit4("text", &TimelineTheme.text.x);
            ImGui::ColorEdit4("dark", &TimelineTheme.dark.x);
            ImGui::ColorEdit4("mid", &TimelineTheme.mid.x);
        }

        if (ImGui::CollapsingHeader("Editor", &open)) {
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


void Application::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _imgui.newFrame();

    if (ImGui::GetIO().WantTextInput && !isTextInputActive()) startTextInput();
    else if (!ImGui::GetIO().WantTextInput && isTextInputActive()) stopTextInput();

    drawCentralWidget();
    drawTransport();
    drawRigids();

    if (_playing) {
        _sequencer.step(1);
    }

    _sequencer.draw();

    if (_showMetrics) {
        ImGui::ShowMetricsWindow(&_showMetrics);
    }

    if (_showStyleEditor) {
        drawThemeEditor();
        ImGui::ShowStyleEditor();
    }

    _imgui.updateApplicationCursor(*this);

    {
        GL::Renderer::enable(GL::Renderer::Feature::Blending);
        GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
        GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
        GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

        _imgui.drawFrame();

        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
        GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
        GL::Renderer::disable(GL::Renderer::Feature::Blending);
    }

    swapBuffers();

    if (_running) {
        redraw();
    }
}

void Application::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _imgui.relayout(Vector2{ event.windowSize() } / dpiScaling(),
        event.windowSize(), event.framebufferSize());
}

void Application::keyPressEvent(KeyEvent& event) {
    if (event.key() == KeyEvent::Key::Esc) this->exit();
    if (event.key() == KeyEvent::Key::Enter) redraw();
    if (event.key() == KeyEvent::Key::Backspace) _running ^= true;
    if (event.key() == KeyEvent::Key::Delete) clear();
    if (event.key() == KeyEvent::Key::F1) _showMetrics ^= true;
    if (event.key() == KeyEvent::Key::F2) _showStyleEditor ^= true;
    if(_imgui.handleKeyPressEvent(event)) return;
}

void Application::keyReleaseEvent(KeyEvent& event) {
    if(_imgui.handleKeyReleaseEvent(event)) return;
}

void Application::mousePressEvent(MouseEvent& event) {
    if(_imgui.handleMousePressEvent(event)) return;
}

void Application::mouseReleaseEvent(MouseEvent& event) {
    if(_imgui.handleMouseReleaseEvent(event)) return;
}

void Application::mouseMoveEvent(MouseMoveEvent& event) {
    if(_imgui.handleMouseMoveEvent(event)) return;
}

void Application::mouseScrollEvent(MouseScrollEvent& event) {
    if(_imgui.handleMouseScrollEvent(event)) {
        /* Prevent scrolling the page */
        event.setAccepted();
        return;
    }
}

void Application::textInputEvent(TextInputEvent& event) {
    if(_imgui.handleTextInputEvent(event)) return;
}


MAGNUM_APPLICATION_MAIN(Application)