/**

Example usage of Sequentity.inl

*/

// (Optional) Magnum prefers to have its imgui.h included first
#include <Magnum/ImGuiIntegration/Context.hpp>

// Sequentity is a single-file library, which means .h and .cpp files are combined
// This define causes the .cpp portion of the library to be included and only needs
// to exist (before the #include) in *one* of your translation units.
#define SEQUENTITY_IMPLEMENTATION

#include <Sequentity.h>

#include <string>
#include <iostream>
#include <unordered_map>

#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector.h>
#include <Magnum/Magnum.h>

#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Platform/GlfwApplication.h>

#include <imgui_internal.h>
#include <entt/entity/registry.hpp>

using namespace Magnum;
using namespace Math::Literals;

static entt::registry Registry;
static entt::registry Events;

// For readability only; this really is just one big cpp file
#include "Utils.inl"
#include "Theme.inl"
#include "Components.inl"
#include "Widgets.inl"
#include "IntentSystem.inl"
#include "Tools.inl"

using ToolContextRef = std::unique_ptr<ToolContext>;

class Application : public Platform::Application {
public:
    explicit Application(const Arguments& arguments);

    void drawEvent() override;
    void drawScene();
    void drawTransport();
    void drawCentralWidget();
    void drawEventEditor();

    void play();
    void step(int time);
    void stop();

    void setCurrentTool(ToolType type);

    void setup();  // Populate registry with entities and components
    void reset();  // Reset all entities
    void clear();  // Clear all events

private:
    void onTimeChanged(); // Apply active events from Sequentity
    void onNewTrack();
    void pollGamepad();

    auto dpiScaling() const -> Vector2;
    void viewportEvent(ViewportEvent& event) override;

    // Sequentity event handlers
    void onToolEvent(entt::entity entity, const Sequentity::Event& event, int time);
    void onTranslateEvent(entt::entity entity, const Sequentity::Event& event, int time);
    void onRotateEvent(entt::entity entity, const Sequentity::Event& event, int time);
    void onScaleEvent(entt::entity entity, const Sequentity::Event& event, int time);

    void keyPressEvent(KeyEvent& event) override;
    void keyReleaseEvent(KeyEvent& event) override;

    void mousePressEvent(MouseEvent& event) override;
    void mouseReleaseEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void mouseScrollEvent(MouseScrollEvent& event) override;
    void textInputEvent(TextInputEvent& event) override;

    ImGuiIntegration::Context _imgui{ NoCreate };

    Vector2 _dpiScaling { 1.0f, 1.0f };

    bool _playing { false };
    bool _recording { false };

    // Debugging, for control over the event loop
    bool _running { true };

    // Track in order to determine whether time has changed
    int _previousTime { 0 };
    Vector2i _mousePressPosition { 0, 0 };

    ToolContextRef _toolContext;
    ToolType _currentToolType { ToolType::Translate };
    ToolType _previousToolType { ToolType::Translate };

    std::unordered_map<ToolType, ToolContextRef> _allTools;

    bool _showSequencer { true };
    bool _showMetrics { false };
    bool _showStyleEditor { false };

    // Tool contexts are relative the currently active panel.
    // E.g. when working in the 3d view, a different set of tools
    // are made available, with one being "current".
    struct ApplicationContext {
        ToolType currentTool { ToolType::Select };
    } _applicationContext;

    struct SceneContext {
        ToolType currentTool { ToolType::Translate };
    } _sceneContext;

    struct EditorContext {
        ToolType currentTool { ToolType::Select };
    } _editorContext;

    std::vector<entt::entity> _entities;
};


static void on_select_constructed(entt::entity entity, entt::registry& registry) {
    registry.assign<Sequentity::Selected>(entity);
}

static void on_select_destroyed(entt::entity entity, entt::registry& registry) {
    registry.remove<Sequentity::Selected>(entity);
}

static void on_position_constructed(entt::entity entity, entt::registry& registry, const Position& position) {
    registry.assign<InitialPosition>(entity, position);
}

static void on_size_constructed(entt::entity entity, entt::registry& registry, const Size& size) {
    registry.assign<InitialSize>(entity, size);
}


Application::Application(const Arguments& arguments): Platform::Application{arguments,
    Configuration{}.setTitle("Sequentity Example Application")
                   .setSize({1600, 900})
                   .setWindowFlags(Configuration::WindowFlag::Resizable)}
{
    // Use virtual scale, rather than physical
    glfwGetWindowContentScale(this->window(), &_dpiScaling.x(), &_dpiScaling.y());

    // Center window on primary monitor
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(this->window(),
        (mode->width / 2) - (windowSize().x() / 2),
        (mode->height / 2) - (windowSize().y() / 2)
    );

    _imgui = ImGuiIntegration::Context(
        Vector2{ windowSize() } / dpiScaling(),
        windowSize(), framebufferSize()
    );

    ImGui::GetIO().Fonts->Clear();
    ImGui::GetIO().Fonts->AddFontFromFileTTF("OpenSans-Regular.ttf", 16.0f * dpiScaling().x());

    // Refresh fonts
    _imgui.relayout(
        Vector2{ windowSize() } / dpiScaling(),
        windowSize(), framebufferSize()
    );

    // Required, else you can't interact with events in the editor
    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

    // Optional
    ImGui::GetIO().ConfigWindowsResizeFromEdges = true;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
    ImGui::GetIO().ConfigDockingWithShift = true;

    Theme();

    /* Set up proper blending to be used by ImGui */
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
                                   GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
                                   GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    this->setSwapInterval(1);  // VSync

    // Synchronise Sequentity with our internal application selection
    Registry.on_construct<Selected>().connect<on_select_constructed>();
    Registry.on_destroy<Selected>().connect<on_select_destroyed>();

    // AQ4: Keep track of initial values
    Registry.on_construct<Position>().connect<on_position_constructed>();
    Registry.on_construct<Size>().connect<on_size_constructed>();

    // Keep tracks sorted in the order of our application Index component
    // E.g. in the order of your 3D character hierarchy
    // Events.on_construct<Sequentity::Track>().connect<&Application::onNewTrack>(*this);

    // Initialise internal sqty
    Events.set<Sequentity::State>();

    Events.create();  // Reserve ID 0

    setCurrentTool(ToolType::Translate);

    _allTools[ToolType::Select] = std::make_unique<SelectContext>();
    _allTools[ToolType::Translate] = std::make_unique<TranslateContext>();
    _allTools[ToolType::Rotate] = std::make_unique<RotateContext>();
    _allTools[ToolType::Scale] = std::make_unique<ScaleContext>();
    _allTools[ToolType::Scrub] = std::make_unique<ScrubContext>();

    setup();
    play();
}


void Application::onNewTrack() {
    // Events.sort<Sequentity::Track>([this](const entt::entity lhs, const entt::entity rhs) {
    //     return Events.get<Index>(lhs) < Events.get<Index>(rhs);
    // });
}


/**
 * @brief Setup scene data
 *
 * This would typically come off of disk
 *
 */
void Application::setup() {
    auto global = Registry.create();
    Registry.assign<Name>(global, "Global");
    Registry.assign<Index>(global, 0);
    Registry.set<entt::entity>(global);

    auto red = Registry.create();
    auto green = Registry.create();
    auto blue = Registry.create();
    auto purple = Registry.create();
    auto gray = Registry.create();

    Registry.assign<Name>(red, "hip");
    Registry.assign<Index>(red, 1);
    Registry.assign<Size>(red, 100, 100);
    Registry.assign<Color>(red, ImColor::HSV(0.0f, 0.75f, 0.75f));
    Registry.assign<Orientation>(red, 0.0_degf);
    Registry.assign<Position>(red, 300, 50 + 50);

    Registry.assign<Name>(green, "leftLeg");
    Registry.assign<Index>(green, 2);
    Registry.assign<Size>(green, 100, 100);
    Registry.assign<Color>(green, ImColor::HSV(0.33f, 0.75f, 0.75f));
    Registry.assign<Orientation>(green, 0.0_degf);
    Registry.assign<Position>(green, 500, 50 + 50);

    Registry.assign<Name>(blue, "foot");
    Registry.assign<Index>(blue, 3);
    Registry.assign<Size>(blue, 100, 100);
    Registry.assign<Color>(blue, ImColor::HSV(0.55f, 0.75f, 0.75f));
    Registry.assign<Orientation>(blue, 0.0_degf);
    Registry.assign<Position>(blue, 700, 50 + 50);

    Registry.assign<Name>(purple, "leftShoulder");
    Registry.assign<Index>(purple, 4);
    Registry.assign<Size>(purple, 80, 100);
    Registry.assign<Color>(purple, ImColor::HSV(0.45f, 0.75f, 0.75f));
    Registry.assign<Orientation>(purple, 0.0_degf);
    Registry.assign<Position>(purple, 350, 200 + 50);

    Registry.assign<Name>(gray, "head");
    Registry.assign<Index>(gray, 5);
    Registry.assign<Size>(gray, 80, 40);
    Registry.assign<Color>(gray, ImColor::HSV(0.55f, 0.0f, 0.55f));
    Registry.assign<Orientation>(gray, 0.0_degf);
    Registry.assign<Position>(gray, 550, 200 + 50);

    _entities.push_back(red);
    _entities.push_back(green);
    _entities.push_back(blue);
    _entities.push_back(purple);
    _entities.push_back(gray);
}


void Application::play() {
    if (!_playing) {
        stop();
        reset();
    }

    _playing ^= true;
}


void Application::step(int delta) {
    auto& sqty = Events.ctx_or_set<Sequentity::State>();
    auto time = sqty.current_time + delta;

    if (time > sqty.range[1]) {
        time = sqty.range[0];
    }

    else if (time < sqty.range[0]) {
        time = sqty.range[1];
    }

    sqty.current_time = time;
}


void Application::stop() {
    auto& sqty = Events.ctx_or_set<Sequentity::State>();

    sqty.current_time = sqty.range[0];
    _playing = false;
}


void Application::onTimeChanged() {
    auto& sqty = Events.ctx<Sequentity::State>();
    auto startTime = sqty.range[0];
    auto current_time = sqty.current_time;

    if (current_time <= startTime) {
        reset();
    }

    else {
        Sequentity::Intersect(Events, current_time, [&](const entt::entity subject, const entt::entity event) {
            auto& data = Events.get<ToolEventData>(event);
            auto time = Events.get<Sequentity::Event>(event).time;
            const auto index = current_time + (data.start - time);

            if (data.input.count(index)) {
                auto& tool = _allTools.at(data.type);
                auto& input = data.input[index];
                tool->update(subject, input);
            }
        });
    }
}

auto Application::dpiScaling() const -> Vector2 { return _dpiScaling; }


void Application::reset() {
    // TODO: Reset to first frame of each event, rather than this additional "Initial" component
    Registry.view<Position, InitialPosition>().each([&](auto& position, const auto& initial) {
        position = initial;
    });

    Registry.view<Orientation>().each([&](auto& orientation) {
        orientation = 0.0f;
    });

    Registry.view<Size, InitialSize>().each([&](auto& size, const auto& initial) {
        size = initial;
    });

    _toolContext->abort();
}


void Application::clear() {

    Events.reset();

    // TODO: This is much too explicit and error prone. Is there a better
    //       way to assert that when a channel goes away, so does the data?
    int deletedCount { 0 };
    Events.view<Sequentity::Track>().each([&deletedCount](auto& track) {
        for (auto& [type, channel] : track.channels) {
            for (auto& event : channel.events) {
                if (type == TranslateEvent ||
                    type == RotateEvent ||
                    type == ScaleEvent)
                {
                    delete static_cast<ToolEventData*>(event.data);
                    deletedCount++;
                }

                else if (type == EventType::MousePressEvent || type == EventType::MouseMoveEvent || type == EventType::MouseReleaseEvent) {}

                else {
                    Warning() << "Unknown event type" << event.type << "memory has leaked!";
                }
            }
        }
    });

    Events.reset<Sequentity::Track>();

    reset();

    if (deletedCount) Debug() << "Deleted" << deletedCount << "events";
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
    auto& sqty = Events.ctx<Sequentity::State>();

    ImGui::Begin("Transport", nullptr);
    {
        if (ImGui::Button("Play")) this->play(); ImGui::SameLine();
        if (ImGui::Button("Record")) _recording ^= true; ImGui::SameLine();
        if (ImGui::Button("<")) this->step(-1); ImGui::SameLine();
        if (ImGui::Button(">")) this->step(1); ImGui::SameLine();
        if (ImGui::Button("Stop")) this->stop(); ImGui::SameLine();
        if (ImGui::Button("Clear")) clear();

        ImGui::DragInt("Time", &sqty.current_time, 1.0f, sqty.range[0], sqty.range[1]);

        if (ImGui::DragInt2("Range", sqty.range)) {
            if (sqty.range[0] < 0) sqty.range[0] = 0;
            if (sqty.range[1] < 5) sqty.range[1] = 5;

            if (sqty.current_time < sqty.range[0]) {
                sqty.current_time = sqty.range[0];
            }

            if (sqty.current_time > sqty.range[1]) {
                sqty.current_time = sqty.range[1];
            }
        }

        ImGui::SetNextItemWidth(70.0f);
        ImGui::SliderFloat("##zoom", &sqty.target_zoom[0], 50.0f, 400.0f, "%.3f", 2.0f); ImGui::SameLine();
        ImGui::SetNextItemWidth(70.0f);
        ImGui::SliderFloat("Zoom", &sqty.target_zoom[1], 20.0f, 400.0f, "%.3f", 3.0f);
        ImGui::DragFloat2("Pan", sqty.target_pan);
        ImGui::SliderInt("Stride", &sqty.stride, 1, 5);
    }
    ImGui::End();
}


void Application::setCurrentTool(ToolType type) {
    _previousToolType = _currentToolType;
    _currentToolType = type;

    if (type == ToolType::Select)    _toolContext = std::make_unique<SelectContext>();
    if (type == ToolType::Scrub)     _toolContext = std::make_unique<ScrubContext>();
    if (type == ToolType::Translate) _toolContext = std::make_unique<TranslateContext>();
    if (type == ToolType::Rotate)    _toolContext = std::make_unique<RotateContext>();
    if (type == ToolType::Scale)     _toolContext = std::make_unique<ScaleContext>();
}


void Application::drawScene() {
    auto& sqty = Events.ctx<Sequentity::State>();
    static bool windowEntered { false };

    ImGui::Begin("3D Viewport", nullptr);
    {
        // Determine whether cursor entered or exited the 3d scene view
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
            if (!windowEntered) {
                Debug() << "Scene is entered";
                setCurrentTool(_sceneContext.currentTool);
                windowEntered = true;
            }
        } else {
            if (windowEntered) {
                Debug() << "Scene is exited";
                windowEntered = false;
            }
        }

        if (Widgets::Button("Select (Q)", _currentToolType == ToolType::Select)) {
            _sceneContext.currentTool = ToolType::Select;
            setCurrentTool(ToolType::Select);
        }

        if (Widgets::Button("Translate (W)", _currentToolType == ToolType::Translate)) {
            _sceneContext.currentTool = ToolType::Translate;
            setCurrentTool(ToolType::Translate);
        }

        if (Widgets::Button("Rotate (E)", _currentToolType == ToolType::Rotate)) {
            _sceneContext.currentTool = ToolType::Rotate;
            setCurrentTool(ToolType::Rotate);
        }

        if (Widgets::Button("Scale (R)", _currentToolType == ToolType::Scale)) {
            _sceneContext.currentTool = ToolType::Scale;
            setCurrentTool(ToolType::Scale);
        }

        if (Widgets::Button("Scrub (K)", _currentToolType == ToolType::Scrub)) {}
        if (Widgets::RecordButton("Record (T)", _recording)) _recording ^= true;

        auto dpos = Vector2i(Vector2(ImGui::GetIO().MouseDelta));
        auto rpos = Vector2i(Vector2(ImGui::GetMouseDragDelta(0, 0.0f)));
        auto apos = Vector2i(Vector2(ImGui::GetIO().MousePos.x - ImGui::GetWindowPos().x,
                                     ImGui::GetIO().MousePos.y - ImGui::GetWindowPos().y));
        Position deltaPosition { dpos.x(), dpos.y() };
        Position relativePosition { rpos.x(), rpos.y() };
        Position absolutePosition { apos.x(), apos.y() };

        Registry.view<Name, Position, Orientation, Color, Size>().each([&](auto entity,
                                                                           const auto& name,
                                                                           const auto& position,
                                                                           const auto& orientation,
                                                                           const auto& color,
                                                                           const auto& size) {
            auto imsize = ImVec2((float)size.x, (float)size.y);
            auto impos = ImVec2((float)position.x, (float)position.y);
            auto imangle = static_cast<float>(orientation);
            auto imcolor = ImColor(color);

            auto time = sqty.current_time + (_playing ? 1 : 0);
            bool selected = Registry.has<Selected>(entity);
            Widgets::Graphic(name.text, impos, imsize, imangle, imcolor, selected);

            if (ImGui::IsItemHovered()) {
                Registry.assign<Hovered>(entity);
            }

            if (Registry.has<Tooltip>(entity)) {
                ImGui::BeginTooltip();
                ImGui::SetTooltip(Registry.get<Tooltip>(entity).text);
                ImGui::EndTooltip();
            }
        });

        // Registry.view<Position, Sequentity::Track, Color>().each([&](const auto& position,
        //                                                              const auto& track,
        //                                                              const auto& color) {
        //     Sequentity::Intersect(track, sqty.current_time, [&](auto& event) {
        //         if (event.type == TranslateEvent) {
        //             auto data = static_cast<ToolEventData*>(event.data);
        //             auto input = data->input[data->start];
        //             Position pos = position + (input.absolute - data->origin);
        //             auto impos = ImVec2(Vector2(Vector2i(pos.x, pos.y)));
        //             Widgets::Cursor(impos, color);
        //         }
        //     });
        // });
    }

    ImGui::End();
}


void Application::drawEventEditor() {
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar
                                 | ImGuiWindowFlags_NoScrollWithMouse;

    static bool windowEntered { false };
    ImGui::Begin("Event Editor", &_showSequencer, windowFlags);
    {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
            if (!windowEntered) {
                Debug() << "Event Editor is entered";
                setCurrentTool(_editorContext.currentTool);
                windowEntered = true;
            }
        } else {
            if (windowEntered) {
                Debug() << "Event Editor is exited";
                windowEntered = false;
            }
        }

        Sequentity::EventEditor(Events);
    }
    ImGui::End();
}


void Application::pollGamepad() {
    const auto& sqty = Events.ctx<Sequentity::State>();

    // Work around the fact that we're working in absolute coords for position
    static std::unordered_map<entt::entity, Position> start_pos;

    // Derive activation from changes to button presses
    static std::unordered_map<int, bool> is_down {
        { GLFW_GAMEPAD_BUTTON_A, false },
        { GLFW_GAMEPAD_BUTTON_B, false },
        { GLFW_GAMEPAD_BUTTON_X, false },
        { GLFW_GAMEPAD_BUTTON_Y, false }
    };

    GLFWgamepadstate gamepad;
    auto poll_button = [&](int button, entt::entity entity) {
        if (gamepad.buttons[button]) {
            const float right_trigger = gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
            const float left_x = gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
            const float left_y = gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];

            const Position pos {
                static_cast<int>(left_x * 100.0f),
                static_cast<int>(left_y * 100.0f),
            };

            if (!is_down[button]) {
                start_pos[entity] = Registry.get<Position>(entity);
                is_down[button] = true;
            }

            else {
            }
        }

        else if (is_down[button]) {
            is_down[button] = false;
        }
    };

    // Hardcode each button to one entity each
    if (glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepad)) {
        poll_button(GLFW_GAMEPAD_BUTTON_A, _entities[0]);
        poll_button(GLFW_GAMEPAD_BUTTON_B, _entities[1]);
        poll_button(GLFW_GAMEPAD_BUTTON_X, _entities[2]);
        poll_button(GLFW_GAMEPAD_BUTTON_Y, _entities[3]);
    }
}


void Application::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _imgui.newFrame();

    // Erase all current inputs
    Registry.reset<InputPosition2D>();
    Registry.reset<InputPosition3D>();

    // Restore order to this world
    Registry.reset<Hovered>();

         if ( ImGui::GetIO().WantTextInput && !isTextInputActive()) startTextInput();
    else if (!ImGui::GetIO().WantTextInput &&  isTextInputActive()) stopTextInput();

    IntentSystem();

    drawCentralWidget();
    drawTransport();
    drawScene();

    pollGamepad();

    auto& sqty = Events.ctx_or_set<Sequentity::State>();

    if (_playing) step(1);

    // current_time is *mutable* and can change from anywhere
    // Thus, we compare it against the previous time to
    // determine whether or not it has changed.
    if (sqty.current_time != _previousTime) {
        onTimeChanged();
        _previousTime = sqty.current_time;
    }

    drawEventEditor();


    if (_showMetrics) {
        ImGui::ShowMetricsWindow(&_showMetrics);
    }

    if (_showStyleEditor) {
        Sequentity::ThemeEditor(&_showStyleEditor);
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
    if (event.key() == KeyEvent::Key::Esc)          this->exit();
    if (event.key() == KeyEvent::Key::Enter)        redraw();
    if (event.key() == KeyEvent::Key::Delete)       clear();
    if (event.key() == KeyEvent::Key::Backspace)    { _running ^= true; if (_running) redraw(); }
    if (event.key() == KeyEvent::Key::Space)        { this->play(); }
    if (event.key() == KeyEvent::Key::F1)           _showMetrics ^= true;
    if (event.key() == KeyEvent::Key::F2)           _showStyleEditor ^= true;
    if (event.key() == KeyEvent::Key::F5)           _showSequencer ^= true;

    if (event.key() == KeyEvent::Key::K && !event.isRepeated()) setCurrentTool(ToolType::Scrub);
    if (event.key() == KeyEvent::Key::Q)                        setCurrentTool(ToolType::Select);
    if (event.key() == KeyEvent::Key::W)                        setCurrentTool(ToolType::Translate);
    if (event.key() == KeyEvent::Key::E)                        setCurrentTool(ToolType::Rotate);
    if (event.key() == KeyEvent::Key::R)                        setCurrentTool(ToolType::Scale);
    if (event.key() == KeyEvent::Key::T)                        _recording ^= true;

    if(_imgui.handleKeyPressEvent(event)) return;
}

void Application::keyReleaseEvent(KeyEvent& event) {
    if (event.key() == KeyEvent::Key::K) {
        setCurrentTool(_previousToolType);
    }

    if(_imgui.handleKeyReleaseEvent(event)) return;
}


void Application::mousePressEvent(MouseEvent& event) {
    auto& sqty = Events.ctx<Sequentity::State>();

    Registry.view<Hovered>().each([&](auto entity, const auto) {
        auto absolute = event.position() / dpiScaling();

        InputPosition2D input;
        input.absolute = { absolute.x(), absolute.y() };

        _toolContext->begin(entity, input);

        if (_recording) {
            record_tool(*_toolContext.get(), sqty.current_time);
        }
    });

    _mousePressPosition = event.position();
    if (_imgui.handleMousePressEvent(event)) return;
}


void Application::mouseMoveEvent(MouseMoveEvent& event) {
    auto delta = event.relativePosition() / dpiScaling();
    auto absolute = event.position() / dpiScaling();
    auto relative = (event.position() - _mousePressPosition) / dpiScaling();

    InputPosition2D input;
    input.absolute = { absolute.x(), absolute.y() };
    input.relative = { relative.x(), relative.y() };
    input.delta = { delta.x(), delta.y() };

    if (_toolContext->update(input) && _recording) {
        auto& sqty = Events.ctx<Sequentity::State>();

        if (sqty.current_time > sqty.range[1] - 1) {
            _toolContext->abort();
        }
        else {
            record_tool(*_toolContext.get(), sqty.current_time);
        }
    }

    if (_imgui.handleMouseMoveEvent(event)) return;
}

void Application::mouseReleaseEvent(MouseEvent& event) {
    _toolContext->finish();

    if (_imgui.handleMouseReleaseEvent(event)) return;
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