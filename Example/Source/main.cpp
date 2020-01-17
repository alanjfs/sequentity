/**

Example usage of Sequentity.inl

*/

// (Optional) Magnum prefers to have its imgui.h included first
#include <Magnum/ImGuiIntegration/Context.hpp>

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

struct ApplicationState {
    bool playing { false };
    bool recording { false };

    // Debugging, for control over the event loop
    bool running { true };

    // Track in order to determine whether time has changed
    int time { 0 };
    int previousTime { 0 };

};

// For readability only; this really is just one big cpp file
#include "Utils.inl"
#include "Theme.inl"
#include "Components.inl"
#include "Widgets.inl"

#include "IntentSystem.inl"
#include "ToolSystem.inl"
#include "InputSystem.inl"


class Application : public Platform::Application {
public:
    explicit Application(const Arguments& arguments);

    void drawEvent() override;
    void drawScene();
    void drawTool();
    void drawTransport();
    void drawCentralWidget();
    void drawEventEditor();

    void play();
    void step(int time);
    void stop();

    void setCurrentTool(Tool::Type type);

    void setup();  // Populate registry with entities and components
    void reset();  // Reset all entities
    void clear();  // Clear all events

private:
    void onTimeChanged(); // Apply active events from Sequentity
    void onRecordingChanged(bool recording);
    void onNewTrack(entt::entity);
    void pollGamepad();

    auto dpiScaling() const -> Vector2;
    void viewportEvent(ViewportEvent& event) override;


    void keyPressEvent(KeyEvent& event) override;
    void keyReleaseEvent(KeyEvent& event) override;

    void mousePressEvent(MouseEvent& event) override;
    void mouseReleaseEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void mouseScrollEvent(MouseScrollEvent& event) override;
    void textInputEvent(TextInputEvent& event) override;

    ImGuiIntegration::Context _imgui{ NoCreate };

    Vector2 _dpiScaling { 1.0f, 1.0f };

    Tool::Type _currentToolType { Tool::Type::Translate };
    Tool::Type _previousToolType { Tool::Type::Translate };

    std::unordered_map<std::string_view, entt::entity> _devices;

    bool _showSequencer { true };
    bool _showMetrics { false };
    bool _showStyleEditor { false };

    // Tool contexts are relative the currently active panel.
    // E.g. when working in the 3d view, a different set of tools
    // are made available, with one being "current".
    struct ApplicationContext {
        Tool::Type currentTool { Tool::Type::Select };
    } _applicationContext;

    struct SceneContext {
        Tool::Type currentTool { Tool::Type::Translate };
    } _sceneContext;

    struct EditorContext {
        Tool::Type currentTool { Tool::Type::Select };
    } _editorContext;

    // Experimental gamepad support
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

    Registry.destroy(Registry.create());  // Make index 0 invalid

    // Synchronise Sequentity with our internal application selection
    Registry.on_construct<Selected>().connect<on_select_constructed>();
    Registry.on_destroy<Selected>().connect<on_select_destroyed>();

    // AQ4: Keep track of initial values
    Registry.on_construct<Position>().connect<on_position_constructed>();
    Registry.on_construct<Size>().connect<on_size_constructed>();

    // Keep tracks sorted in the order of our application Index component
    // E.g. in the order of your 3D character hierarchy
    Registry.on_construct<Sequentity::Track>().connect<&Application::onNewTrack>(*this);

    // Initialise state
    Registry.set<Sequentity::State>();
    Registry.set<ApplicationState>();

    // Hardware singletons
    _devices["mouse0"] = Registry.create();
    _devices["mouse1"] = Registry.create();
    _devices["wacomTouch0"] = Registry.create();
    _devices["wacomTouch1"] = Registry.create();
    _devices["wacomTouch2"] = Registry.create();

    Registry.assign<Input::MouseDevice>(_devices["mouse0"]);
    Registry.assign<Input::MouseDevice>(_devices["mouse1"]);

    setCurrentTool(Tool::Type::Translate);

    setup();
    play();
}


void Application::onNewTrack(entt::entity entity) {
    Registry.assign<SortTracksIntent>(entity);
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
    Registry.assign<Position>(red, 500, 200);

    Registry.assign<Name>(green, "leftLeg");
    Registry.assign<Index>(green, 2);
    Registry.assign<Size>(green, 100, 100);
    Registry.assign<Color>(green, ImColor::HSV(0.33f, 0.75f, 0.75f));
    Registry.assign<Orientation>(green, 0.0_degf);
    Registry.assign<Position>(green, 700, 200);

    Registry.assign<Name>(blue, "foot");
    Registry.assign<Index>(blue, 3);
    Registry.assign<Size>(blue, 100, 100);
    Registry.assign<Color>(blue, ImColor::HSV(0.55f, 0.75f, 0.75f));
    Registry.assign<Orientation>(blue, 0.0_degf);
    Registry.assign<Position>(blue, 1000, 200);

    Registry.assign<Name>(purple, "leftShoulder");
    Registry.assign<Index>(purple, 4);
    Registry.assign<Size>(purple, 80, 100);
    Registry.assign<Color>(purple, ImColor::HSV(0.45f, 0.75f, 0.75f));
    Registry.assign<Orientation>(purple, 0.0_degf);
    Registry.assign<Position>(purple, 400, 400);

    Registry.assign<Name>(gray, "head");
    Registry.assign<Index>(gray, 5);
    Registry.assign<Size>(gray, 80, 40);
    Registry.assign<Color>(gray, ImColor::HSV(0.55f, 0.0f, 0.55f));
    Registry.assign<Orientation>(gray, 0.0_degf);
    Registry.assign<Position>(gray, 600, 400);

    _entities.push_back(red);
    _entities.push_back(green);
    _entities.push_back(blue);
    _entities.push_back(purple);
    _entities.push_back(gray);
}


void Application::play() {
    if (!Registry.ctx<ApplicationState>().playing) {
        stop();
        reset();
    }

    Registry.ctx<ApplicationState>().playing ^= true;
}


void Application::step(int delta) {
    auto& sqty = Registry.ctx_or_set<Sequentity::State>();
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
    auto& sqty = Registry.ctx_or_set<Sequentity::State>();

    sqty.current_time = sqty.range[0];
    Registry.ctx<ApplicationState>().playing = false;
}


void Application::onTimeChanged() {
    auto& app = Registry.ctx<ApplicationState>();
    auto& sqty = Registry.ctx<Sequentity::State>();
    auto startTime = sqty.range[0];
    auto current_time = sqty.current_time;

    if (current_time <= startTime) {
        reset();
    }

    else {
        Sequentity::Intersect(Registry, current_time, [&](entt::entity subject,
                                                          const Sequentity::Channel& channel,
                                                          const Sequentity::Event& event) {
            entt::entity tool = event.payload;
            if (!Registry.valid(event.payload)) return;

            // Some events don't carry a tool, and that's ok.
            if (!Registry.has<Tool::Data>(tool)) return;

            auto& data = Registry.get<Tool::Data>(tool);
            const auto local_time = current_time + (data.startTime - event.time);

            if (data.positions.count(local_time)) {
                Registry.assign<Tool::UpdateIntent>(tool, local_time);
            }
        });
    }

    app.time = sqty.current_time;
    app.previousTime = sqty.current_time;
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

    Registry.view<Input::MouseDevice>().each([](auto& device) {
        device.released = device.dragging;
        device.dragging = false;
    });
}


void Application::clear() {

    // TODO: This is much too explicit and error prone. Is there a better
    //       way to assert that when a channel goes away, so does the data?
    int eventCount { 0 };
    int channelCount { 0 };
    Registry.view<Sequentity::Track>().each([&channelCount, &eventCount](auto& track) {
        for (auto& [type, channel] : track.channels) {

            if (Registry.valid(channel.payload)) {
                Registry.destroy(channel.payload);
                channelCount++;
            }

            for (auto& event : channel.events) {
                if (Registry.valid(event.payload)) {
                    Registry.destroy(event.payload);
                    eventCount++;
                }
            }
        }
    });

    Registry.reset<Sequentity::Track>();

    reset();

    if (eventCount) Debug() << "Deleted" << eventCount << "events &" << channelCount << "channels";
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


void Application::onRecordingChanged(bool recording) {
    // TODO: Give user control over which devices become record-enabled
    Registry.reset<Tool::Recording>();

    if (recording) {
        Registry.view<Tool::Meta>().each([](auto entity, const auto& meta) {
            Registry.assign<Tool::Recording>(entity);
        });
    }
}


void Application::drawTransport() {
    auto& sqty = Registry.ctx<Sequentity::State>();
    auto& app = Registry.ctx<ApplicationState>();

    ImGui::Begin("Transport", nullptr);
    {
        if (ImGui::Button("Play")) this->play(); ImGui::SameLine();
        if (ImGui::Button("Record")) {
            app.recording ^= true;
            onRecordingChanged(app.recording);
        }
        ImGui::SameLine();
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


void Application::setCurrentTool(Tool::Type type) {
    _previousToolType = _currentToolType;
    _currentToolType = type;

    Registry.view<Input::MouseDevice>().each([](auto& device) {
        device.released = device.dragging;
        device.dragging = false;
    });

    auto& app = Registry.ctx<ApplicationState>();
    auto& device = Registry.get<Input::MouseDevice>(_devices["mouse0"]);
    auto& tool = device.assignedTool;

    if (Registry.valid(tool)) {
        Registry.destroy(tool);
    }

    Debug() << "Assigning a new tool..";
    tool = Registry.create();

    if (app.recording) {
        Registry.assign<Tool::Recording>(tool);
    }

    if (type == Tool::Type::Translate) {
        this->setCursor(Cursor::Crosshair);
        Registry.assign<Tool::TranslateTool>(tool);
        Registry.assign<Tool::SetupIntent>(tool);
        Registry.assign<Tool::Label>(tool, "Translate");
        Registry.assign<Tool::Color>(tool, ImColor::HSV(0.0f, 1.0f, 0.5f));
        Registry.assign<Tool::Type>(tool, Tool::Type::Translate);
        Registry.assign<Tool::Meta>(tool, "Translate", ImColor::HSV(0.0f, 0.75f, 0.75f), Tool::Type::Translate, Tool::TranslateEvent);
    }

    else if (type == Tool::Type::Rotate) {
		this->setCursor(Cursor::Crosshair);
        Registry.assign<Tool::RotateTool>(tool);
        Registry.assign<Tool::SetupIntent>(tool);
        Registry.assign<Tool::Label>(tool, "Rotate");
        Registry.assign<Tool::Color>(tool, ImColor::HSV(0.0f, 1.0f, 0.5f));
        Registry.assign<Tool::Type>(tool, Tool::Type::Rotate);
        Registry.assign<Tool::Meta>(tool, "Rotate", ImColor::HSV(0.33f, 0.75f, 0.75f), Tool::Type::Rotate, Tool::RotateEvent);
    }

    else if (type == Tool::Type::Scale) {
		this->setCursor(Cursor::Crosshair);
        Registry.assign<Tool::ScaleTool>(tool);
        Registry.assign<Tool::SetupIntent>(tool);
        Registry.assign<Tool::Label>(tool, "Scale");
        Registry.assign<Tool::Color>(tool, ImColor::HSV(0.66f, 0.5f, 0.5f));
        Registry.assign<Tool::Type>(tool, Tool::Type::Scale);
        Registry.assign<Tool::Meta>(tool, "Scale", ImColor::HSV(0.55f, 0.75f, 0.75f), Tool::Type::Scale, Tool::ScaleEvent);
    }

    else if (type == Tool::Type::Scrub) {
		this->setCursor(Cursor::ResizeWE);
        Registry.assign<Tool::ScrubTool>(tool);
        Registry.assign<Tool::SetupIntent>(tool);
        Registry.assign<Tool::Label>(tool, "Scrub");
        Registry.assign<Tool::Color>(tool, ImColor::HSV(0.66f, 0.5f, 0.5f));
        Registry.assign<Tool::Type>(tool, Tool::Type::Scrub);
        Registry.assign<Tool::Meta>(tool, "Scrub", ImColor::HSV(0.66f, 0.75f, 0.75f), Tool::Type::Scrub, Tool::ScrubEvent);
    }

    else if (type == Tool::Type::Select) {
		this->setCursor(Cursor::Arrow);
        Registry.assign<Tool::SelectTool>(tool);
        Registry.assign<Tool::SetupIntent>(tool);
        Registry.assign<Tool::Label>(tool, "Select");
        Registry.assign<Tool::Color>(tool, ImColor::HSV(0.66f, 0.5f, 0.5f));
        Registry.assign<Tool::Type>(tool, Tool::Type::Select);
        Registry.assign<Tool::Meta>(tool, "Select", ImColor::HSV(0.66f, 0.75f, 0.75f), Tool::Type::Select, Tool::ScaleEvent);
    }

    else {
        Warning() << "Woops, what tool is that?";
        assert(false);
    }
}


void Application::drawScene() {
    auto& sqty = Registry.ctx<Sequentity::State>();
    auto& app = Registry.ctx<ApplicationState>();

    static bool windowEntered { false };

    // This method is solely responsible for providing this component
    Registry.reset<Hovered>();

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

        if (Widgets::Button("Select (Q)", _currentToolType == Tool::Type::Select)) {
            _sceneContext.currentTool = Tool::Type::Select;
            setCurrentTool(Tool::Type::Select);
        }

        if (Widgets::Button("Translate (W)", _currentToolType == Tool::Type::Translate)) {
            _sceneContext.currentTool = Tool::Type::Translate;
            setCurrentTool(Tool::Type::Translate);
        }

        if (Widgets::Button("Rotate (E)", _currentToolType == Tool::Type::Rotate)) {
            _sceneContext.currentTool = Tool::Type::Rotate;
            setCurrentTool(Tool::Type::Rotate);
        }

        if (Widgets::Button("Scale (R)", _currentToolType == Tool::Type::Scale)) {
            _sceneContext.currentTool = Tool::Type::Scale;
            setCurrentTool(Tool::Type::Scale);
        }

        if (Widgets::Button("Scrub (K)", _currentToolType == Tool::Type::Scrub)) {}
        if (Widgets::RecordButton("Record (T)", app.recording)) {
            app.recording ^= true;
            onRecordingChanged(app.recording);
        }

        Registry.view<Name, Position, Orientation, Color, Size>().each([&](auto entity,
                                                                           const auto& name,
                                                                           const auto& position,
                                                                           const auto& orientation,
                                                                           const auto& color,
                                                                           const auto& size) {
            auto scaledpos = Vector2(position) / dpiScaling();
            auto imsize = ImVec2((float)size.x(), (float)size.y());
            auto impos = ImVec2(scaledpos);
            auto imangle = static_cast<float>(orientation);
            auto imcolor = ImColor(color);

            auto time = sqty.current_time + (app.playing ? 1 : 0);
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

        Sequentity::Intersect(Registry, sqty.current_time, [&](auto entity, auto& event) {
            if (event.type == Tool::TranslateEvent) {
                auto& [position, color] = Registry.get<Position, Color>(entity);
                auto scaledpos = Vector2(position) / dpiScaling();
                if (!Registry.valid(event.payload)) return;
				if (!Registry.has<Tool::Data>(event.payload)) return;
                auto& data = Registry.get<Tool::Data>(event.payload);
                auto impos = ImVec2(scaledpos.x(), scaledpos.y());
                Widgets::Cursor(impos, color);
            }
        });

        // Preview active tools
        Registry.view<Tool::Data, Tool::Meta>().each([&](auto entity, const auto& data, const auto& meta) {
            auto* drawlist = ImGui::GetWindowDrawList();

            drawlist->PathClear();

            ImVec2 tip { 0, 0 };
            for (const auto& [time, position] : data.positions) {
                if (time < data.startTime || time > data.endTime) continue;

                tip = ImVec2(Vector2(position.absolute) / this->dpiScaling());
                drawlist->PathLineTo(tip);
            }

            drawlist->PathStroke(ImColor(meta.color), false, 1.0f);
            drawlist->AddCircleFilled(
                tip,
                5.0f,
                ImColor(meta.color)
            );
        });
    }

    ImGui::End();
}



void Application::drawTool() {
    auto& sqty = Registry.ctx<Sequentity::State>();
    static bool windowEntered { false };

    ImGui::Begin("Instrument Settings", nullptr);
    {
        // ImGui::Text(currentTool()->name());

        // entt::entity subject = currentTool()->subject();
        // if (Registry.valid(subject)) {
        //     ImGui::Text(Registry.get<Name>(subject).text);
        // } else {
        //     ImGui::Text("None");
        // }

        // ImGui::Text(Tool::tooltype_to_char(currentTool()->type()));
        // ImGui::Text(Tool::eventtype_to_char(currentTool()->eventType()));

        // auto& color = currentTool()->color();
        // ImGui::ColorEdit4("Color", &color.x);

        // auto& input = currentTool()->input();
        // ImGui::DragInt2("Input", &input.relative.x);
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

        Sequentity::EventEditor(Registry);
    }
    ImGui::End();
}


void Application::pollGamepad() {
    const auto& sqty = Registry.ctx<Sequentity::State>();

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

    auto& sqty = Registry.ctx_or_set<Sequentity::State>();
    auto& app = Registry.ctx<ApplicationState>();

    _imgui.newFrame();

         if ( ImGui::GetIO().WantTextInput && !isTextInputActive()) startTextInput();
    else if (!ImGui::GetIO().WantTextInput &&  isTextInputActive()) stopTextInput();

    Input::System();

    if (app.playing) step(1);

    // current_time is *mutable* and can change from anywhere
    // Thus, we compare it against the previous time to
    // determine whether or not it has changed.
    if (sqty.current_time != app.previousTime) {
        onTimeChanged();
    }

    Tool::System();
    IntentSystem();

    drawCentralWidget();
    drawTransport();
    drawScene();
    drawTool();

    drawEventEditor();

    if (_showMetrics) {
        ImGui::ShowMetricsWindow(&_showMetrics);
    }

    if (_showStyleEditor) {
        Sequentity::ThemeEditor(&_showStyleEditor);
        ImGui::ShowStyleEditor();
    }

    // _imgui.updateApplicationCursor(*this);

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

    if (app.running) {
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
    if (event.key() == KeyEvent::Key::Backspace)    { Registry.ctx<ApplicationState>().running ^= true; if (Registry.ctx<ApplicationState>().running) redraw(); }
    if (event.key() == KeyEvent::Key::Space)        { this->play(); }
    if (event.key() == KeyEvent::Key::F1)           _showMetrics ^= true;
    if (event.key() == KeyEvent::Key::F2)           _showStyleEditor ^= true;
    if (event.key() == KeyEvent::Key::F5)           _showSequencer ^= true;

    if (event.key() == KeyEvent::Key::K && !event.isRepeated()) setCurrentTool(Tool::Type::Scrub);
    if (event.key() == KeyEvent::Key::Q)                        setCurrentTool(Tool::Type::Select);
    if (event.key() == KeyEvent::Key::W)                        setCurrentTool(Tool::Type::Translate);
    if (event.key() == KeyEvent::Key::E)                        setCurrentTool(Tool::Type::Rotate);
    if (event.key() == KeyEvent::Key::R)                        setCurrentTool(Tool::Type::Scale);
    if (event.key() == KeyEvent::Key::T)                        {
        auto& app = Registry.ctx<ApplicationState>();
        app.recording ^= true;
        onRecordingChanged(app.recording);
    }

    if(_imgui.handleKeyPressEvent(event)) return;
}

void Application::keyReleaseEvent(KeyEvent& event) {
    if (event.key() == KeyEvent::Key::K) {
        setCurrentTool(_previousToolType);
    }

    if(_imgui.handleKeyReleaseEvent(event)) return;
}


void Application::mousePressEvent(MouseEvent& event) {
    auto& app = Registry.ctx<ApplicationState>();

    entt::entity entity { entt::null };
    for (auto hovered : Registry.view<Hovered>()) {
        entity = hovered;
        break;
    };

    auto& device = Registry.get<Input::MouseDevice>(_devices["mouse0"]);
    device.pressed = true;
    device.lastPressed = entity;
    device.lastHovered = entity;

    device.time = app.time;
    device.pressTime = app.time;
    device.releaseTime = app.time;
    device.position = event.position();

    if (_imgui.handleMousePressEvent(event)) return;
}


void Application::mouseMoveEvent(MouseMoveEvent& event) {
    auto& app = Registry.ctx<ApplicationState>();

    entt::entity entity { entt::null };
    for (auto hovered : Registry.view<Hovered>()) {
        entity = hovered;
        break;
    };

    auto& device = Registry.get<Input::MouseDevice>(_devices["mouse0"]);
    device.lastHovered = entity;
    device.changed = true;

    device.time = app.time;
    device.releaseTime = app.time;
    device.position = event.position();

    if (_imgui.handleMouseMoveEvent(event)) return;
}

void Application::mouseReleaseEvent(MouseEvent& event) {
    auto& device = Registry.get<Input::MouseDevice>(_devices["mouse0"]);

    device.released = true;

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