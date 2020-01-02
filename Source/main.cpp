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
struct Event {
    enum Type {
        Red, Green, Blue
    } type;

    int time;
    int length;
};

using Events = std::vector<Event>;
using Position = Vector2i;
struct InitialPosition : Vector2i { using Vector2i::Vector2i; };
struct Size : Vector2i { using Vector2i::Vector2i; };

struct Color {
    ImColor fill;
    ImColor stroke;
};

struct Name {
    const char* text;
};

using Channel = std::unordered_map<int, Vector2i>;


static struct TimelineTheme_ {
    ImColor background  { ImColor::HSV(0.0f, 0.0f, 0.250f) };
    ImColor text        { ImColor::HSV(0.0f, 0.0f, 0.850f) };
    ImColor dark        { ImColor::HSV(0.0f, 0.0f, 0.322f) };
    ImColor mid         { ImColor::HSV(0.0f, 0.0f, 0.314f) };

    float height { 60.0f };

} TimelineTheme;


static struct EditorTheme_ {
    ImColor background  { ImColor::HSV(0.0f, 0.00f, 0.651f) };
    ImColor alternate   { ImColor::HSV(0.0f, 0.00f, 0.627f) };
    ImColor text        { ImColor::HSV(0.0f, 0.00f, 0.200f) };
    ImColor mid         { ImColor::HSV(0.0f, 0.00f, 0.600f) };
    ImColor dark        { ImColor::HSV(0.0f, 0.00f, 0.498f) };
    ImColor accent      { ImColor::HSV(0.0f, 0.75f, 0.750f) };
    
    float radius { 0.0f };

} EditorTheme;


using SmartButtonState = int;
enum SmartButtonState_ {
    SmartButtonState_None = 0,
    SmartButtonState_Hovered = 1 << 1,
    SmartButtonState_Pressed = 1 << 2,
    SmartButtonState_Dragged = 1 << 3,
    SmartButtonState_Released = 1 << 4,
    SmartButtonState_DoubleClicked = 1 << 5, // Not yet implemented
    SmartButtonState_Idle
};


static SmartButtonState SmartButton(const char* label, SmartButtonState& previous, ImVec2 size = {0, 0}) {
    bool released = ImGui::Button(label, size);

    const bool wasDragged = previous & SmartButtonState_Dragged;
    const bool wasPressed = previous & SmartButtonState_Pressed;

    SmartButtonState current { 0 };
    if (released) current |= SmartButtonState_Released;
    if (ImGui::IsItemActive()) current |= SmartButtonState_Pressed;
    if (ImGui::IsItemHovered()) current |= SmartButtonState_Hovered;

    // Detect when button is dragged
    if (current & SmartButtonState_Pressed) {
        if (wasDragged || wasPressed) {
            current = SmartButtonState_Dragged;
        }
    }

    // Detect release event, despite not happening within bounds of button
    if ((wasPressed || wasDragged) && !ImGui::IsAnyMouseDown()) {
        current = SmartButtonState_Released;
    }

    previous = current;
    return current;
}


struct Sequencer {
    void draw();

    Vector2i range { 0, 250 };
    int currentTime { 0 };
    float zoom { 200.0f };
    int stride { 3 };

    // If an event reaches beyond the end and loops around,
    // should we create a new event at the start to emulate
    // a long-running event? Default is to stop the event
    // at the end.
    bool splitEndEvent { false };
};


void Sequencer::draw() {
    ImGui::Begin("Editor", nullptr);
    {

        auto* painter = ImGui::GetWindowDrawList();
        const auto& style = ImGui::GetStyle();
        ImVec2 windowSize = ImGui::GetWindowSize();
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

        auto A = ImGui::GetWindowPos() + ImVec2{ 5.0f, 15.0f };
        auto B = ImGui::GetWindowPos() + ImVec2{ 5.0f, 15.0f };
        auto C = B + ImVec2{ 0, TimelineTheme.height };

        int stride_ = stride * 5;  // How many frames to skip drawing
        float zoom_ = zoom / stride;
        int minTime = range.x() / stride_;
        int maxTime = range.y() / stride_;

        auto drawTimelineBackground = [&]() {
            auto bottomRight = ImVec2(B.x + windowSize.x, B.y + TimelineTheme.height);
            painter->AddRectFilled(
                B,
                bottomRight,
                TimelineTheme.background
            );
        };

        auto drawEditorBackground = [&]() {
            auto bottomRight = ImVec2(C.x + windowSize.x, C.y + windowSize.y);
            painter->AddRectFilled(
                C,
                bottomRight,
                EditorTheme.background
            );
        };

        auto drawTimeline = [&]() {
            for (int time = minTime; time < maxTime + 1; time++) {
                const auto xMin = B.x + time * zoom_;
                const auto yMin = B.y;
                const auto yMax = B.y + TimelineTheme.height;

                painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), TimelineTheme.dark);
                painter->AddText(
                    ImGui::GetFont(),
                    ImGui::GetFontSize() * 0.85f,
                    ImVec2(xMin + 5.0f, yMin + lineHeight),
                    TimelineTheme.text,
                    std::to_string(time * stride_).c_str()
                );

                if (time == maxTime) break;

                for (int z = 0; z < 5 - 1; z++) {
                    const auto innerSpacing = zoom_ / 5;
                    auto subline = innerSpacing * (z + 1);
                    painter->AddLine(ImVec2(xMin + subline, yMin + 35), ImVec2(xMin + subline, yMax), TimelineTheme.mid);
                }
            }
        };

        auto drawHorizontalBar = [&]() {
            const auto xMin = B.x;
            const auto xMax = B.x + windowSize.x;
            const auto yMin = B.y + TimelineTheme.height;
            const auto yMax = B.y + TimelineTheme.height;

            painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMax, yMax), EditorTheme.dark, 1.0f);
        };

        auto drawVerticalGrid = [&]() {
            for (int time = minTime; time < maxTime + 1; time++) {
                const auto xMin = C.x + time * zoom_;
                const auto yMin = C.y;
                const auto yMax = C.y + windowSize.y;

                painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), EditorTheme.dark);

                if (time == maxTime) break;

                for (int z = 0; z < 5 - 1; z++) {
                    const auto innerSpacing = zoom_ / 5;
                    auto subline = innerSpacing * (z + 1);
                    painter->AddLine(ImVec2(xMin + subline, yMin), ImVec2(xMin + subline, yMax), EditorTheme.mid);
                }
            }
        };

        auto drawCurrentTime = [&]() {
            const auto xMin = B.x + currentTime * zoom_ / stride_;
            const auto yMin = B.y;
            const auto yMax = yMin + windowSize.y;
            painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), EditorTheme.accent, 2.0f);

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
            float yMin = C.y;
            float yMax = C.y + windowSize.y;
            float xMin = C.x;
            float xMax = C.x + windowSize.x;

            bool isOdd = false;
            for (float y = yMin; y < yMax; y += lineHeight) {
                isOdd ^= true;

                if (isOdd) painter->AddRectFilled(
                    { xMin, y },
                    { xMax, y + lineHeight - 1 },
                    EditorTheme.alternate
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

            Registry.view<Events, Color>().each([&](auto& events, const auto& color) {
                for (std::size_t i=0; i < events.size(); ++i) {
                    auto& event = events.at(i);

                    int startTime = event.time;
                    int endTime = startTime + event.length;

                    float xMin = static_cast<float>(startTime) * zoom_ / stride_;
                    float xMax = static_cast<float>(endTime) * zoom_ / stride_;
                    float yMin = 0;
                    float yMax = lineHeight;

                    xMin += C.x;
                    xMax += C.x;
                    yMin += C.y + (lineHeight * yOffset);
                    yMax += C.y + (lineHeight * yOffset);

                    painter->AddRectFilled({ xMin, yMin }, { xMax, yMax - 1 }, color.fill, EditorTheme.radius);
                    painter->AddRect({ xMin, yMin }, { xMax, yMax - 1 }, color.stroke, EditorTheme.radius);
                }

                yOffset += 1;
            });
        };

        drawTimelineBackground();
        drawEditorBackground();
        drawTimeline();
        drawHorizontalGrid();
        drawHorizontalBar();
        drawVerticalGrid();
        drawEvents();
        drawCurrentTime();
    }
    ImGui::End();
}


class Application : public Platform::Application {
public:
    explicit Application(const Arguments& arguments);

    void drawEvent() override;
    void drawShapes();
    void drawThemeEditor();
    void drawTransport();
    void drawTimeline();
    void drawCentralWidget();

    void setup();
    void clear();
    void updatePositions();

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

    Registry.assign<Name>(red, "Red");
    Registry.assign<Size>(red, Vector2i{ 100, 100 });
    Registry.assign<Color>(red, ImColor::HSV(0.0f, 0.75f, 0.75f), ImColor::HSV(0.0f, 0.75f, 0.1f));
    Registry.assign<Position>(red, Vector2i{ 300, 50 });
    Registry.assign<InitialPosition>(red, Vector2i{ 300, 50 });
    Registry.assign<Events>(red);
    Registry.assign<Channel>(red);
    Registry.assign<SmartButtonState>(red);

    Registry.assign<Name>(green, "green");
    Registry.assign<Size>(green, Vector2i{ 100, 100 });
    Registry.assign<Color>(green, ImColor::HSV(0.33f, 0.75f, 0.75f), ImColor::HSV(0.33f, 0.75f, 0.1f));
    Registry.assign<Position>(green, Vector2i{ 500, 50 });
    Registry.assign<InitialPosition>(green, Vector2i{ 500, 50 });
    Registry.assign<Events>(green);
    Registry.assign<Channel>(green);
    Registry.assign<SmartButtonState>(green);

    Registry.assign<Name>(blue, "blue");
    Registry.assign<Size>(blue, Vector2i{ 100, 100 });
    Registry.assign<Color>(blue, ImColor::HSV(0.55f, 0.75f, 0.75f), ImColor::HSV(0.55f, 0.75f, 0.1f));
    Registry.assign<Position>(blue, Vector2i{ 700, 50 });
    Registry.assign<InitialPosition>(blue, Vector2i{ 700, 50 });
    Registry.assign<Events>(blue);
    Registry.assign<Channel>(blue);
    Registry.assign<SmartButtonState>(blue);
}


auto Application::dpiScaling() const -> Vector2 { return _dpiScaling; }


void Application::clear() {
    Registry.view<Channel, Events>().each([](auto& channel, auto& events) {
        channel.clear();
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
        if (ImGui::Button("<")) _sequencer.currentTime -= 1;
        ImGui::SameLine();
        if (ImGui::Button(">")) _sequencer.currentTime += 1;

        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            _sequencer.currentTime = _sequencer.range.x();
            _playing = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            clear();
        }

        ImGui::DragInt("Time", &_sequencer.currentTime);
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
        ImGui::SliderInt("Stride", &_sequencer.stride, 1, 5);
    }
    ImGui::End();
}


void Application::drawShapes() {
    ImVec2 size = ImGui::GetWindowSize();
    Vector2i shapeSize { 100, 100 };

    auto drawCross = [&](ImVec2 corner, ImColor color) {
        auto root = ImGui::GetWindowPos();
        auto painter = ImGui::GetWindowDrawList();
        auto size = ImVec2((float)shapeSize.x(), (float)shapeSize.y());

        auto abscorner = root + corner;
        auto hlineStart = ImVec2(abscorner.x, abscorner.y + size.y / 2);
        auto hlineEnd   = ImVec2(hlineStart.x + size.x, hlineStart.y);
        auto vlineStart = ImVec2(abscorner.x + size.x / 2, abscorner.y);
        auto vlineEnd   = ImVec2(vlineStart.x, vlineStart.y + size.y);

        painter->AddCircleFilled(
            abscorner,
            5.0f,
            ImColor::HSV(0.0, 0.0, 1.0f)
        );
    };

    auto time = _sequencer.currentTime;
    ImGui::Begin("Shapes", nullptr);
    {
        Registry.view<Name, Position, Size, Color, Events, Channel, SmartButtonState>().each([&drawCross, time](
                                                                    auto entity,
                                                                    const auto& name,
                                                                    const auto& position,
                                                                    const auto& size,
                                                                    const auto& color,
                                                                    auto& events,
                                                                    auto& channel,
                                                                    auto& state) {
            static Event* current { nullptr };

            auto imsize = ImVec2((float)size.x(), (float)size.y());
            auto impos = ImVec2((float)position.x(), (float)position.y());
            auto imdelta = Vector2i(Vector2(ImGui::GetIO().MouseDelta));

            ImGui::SetCursorPos(impos);
            SmartButton(name.text, state, imsize);

            if (state & SmartButtonState_Pressed) {
                events.push_back({ Event::Red, time, 1 });
                current = &events.back();
            }

            else if (state & SmartButtonState_Dragged) {
                if (current != nullptr && current->time + current->length > time) {
                    current = nullptr;
                }

                if (current != nullptr) {
                    channel[time] = imdelta;
                    current->length += 1;
                }
            }

            else if (state & SmartButtonState_Released) {
                current = nullptr;
            }

            if (channel.count(time)) {
                drawCross(impos, color.fill);
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
            ImGui::ColorEdit4("background", &TimelineTheme.background.Value.x);
            ImGui::ColorEdit4("text", &TimelineTheme.text.Value.x);
            ImGui::ColorEdit4("dark", &TimelineTheme.dark.Value.x);
            ImGui::ColorEdit4("mid", &TimelineTheme.mid.Value.x);
        }

        if (ImGui::CollapsingHeader("Editor", &open)) {
            ImGui::ColorEdit4("background##editor", &EditorTheme.background.Value.x);
            ImGui::ColorEdit4("alternate##editor", &EditorTheme.alternate.Value.x);
            ImGui::ColorEdit4("text##editor", &EditorTheme.text.Value.x);
            ImGui::ColorEdit4("mid##editor", &EditorTheme.mid.Value.x);
            ImGui::ColorEdit4("dark##editor", &EditorTheme.dark.Value.x);
            ImGui::ColorEdit4("accent##editor", &EditorTheme.accent.Value.x);
        }
    }
    ImGui::End();
}


void Application::updatePositions() {
    auto startTime = _sequencer.range.x();
    auto currentTime = _sequencer.currentTime;

    if (currentTime == startTime) {
        Registry.view<Position, InitialPosition>().each([&](auto& position, const auto& initial) {
            position = initial;
        });
    }
    else {
        Registry.view<Position, Channel>().each([&](auto& position, const auto& channel) {
            if (!channel.count(currentTime)) return;
            position += channel.at(currentTime);
        });
    }
}


void Application::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _imgui.newFrame();

    if (_playing) {
        _sequencer.currentTime += 1;
    }

    if (_sequencer.currentTime > _sequencer.range.y()) {
        _sequencer.currentTime = _sequencer.range.x();
    }

    if (ImGui::GetIO().WantTextInput && !isTextInputActive()) startTextInput();
    else if (!ImGui::GetIO().WantTextInput && isTextInputActive()) stopTextInput();

    drawCentralWidget();
    drawTransport();
    drawShapes();
    _sequencer.draw();

    updatePositions();

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