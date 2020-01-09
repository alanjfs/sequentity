/**

Example usage of Sequentity.inl

*/

#include <string>
#include <iostream>
#include <unordered_map>

#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector.h>
#include <Magnum/Magnum.h>

#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Platform/GlfwApplication.h>

#include <imgui_internal.h>
#include <entt/entity/registry.hpp>

using namespace Magnum;
using namespace Math::Literals;

static entt::registry Registry;

#include "Utils.hpp"
#include "Theme.inl"
#include "Components.inl"
#include "Widgets.inl"
#include "Sequentity.inl"
#include "Tools.inl"


class Application : public Platform::Application {
public:
    explicit Application(const Arguments& arguments);

    void drawEvent() override;
    void drawScene();
    void drawTransport();
    void drawCentralWidget();

    void play();
    void step(int time);
    void stop();

    void setup();  // Populate registry with entities and components
    void reset();  // Reset all entities
    void clear();  // Clear all events

    void onTimeChanged(); // Apply active events from Sequentity
    void onNewTrack();

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
    Sequentity::Sequentity _sequentity { Registry };

    Vector2 _dpiScaling { 1.0f, 1.0f };

    bool _playing { false };
    bool _running { true };
    int _previous_time { 0 };

    Tool _activeTool { ToolType::Translate, TranslateTool };
    Tool _previousTool { ToolType::Translate, TranslateTool };

    bool _showSequencer { true };
    bool _showMetrics { false };
    bool _showStyleEditor { false };
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
    Configuration{}.setTitle("Application")
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

    /* Set up proper blending to be used by ImGui. There's a great chance
       you'll need this exact behavior for the rest of your scene. If not, set
       this only for the drawFrame() call. */
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
    Registry.on_construct<Sequentity::Track>().connect<&Application::onNewTrack>(*this);

    // Initialise internal state
    Registry.set<Sequentity::State>();

    setup();
    play();
}


void Application::onNewTrack() {
    Registry.sort<Sequentity::Track>([this](const entt::entity lhs, const entt::entity rhs) {
        return Registry.get<Index>(lhs) < Registry.get<Index>(rhs);
    });
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

    Registry.assign<Name>(red, "Red Rectangle");
    Registry.assign<Index>(red, 1);
    Registry.assign<Size>(red, 100, 100);
    Registry.assign<Color>(red, ImColor::HSV(0.0f, 0.75f, 0.75f));
    Registry.assign<Orientation>(red, 0.0_degf);
    Registry.assign<Position>(red, 300, 50 + 50);

    Registry.assign<Name>(green, "Green Rectangle");
    Registry.assign<Index>(green, 2);
    Registry.assign<Size>(green, 100, 100);
    Registry.assign<Color>(green, ImColor::HSV(0.33f, 0.75f, 0.75f));
    Registry.assign<Orientation>(green, 0.0_degf);
    Registry.assign<Position>(green, 500, 50 + 50);

    Registry.assign<Name>(blue, "Blue Rectangle");
    Registry.assign<Index>(blue, 3);
    Registry.assign<Size>(blue, 100, 100);
    Registry.assign<Color>(blue, ImColor::HSV(0.55f, 0.75f, 0.75f));
    Registry.assign<Orientation>(blue, 0.0_degf);
    Registry.assign<Position>(blue, 700, 50 + 50);

    Registry.assign<Name>(purple, "Purple Rectangle");
    Registry.assign<Index>(purple, 4);
    Registry.assign<Size>(purple, 80, 100);
    Registry.assign<Color>(purple, ImColor::HSV(0.45f, 0.75f, 0.75f));
    Registry.assign<Orientation>(purple, 0.0_degf);
    Registry.assign<Position>(purple, 350, 200 + 50);

    Registry.assign<Name>(gray, "Gray Rectangle");
    Registry.assign<Index>(gray, 5);
    Registry.assign<Size>(gray, 80, 40);
    Registry.assign<Color>(gray, ImColor::HSV(0.55f, 0.0f, 0.55f));
    Registry.assign<Orientation>(gray, 0.0_degf);
    Registry.assign<Position>(gray, 550, 200 + 50);
}


void Application::play() {
    _playing ^= true;
}


void Application::step(int delta) {
    auto& state = Registry.ctx_or_set<Sequentity::State>();
    auto time = state.current_time + delta;

    if (time > state.range[1]) {
        time = state.range[0];
    }

    else if (time < state.range[0]) {
        time = state.range[1];
    }

    state.current_time = time;
}


void Application::stop() {
    auto& state = Registry.ctx_or_set<Sequentity::State>();

    state.current_time = state.range[0];
    _playing = false;
}


void Application::onTimeChanged() {
    auto& state = Registry.ctx<Sequentity::State>();
    auto startTime = state.range[0];
    auto current_time = state.current_time;

    if (!Registry.empty<Deactivated>()) {
        Registry.reset<Abort>();
    }

    if (current_time >= state.range[1]) {
        Registry.view<Active>().each([](auto entity, const auto) {
            Registry.assign_or_replace<Abort>(entity);
        });
    }

    if (current_time <= startTime) {
        reset();
    }

    else {
        Registry.view<Sequentity::Track>().each([&](auto entity, const auto& track) {
            Sequentity::Intersect(track, current_time, [&](auto& event) {
                if (event.data == nullptr) { Warning() << "This is a bug"; return; }

                if (event.type == TranslateEvent) {
                    // Guaranteed to exist by the tool, operating solely on entities that carry it
                    auto& position = Registry.get<Position>(entity);

                    auto data = static_cast<TranslateEventData*>(event.data);
                    const int index = current_time - event.time;
                    auto value = data->positions[index] - data->offset;
                    position = value;
                }

                else if (event.type == RotateEvent) {
                    auto& orientation = Registry.get<Orientation>(entity);
                    auto data = static_cast<RotateEventData*>(event.data);
                    const int index = current_time - event.time;
                    const auto value = data->orientations[index];
                    orientation = value;
                }

                else if (event.type == ScaleEvent) {
                    auto& [initial, size] = Registry.get<InitialSize, Size>(entity);
                    auto data = static_cast<ScaleEventData*>(event.data);
                    const int index = current_time - event.time;
                    const auto value = data->scales[index];
                    size.x = static_cast<int>(initial.x * value);
                    size.y = static_cast<int>(initial.y * value);
                }

                else {
                    Warning() << "Unknown event type!" << event.type;
                }
            });
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
}


void Application::clear() {

    // TODO: This is much too explicit and error prone. Is there a better
    //       way to assert that when a channel goes away, so does the data?
    int deletedCount { 0 };
    Registry.view<Sequentity::Track>().each([&deletedCount](auto& track) {
        for (auto& [type, channel] : track.channels) {
            for (auto& event : channel.events) {
                if (type == TranslateEvent) {
                    delete static_cast<TranslateEventData*>(event.data);
                    deletedCount++;
                }

                else if (type == RotateEvent) {
                    delete static_cast<RotateEventData*>(event.data);
                    deletedCount++;
                }

                else if (type == ScaleEvent) {
                    delete static_cast<ScaleEventData*>(event.data);
                    deletedCount++;
                }

                else {
                    Warning() << "Unknown event type" << event.type << "memory has leaked!";
                }
            }
        }
    });

    Registry.reset<Sequentity::Track>();

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
    auto& state = Registry.ctx<Sequentity::State>();

    ImGui::Begin("Transport", nullptr);
    {
        if (ImGui::Button("Play")) this->play();
        ImGui::SameLine();
        if (ImGui::Button("<")) this->step(-1);
        ImGui::SameLine();
        if (ImGui::Button(">")) this->step(1);

        ImGui::SameLine();
        if (ImGui::Button("Abort")) {
            this->stop();
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            clear();
        }

        ImGui::DragInt("Time", &state.current_time, 1.0f, state.range[0], state.range[1]);

        if (ImGui::DragInt2("Range", state.range)) {
            if (state.range[0] < 0) state.range[0] = 0;
            if (state.range[1] < 5) state.range[1] = 5;

            if (state.current_time < state.range[0]) {
                state.current_time = state.range[0];
            }

            if (state.current_time > state.range[1]) {
                state.current_time = state.range[1];
            }
        }

        ImGui::SetNextItemWidth(70.0f);
        ImGui::SliderFloat("##zoom", &state.target_zoom[0], 50.0f, 400.0f, "%.3f", 2.0f); ImGui::SameLine();
        ImGui::SetNextItemWidth(70.0f);
        ImGui::SliderFloat("Zoom", &state.target_zoom[1], 20.0f, 400.0f, "%.3f", 3.0f);
        ImGui::DragFloat2("Pan", state.target_pan);
        ImGui::SliderInt("Stride", &state.stride, 1, 5);
    }
    ImGui::End();
}


void Application::drawScene() {
    auto& state = Registry.ctx<Sequentity::State>();

    ImGui::Begin("3D Viewport", nullptr);
    {
        if (Widgets::Button("Select (Q)", _activeTool.type == ToolType::Select)) {
            _activeTool = Tool{ ToolType::Select, SelectTool };
        }

        if (Widgets::Button("Translate (W)", _activeTool.type == ToolType::Translate)) {
            _activeTool = Tool{ ToolType::Translate, TranslateTool };
        }

        if (Widgets::Button("Rotate (E)", _activeTool.type == ToolType::Rotate)) {
            _activeTool = Tool{ ToolType::Rotate, RotateTool };
        }

        if (Widgets::Button("Scale (R)", _activeTool.type == ToolType::Scale)) {
            _activeTool = Tool{ ToolType::Scale, ScaleTool };
        }

        Widgets::Button("Scrub (K)", _activeTool.type == ToolType::Scrub);

        auto rpos = Vector2i(Vector2(ImGui::GetMouseDragDelta(0, 0.0f)));
        auto apos = Vector2i(Vector2(ImGui::GetIO().MousePos.x - ImGui::GetWindowPos().x,
                                     ImGui::GetIO().MousePos.y - ImGui::GetWindowPos().y));
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

            auto time = state.current_time + (_playing ? 1 : 0);
            bool selected = Registry.has<Selected>(entity);
            Widgets::Graphic(name.text, impos, imsize, imangle, imcolor, selected);

            if (ImGui::IsItemActivated()) {
                Registry.assign<Activated>(entity, state.current_time);
                Registry.assign<InputPosition2D>(entity, absolutePosition, relativePosition);
            }

            else if (ImGui::IsItemActive()) {
                Registry.assign<Active>(entity);
                Registry.replace<InputPosition2D>(entity, absolutePosition, relativePosition);
            }

            else if (ImGui::IsItemDeactivated()) {
                Registry.assign<Deactivated>(entity);
            }
        });

        Registry.view<Position, Sequentity::Track, Color>().each([&](const auto& position,
                                                                     const auto& track,
                                                                     const auto& color) {
            Sequentity::Intersect(track, state.current_time, [&](auto& event) {
                if (event.type == TranslateEvent) {
                    auto& data = *static_cast<TranslateEventData*>(event.data);
                    auto pos = position + data.offset;
                    auto impos = ImVec2(Vector2(Vector2i(pos.x, pos.y)));
                    Widgets::Cursor(impos, color);
                }
            });
        });
    }
    ImGui::End();
}


void Application::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _imgui.newFrame();

         if ( ImGui::GetIO().WantTextInput && !isTextInputActive()) startTextInput();
    else if (!ImGui::GetIO().WantTextInput &&  isTextInputActive()) stopTextInput();

    drawCentralWidget();
    drawTransport();
    drawScene();

    // Handle any input coming from the above drawScene()
    _activeTool.system();

    auto& state = Registry.ctx_or_set<Sequentity::State>();

    if (_playing) step(1);

    // current_time is *mutable* and can change from anywhere
    // Thus, we compare it against the previous time change to
    // determine whether or not it has changed.
    if (state.current_time != _previous_time) {
        onTimeChanged();
        _previous_time = state.current_time;
    }

    _sequentity.draw(&_showSequencer);

    // Erase all current inputs
    if (!Registry.empty<Deactivated>()) {
        Registry.reset<InputPosition2D>();
        Registry.reset<InputPosition3D>();
    }

    // Restore order to this world
    Registry.reset<Active>();
    Registry.reset<Activated>();
    Registry.reset<Deactivated>();

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

    if (event.key() == KeyEvent::Key::K && !event.isRepeated()) {
        Debug() << "Scrub tool..";
        _previousTool = _activeTool;
        _activeTool = Tool{ ToolType::Scrub, ScrubTool };
    }

    if (event.key() == KeyEvent::Key::Q) {
        Debug() << "Select tool..";
        _previousTool = _activeTool;
        _activeTool = Tool{ ToolType::Select, SelectTool };
    }

    if (event.key() == KeyEvent::Key::W) {
        Debug() << "Translate tool..";
        _previousTool = _activeTool;
        _activeTool = Tool{ ToolType::Translate, TranslateTool };
    }

    if (event.key() == KeyEvent::Key::E) {
        Debug() << "Rotate tool..";
        _previousTool = _activeTool;
        _activeTool = Tool{ ToolType::Rotate, RotateTool };
    }

    if (event.key() == KeyEvent::Key::R) {
        Debug() << "Scale tool..";
        _previousTool = _activeTool;
        _activeTool = Tool{ ToolType::Scale, ScaleTool };
    }

    if(_imgui.handleKeyPressEvent(event)) return;
}

void Application::keyReleaseEvent(KeyEvent& event) {
    if (event.key() == KeyEvent::Key::K) {
        _activeTool = _previousTool;
    }

    if(_imgui.handleKeyReleaseEvent(event)) return;
}

void Application::mousePressEvent(MouseEvent& event) {
    if (_imgui.handleMousePressEvent(event)) return;
}

void Application::mouseMoveEvent(MouseMoveEvent& event) {
    if (_imgui.handleMouseMoveEvent(event)) return;
}

void Application::mouseReleaseEvent(MouseEvent& event) {
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