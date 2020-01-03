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
#include "Sequencer.inl"
#include "Widgets.inl"


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
    Sequencer::Sequencer _sequencer;

    Vector2 _dpiScaling { 1.0f, 1.0f };

    bool _running { true };

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
    Registry.assign<Index>(red, 0, 0);
    Registry.assign<Size>(red, Vector2i{ 100, 100 });
    Registry.assign<Color>(red, ImColor::HSV(0.0f, 0.75f, 0.75f), ImColor::HSV(0.0f, 0.75f, 0.1f));
    Registry.assign<Position>(red, Vector2i{ 300, 50 });
    Registry.assign<InitialPosition>(red, Vector2i{ 300, 50 });

    Registry.assign<Name>(green, "Green Rigid");
    Registry.assign<Index>(green, 1, 0);
    Registry.assign<Size>(green, Vector2i{ 100, 100 });
    Registry.assign<Color>(green, ImColor::HSV(0.33f, 0.75f, 0.75f), ImColor::HSV(0.33f, 0.75f, 0.1f));
    Registry.assign<Position>(green, Vector2i{ 500, 50 });
    Registry.assign<InitialPosition>(green, Vector2i{ 500, 50 });

    Registry.assign<Name>(blue, "Blue Rigid");
    Registry.assign<Index>(blue, 2, 0);
    Registry.assign<Size>(blue, Vector2i{ 100, 100 });
    Registry.assign<Color>(blue, ImColor::HSV(0.55f, 0.75f, 0.75f), ImColor::HSV(0.55f, 0.75f, 0.1f));
    Registry.assign<Position>(blue, Vector2i{ 700, 50 });
    Registry.assign<InitialPosition>(blue, Vector2i{ 700, 50 });

    Registry.assign<Name>(purple, "Purple Rigid");
    Registry.assign<Index>(purple, 3, 0);
    Registry.assign<Size>(purple, Vector2i{ 80, 100 });
    Registry.assign<Color>(purple, ImColor::HSV(0.45f, 0.75f, 0.75f), ImColor::HSV(0.45f, 0.75f, 0.1f));
    Registry.assign<Position>(purple, Vector2i{ 350, 200 });
    Registry.assign<InitialPosition>(purple, Vector2i{ 350, 200 });

    Registry.assign<Name>(gray, "Gray Rigid");
    Registry.assign<Index>(gray, 4, 0);
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
        Registry.view<Position, Sequencer::Channel, Color>().each([&](auto entity, auto& position, const auto& channel, const auto& color) {
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

            const Sequencer::Event* current = _sequencer.overlapping(channel);

            // Inbetween channel, that's ok
            if (current == nullptr) return;

            const int index = currentTime - current->time;
            const auto value = current->data[index] - current->offset;
            position = value;
        });

        Registry.reset<StartPosition>();
    }
}


auto Application::dpiScaling() const -> Vector2 { return _dpiScaling; }


void Application::clear() {
    Registry.view<Position, InitialPosition>().each([&](auto& position, const auto& initial) {
        position = initial;
    });

    Registry.reset<Sequencer::Channel>();
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
            static Sequencer::Event* current { nullptr };

            auto imsize = ImVec2((float)size.x(), (float)size.y());
            auto impos = ImVec2((float)position.x(), (float)position.y());

            auto mousePosition = Vector2i(Vector2(ImGui::GetIO().MousePos - ImGui::GetWindowPos()));

            ImGui::SetCursorPos(impos);
            auto state = SmartButton(name.text, imsize);
            auto time = _sequencer.currentTime + 1;

            if (state & SmartButtonState_Pressed) {
                Debug() << "Pressed @" << mousePosition;

                Sequencer::Event event;

                event.time = time;
                event.length = 1;
                event.offset = mousePosition - position;
                event.pressPosition = position;
                event.releasePosition = position;
                event.data.push_back(mousePosition);

                const bool shouldSort = !Registry.has<Sequencer::Channel>(entity);
                auto& channel = Registry.get_or_assign<Sequencer::Channel>(entity);
                channel.push_back(event);
                current = &channel.back();

                if (shouldSort) {
                    Registry.sort<Index>([this](const entt::entity lhs, const entt::entity rhs) {
                        return Registry.get<Index>(lhs).absolute < Registry.get<Index>(rhs).absolute;
                    });

                    // Update the relative positions of each event
                    // TODO: Is this really the most efficient way to do this?
                    int previous { 0 };
                    Registry.view<Index>().each([&](auto entity, auto& index) {
                        if (Registry.has<Sequencer::Channel>(entity)) {
                            index.relative = previous;
                            previous++;
                        }
                    });
                }
            }

            else if (state & SmartButtonState_Dragged) {
                if (current != nullptr && current->time + current->length > time) {
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

        });

        Registry.view<Position, Sequencer::Channel, Color>().each([&](const auto& position,
                                                          const auto& channel,
                                                          const auto& color) {
            if (auto ovl = _sequencer.overlapping(channel)) {
                auto impos = ImVec2((float)position.x(), (float)position.y());
                impos.x += ovl->offset.x();
                impos.y += ovl->offset.y();
                drawCursor(impos, color.fill);
            }
        });
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

    _sequencer.update();
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