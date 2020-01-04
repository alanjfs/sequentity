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

#include "Theme.inl"
#include "Math.inl"
#include "Tools.inl"
#include "Components.inl"
#include "Sequentity.inl"
#include "Widgets.inl"

struct Activated { int time; };
struct Active { int time; };
struct Deactivated { int time; };

struct DragdollEventData {
    Vector2i offset;
    std::vector<Vector2i> positions;
};

struct BenddollEventData {
    Deg offset;
    std::vector<Deg> angles;
};

// Possible event types
enum EventType : Sequentity::EventType {
    DragdollEvent = 0,
    BenddollEvent,
};


enum class ToolType : std::uint8_t {
    Select,
    DragSelect,
    LassoSelect,

    Dragdoll,
    Bendoll
};

struct Input1DRange  { int x; };
struct Input2DRange : Vector2i { using Vector2i::Vector2i; };
struct Input3DRange : Vector3i { using Vector3i::Vector3i ;};


struct DragdollTool {
    bool activated { false };
    bool active { false };
    bool deactivated { false };
};


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
    void update();
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
    Sequentity::Sequentity _sequencer { Registry };

    Vector2 _dpiScaling { 1.0f, 1.0f };

    bool _running { true };
    ToolType _activeTool { ToolType::Dragdoll };

    bool _showSequencer { true };
    bool _showMetrics { false };
    bool _showStyleEditor { false };
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

    setup();

    _sequencer.after_stepped([this](int time) { update(); });
    _sequencer.play();
}


void Application::setup() {
    auto red = Registry.create();
    auto green = Registry.create();
    auto blue = Registry.create();
    auto purple = Registry.create();
    auto gray = Registry.create();

    Registry.assign<Name>(red, "Red Rigid");
    Registry.assign<Index>(red, 0);
    Registry.assign<Size>(red, Vector2i{ 100, 100 });
    Registry.assign<Color>(red, ImColor::HSV(0.0f, 0.75f, 0.75f), ImColor::HSV(0.0f, 0.75f, 0.1f));
    Registry.assign<Position>(red, Vector2i{ 300, 50 });
    Registry.assign<InitialPosition>(red, Vector2i{ 300, 50 });

    Registry.assign<Name>(green, "Green Rigid");
    Registry.assign<Index>(green, 1);
    Registry.assign<Size>(green, Vector2i{ 100, 100 });
    Registry.assign<Color>(green, ImColor::HSV(0.33f, 0.75f, 0.75f), ImColor::HSV(0.33f, 0.75f, 0.1f));
    Registry.assign<Position>(green, Vector2i{ 500, 50 });
    Registry.assign<InitialPosition>(green, Vector2i{ 500, 50 });

    Registry.assign<Name>(blue, "Blue Rigid");
    Registry.assign<Index>(blue, 2);
    Registry.assign<Size>(blue, Vector2i{ 100, 100 });
    Registry.assign<Color>(blue, ImColor::HSV(0.55f, 0.75f, 0.75f), ImColor::HSV(0.55f, 0.75f, 0.1f));
    Registry.assign<Position>(blue, Vector2i{ 700, 50 });
    Registry.assign<InitialPosition>(blue, Vector2i{ 700, 50 });

    Registry.assign<Name>(purple, "Purple Rigid");
    Registry.assign<Index>(purple, 3);
    Registry.assign<Size>(purple, Vector2i{ 80, 100 });
    Registry.assign<Color>(purple, ImColor::HSV(0.45f, 0.75f, 0.75f), ImColor::HSV(0.45f, 0.75f, 0.1f));
    Registry.assign<Position>(purple, Vector2i{ 350, 200 });
    Registry.assign<InitialPosition>(purple, Vector2i{ 350, 200 });

    Registry.assign<Name>(gray, "Gray Rigid");
    Registry.assign<Index>(gray, 4);
    Registry.assign<Size>(gray, Vector2i{ 80, 40 });
    Registry.assign<Color>(gray, ImColor::HSV(0.55f, 0.0f, 0.55f), ImColor::HSV(0.55f, 0.0f, 0.1f));
    Registry.assign<Position>(gray, Vector2i{ 550, 200 });
    Registry.assign<InitialPosition>(gray, Vector2i{ 550, 200 });
}


void Application::update() {
    auto startTime = _sequencer.range.x();
    auto currentTime = _sequencer.currentTime;

    if (currentTime <= startTime) {
        Registry.view<Position, InitialPosition>().each([&](auto& position, const auto& initial) {
            position = initial;
        });
    }

    else {
        Registry.view<Position, Sequentity::Channel, Color>().each([&](auto entity,
                                                                       auto& position,
                                                                       const auto& channel,
                                                                       const auto& color) {
            _sequencer.each_overlapping(channel, [&](auto& event) {
                if (event.data == nullptr) { Warning() << "This is a bug"; return; }

                if (event.type == DragdollEvent) {
                    auto data = static_cast<DragdollEventData*>(event.data);
                    const int index = currentTime - event.time;
                    const auto value = data->positions[index] - data->offset;
                    position = value;
                }
            });
        });
    }
}


auto Application::dpiScaling() const -> Vector2 { return _dpiScaling; }


void Application::clear() {
    Registry.view<Position, InitialPosition>().each([&](auto& position, const auto& initial) {
        position = initial;
    });

    Registry.reset<Sequentity::Channel>();
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
        if (ImGui::Button("Play")) _sequencer.play();
        ImGui::SameLine();
        if (ImGui::Button("<")) _sequencer.step(-1);
        ImGui::SameLine();
        if (ImGui::Button(">")) _sequencer.step(1);

        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            _sequencer.stop();
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

        ImGui::SetNextItemWidth(70.0f);
        ImGui::SliderFloat("##zoom", &_sequencer.zoom[0], 50.0f, 400.0f, "%.3f", 2.0f); ImGui::SameLine();
        ImGui::SetNextItemWidth(70.0f);
        ImGui::SliderFloat("Zoom", &_sequencer.zoom[1], 20.0f, 400.0f, "%.3f", 3.0f);
        ImGui::DragFloat2("Scroll", _sequencer.scroll);
        ImGui::SliderInt("Stride", &_sequencer.stride, 1, 5);
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

    ImGui::Begin("3D Viewport", nullptr);
    {
        Registry.view<Name, Position, Size>().each([&](auto entity,
                                                       const auto& name,
                                                       const auto& position,
                                                       const auto& size) {
            auto imsize = ImVec2((float)size.x(), (float)size.y());
            auto impos = ImVec2((float)position.x(), (float)position.y());

            auto mousePosition = Vector2i(Vector2(ImGui::GetIO().MousePos - ImGui::GetWindowPos()));

            ImGui::SetCursorPos(impos);
            auto state = SmartButton(name.text, imsize);
            auto time = _sequencer.currentTime + (_sequencer.playing ? 1 : 0);

            if (state & SmartButtonState_Pressed) {
                Registry.assign<Activated>(entity, _sequencer.currentTime);
                Registry.assign<Input2DRange>(entity, mousePosition);
            }

            else if (state & SmartButtonState_Dragged) {
                Registry.assign<Active>(entity);
                Registry.replace<Input2DRange>(entity, mousePosition);

            }

            else if (state & SmartButtonState_Released) {
                Registry.assign<Deactivated>(entity);
                Registry.reset<Input2DRange>();
            }
        });

        Registry.view<Position, Sequentity::Channel, Color>().each([&](const auto& position,
                                                                       const auto& channel,
                                                                       const auto& color) {
            if (auto event = _sequencer.overlapping(channel)) {
                auto& data = *static_cast<DragdollEventData*>(event->data);
                auto impos = ImVec2(Vector2(position + data.offset));
                drawCursor(impos, color.fill);
            }
        });
    }
    ImGui::End();
}


static void InputSystem() {

    // Handle press input of type: 2D range, relative anything with a position
    Registry.view<Activated, Input2DRange, Position>().each([](auto entity,
                                                               const auto& activated,
                                                               const auto& input,
                                                               const auto& position
                                                               ) {
        const bool is_first_event = !Registry.has<Sequentity::Channel>(entity);

        auto* data = new DragdollEventData{}; {
            data->offset = input - position;
            data->positions.push_back(input);
        }

        Sequentity::Event event; {
            event.time = activated.time + 1;
            event.length = 1;

            // Store reference to our data
            event.type = DragdollEvent;
            event.data = static_cast<void*>(data);
        }

        // Write Sequentity data..
        auto& channel = Registry.get_or_assign<Sequentity::Channel>(entity);
        channel.push_back(event);

        // Update the relative positions of each event
        // TODO: Is this really the most efficient way to do this?
        if (is_first_event) {
            Registry.sort<Index>([this](const entt::entity lhs, const entt::entity rhs) {
                return Registry.get<Index>(lhs).absolute < Registry.get<Index>(rhs).absolute;
            });

            int previous { 0 };
            Registry.view<Index>().each([&](auto entity, auto& index) {
                if (Registry.has<Sequentity::Channel>(entity)) {
                    index.relative = previous;
                    previous++;
                }
            });
        }
    });

    Registry.view<Name, Active, Input2DRange, Sequentity::Channel>().each([](const auto& name,
                                                                             const auto&,
                                                                             const auto& input,
                                                                             auto& channel
                                                                             ) {
        auto& event = channel.back();
        auto data = static_cast<DragdollEventData*>(event.data);
        data->positions.push_back(input);
        event.length += 1;
    });

    Registry.view<Deactivated>().each([](auto entity, const auto&) {});

    // Restore order to this world
    Registry.reset<Active>();
    Registry.reset<Activated>();
    Registry.reset<Deactivated>();
}


void Application::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _imgui.newFrame();

         if ( ImGui::GetIO().WantTextInput && !isTextInputActive()) startTextInput();
    else if (!ImGui::GetIO().WantTextInput &&  isTextInputActive()) stopTextInput();

    drawCentralWidget();
    drawTransport();
    drawRigids();

    // Handle any input coming from the above drawRigids()
    InputSystem();

    Timer updateTimer;
    _sequencer.update();

    Timer drawTimer;
    _sequencer.draw(&_showSequencer);

    if (_showMetrics) {
        ImGui::ShowMetricsWindow(&_showMetrics);
    }

    if (_showStyleEditor) {
        _sequencer.drawThemeEditor();
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
    if (event.key() == KeyEvent::Key::F1)           _showMetrics ^= true;
    if (event.key() == KeyEvent::Key::F2)           _showStyleEditor ^= true;
    if (event.key() == KeyEvent::Key::F5)           _showSequencer ^= true;
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