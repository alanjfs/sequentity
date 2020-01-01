#include <string>
#include <iostream>
#include <map>
#include <unordered_map>

#include <Magnum/Math/Color.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Platform/GlfwApplication.h>

using namespace Magnum;
using namespace Math::Literals;


struct Event {
    enum Type {
        On,
        Off
    } type;
    int time;
};


class Sequenser : public Platform::Application {
    public:
        explicit Sequenser(const Arguments& arguments);

        void drawEvent() override;
        void drawButtons();
        void drawEditor();
        void drawTransport();
        void drawShapes();
        void drawTimeline();

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

        bool _running { true };
        bool _playing { true };
        Vector2i _range { 0, 100 };
        int _currentTime = 0;
        bool _showRed = false;
        bool _showGreen = false;
        bool _showBlue = false;
        Vector2i _redPos { 0, 0 };
        Vector2i _greenPos { 100, 0 };
        Vector2i _bluePos { 200, 0 };
        Vector2 _dpiScaling { 1.0f, 1.0f };
        float _zoom { 130.0f };
        int _stride { 3 };

        std::unordered_map<int, Vector2i> _redChannel;
        std::unordered_map<int, Vector2i> _greenChannel;
        std::unordered_map<int, Vector2i> _blueChannel;

        std::vector<Event> _redEvents;
        std::vector<Event> _greenEvents;
        std::vector<Event> _blueEvents;
};


Sequenser::Sequenser(const Arguments& arguments): Platform::Application{arguments,
    Configuration{}.setTitle("Sequenser")
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
    io.Fonts->AddFontFromFileTTF("OpenSans-Regular.ttf", 16.0f * _dpiScaling.x());

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
    ImGui::StyleColorsLight();

    /* Set up proper blending to be used by ImGui. There's a great chance
       you'll need this exact behavior for the rest of your scene. If not, set
       this only for the drawFrame() call. */
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}


auto Sequenser::dpiScaling() const -> Vector2 { return _dpiScaling; }


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


void Sequenser::drawButtons() {
    ImGui::Begin("Buttons", nullptr);
    {
        static const auto size = ImVec2(150, 40);
        static SmartButtonState red { 0 };
        static SmartButtonState green { 0 };
        static SmartButtonState blue { 0 };
        auto delta = Vector2i(Vector2(ImGui::GetIO().MouseDelta));

        SmartButton("Red", red, size); ImGui::SameLine();
        SmartButton("Green", green, size); ImGui::SameLine();
        SmartButton("Blue", blue, size);

        if (red & SmartButtonState_Pressed) {
            Debug() << "On event..";
            _showRed = true;
            _redEvents.push_back({ Event::On, _currentTime });
        }

        else if (red & SmartButtonState_Dragged) {
            _redPos += delta;
            _redChannel[_currentTime] = _redPos;
        }

        else if (red & SmartButtonState_Released) {
            Debug() << "Off event..";
            _showRed = false;
            _redEvents.push_back({ Event::Off, _currentTime });
        }

        if (green & SmartButtonState_Pressed) {
            Debug() << "On event..";
            _showGreen = true;
            _greenEvents.push_back({ Event::On, _currentTime });
        }

        else if (green & SmartButtonState_Dragged) {
            _greenPos += delta;
            _greenChannel[_currentTime] = _greenPos;
        }

        else if (green & SmartButtonState_Released) {
            Debug() << "Off event..";
            _showGreen = false;
            _greenEvents.push_back({ Event::Off, _currentTime });
        }

        if (blue & SmartButtonState_Pressed) {
            Debug() << "On event..";
            _showBlue = true;
            _blueEvents.push_back({ Event::On, _currentTime });
        }

        else if (blue & SmartButtonState_Dragged) {
            _bluePos += delta;
            _blueChannel[_currentTime] = _bluePos;
        }

        else if (blue & SmartButtonState_Released) {
            Debug() << "Off event..";
            _showBlue = false;
            _blueEvents.push_back({ Event::Off, _currentTime });
        }
    }
    ImGui::End();
}


void Sequenser::drawEditor() {
    ImGui::Begin("Editor", nullptr);
    {
        auto* painter = ImGui::GetWindowDrawList();
        const auto& style = ImGui::GetStyle();

        ImVec2 windowSize = ImGui::GetWindowSize();
        auto tmlCrn = ImGui::GetWindowPos();
        auto tmlHgt = 50.0f;
        auto seqCrn = ImGui::GetWindowPos();
        float lineHeight = ImGui::GetTextLineHeight() + style.ItemSpacing.y;

        tmlCrn.x += 5.0f;
        seqCrn.x += 5.0f;
        seqCrn.y += tmlHgt + 20.0f;

        int stride = _stride * 5;  // How many frames to skip drawing
        float zoom = _zoom / _stride;
        int minTime = _range.x() / stride;
        int maxTime = _range.y() / stride;
        auto bright = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
        auto mid = ImColor(0.2f, 0.2f, 0.2f, 1.0f);
        auto dark = ImColor(0.1f, 0.1f, 0.1f, 1.0f);

        auto timeline = [&]() {
            for (int time = minTime; time < maxTime + 1; time++) {
                const auto xMin = tmlCrn.x + time * zoom;
                const auto yMin = tmlCrn.y;
                const auto yMax = tmlCrn.y + tmlHgt;

                painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), mid);
                painter->AddText(ImVec2(xMin + 5.0f, yMin + 20.0f), bright, std::to_string(time * stride).c_str());

                for (int z = 0; z < 5 - 1; z++) {
                    const auto innerSpacing = zoom / 5;
                    auto subline = innerSpacing * (z + 1);
                    painter->AddLine(ImVec2(xMin + subline, yMin + 35), ImVec2(xMin + subline, yMax), dark);
                }
            }
        };

        auto horizontalBar = [&]() {
            const auto xMin = tmlCrn.x;
            const auto xMax = tmlCrn.x + windowSize.x;
            const auto yMin = tmlCrn.y + tmlHgt + 10.0f;
            const auto yMax = tmlCrn.y + tmlHgt + 10.0f;

            const auto color = ImColor(0.0f, 0.0f, 0.0f, 0.5f);
            painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMax, yMax), color, 5.0f);
        };

        auto verticalGrid = [&]() {
            for (int time = minTime; time < maxTime + 1; time++) {
                const auto xMin = seqCrn.x + time * zoom;
                const auto yMin = seqCrn.y;
                const auto yMax = seqCrn.y + windowSize.y;

                painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), mid);

                for (int z = 0; z < 5 - 1; z++) {
                    const auto innerSpacing = zoom / 5;
                    auto subline = innerSpacing * (z + 1);
                    painter->AddLine(ImVec2(xMin + subline, yMin), ImVec2(xMin + subline, yMax), dark);
                }
            }
        };

        auto currentTime = [&]() {
            auto color = ImColor(1.0f, 0.5f, 0.5f, 1.0f);
            const auto xMin = tmlCrn.x + _currentTime * zoom / stride;
            const auto yMin = tmlCrn.y;
            const auto yMax = yMin + windowSize.y;
            painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), color, 2.0f);

        };

        auto horizontalGrid = [&]() {
            ImColor color { 1.0f, 1.0f, 1.0f, 0.05f };

            float scrollOffsetH = ImGui::GetScrollX();
            float scrollOffsetV = ImGui::GetScrollY();
            float scrolledOutLines = floorf(scrollOffsetV / lineHeight);
            scrollOffsetV -= lineHeight * scrolledOutLines;

            ImVec2 clipRectMin { seqCrn.x, seqCrn.y };
            ImVec2 clipRectMax {
                clipRectMin.x + windowSize.x,
                clipRectMin.y + windowSize.y
            };

            if (ImGui::GetScrollMaxX() > 0) {
                clipRectMax.y -= style.ScrollbarSize;
            }

            float yMin = seqCrn.y - scrollOffsetV + ImGui::GetCursorPosY();
            float yMax = clipRectMax.y - scrollOffsetV + lineHeight;
            float xMin = seqCrn.x;
            float xMax = seqCrn.x + windowSize.x;

            bool isOdd = (static_cast<int>(scrolledOutLines) % 2) == 0;
            for (float y = yMin; y < yMax; y += lineHeight, isOdd = !isOdd) {
                if (isOdd) painter->AddRectFilled(
                    { xMin, y },
                    { xMax, y + lineHeight - 1 },
                    color
                );
            }
        };

        auto keys = [&]() {
            int yOffset = 0;

            for (auto* events : { &_redEvents, &_greenEvents, &_blueEvents }) {
                for (std::size_t i=0; i < events->size(); ++i) {
                    auto& event = events->at(i);

                    if (event.type == Event::On) {
                        int startTime = event.time;
                        int endTime = _currentTime;

                        // Find end
                        if (events->size() - 1 > i) {
                            auto off = events->at(i + 1);
                            endTime = off.time;
                        }

                        static ImColor fill;
                        static ImColor stroke;
                        static float transparency = 0.5f;

                        if (yOffset == 0) {
                            fill = ImColor::HSV(0.0f, 1.0f, 0.5f, transparency);
                            stroke = ImColor::HSV(0.0f, 1.0f, 1.5f);
                        }
                        if (yOffset == 1) {
                            fill = ImColor::HSV(0.33f, 1.0f, 0.5f, transparency);
                            stroke = ImColor::HSV(0.33f, 1.0f, 1.0f);
                        }
                        if (yOffset == 2) {
                            fill = ImColor::HSV(0.5f, 1.0f, 0.5f, transparency);
                            stroke = ImColor::HSV(0.5f, 1.0f, 1.0f);
                        }

                        float xMin = static_cast<float>(startTime) * zoom / stride;
                        float xMax = static_cast<float>(endTime) * zoom / stride;
                        float yMin = 0;
                        float yMax = lineHeight;

                        xMin += seqCrn.x;
                        xMax += seqCrn.x;
                        yMin += seqCrn.y + (lineHeight * yOffset);
                        yMax += seqCrn.y + (lineHeight * yOffset);

                        painter->AddRectFilled({ xMin, yMin }, { xMax, yMax }, fill, 3.0f);
                        painter->AddRect({ xMin, yMin }, { xMax, yMax }, stroke, 3.0f);
                    }
                }

                yOffset += 1;
            }
        };

        timeline();
        horizontalGrid();
        horizontalBar();
        verticalGrid();
        keys();
        currentTime();

    }
    ImGui::End();
}


void Sequenser::drawTransport() {
    ImGui::Begin("Transport", nullptr);
    {
        if (ImGui::Button("Play")) _playing ^= true;
        ImGui::SameLine();
        if (ImGui::Button("<")) _currentTime -= 1;
        ImGui::SameLine();
        if (ImGui::Button(">")) _currentTime += 1;

        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            _currentTime = _range.x();
            _playing = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            _redChannel.clear();
            _greenChannel.clear();
            _blueChannel.clear();

            _redEvents.clear();
            _greenEvents.clear();
            _blueEvents.clear();
        }

        ImGui::DragInt("Time", &_currentTime);
        if (ImGui::DragInt2("Range", _range.data())) {
            if (_currentTime < _range.x()) {
                _currentTime = _range.x();
            }

            if (_currentTime > _range.y()) {
                _currentTime = _range.y();
            }
        }

        ImGui::SliderFloat("Zoom", &_zoom, 5.0f, 200.0f);
        ImGui::SliderInt("Stride", &_stride, 1, 5);
    }
    ImGui::End();
}


void Sequenser::drawShapes() {
    ImVec2 size = ImGui::GetWindowSize();
    auto painter = ImGui::GetForegroundDrawList();
    Vector2i shapeSize { 100, 100 };

    {
        const ImU32 col = ImColor(1.0f, 0.0f, 0.0f, 1.0f);
        auto topLeft = Vector2(_redPos);
        auto bottomRight = Vector2(_redPos + shapeSize);

        auto itl = ImVec2(topLeft.x(), topLeft.y());
        auto ibr = ImVec2(bottomRight.x(), bottomRight.y());

        painter->AddRect(itl, ibr, col);

        if (_redChannel.count(_currentTime)) {
            auto redPos = _redChannel[_currentTime];
            auto topLeft = Vector2(redPos);
            auto bottomRight = Vector2(redPos + shapeSize);

            auto itl = ImVec2(topLeft.x(), topLeft.y());
            auto ibr = ImVec2(bottomRight.x(), bottomRight.y());

            painter->AddRectFilled(itl, ibr, col);
        }
    }

    {
        const ImU32 col = ImColor(0.0f, 1.0f, 0.0f, 1.0f);
        auto topLeft = Vector2(_greenPos);
        auto bottomRight = Vector2(_greenPos + shapeSize);
        auto itl = ImVec2(topLeft.x(), topLeft.y());
        auto ibr = ImVec2(bottomRight.x(), bottomRight.y());

        painter->AddRect(itl, ibr, col);

        if (_greenChannel.count(_currentTime)) {
            auto greenPos = _greenChannel[_currentTime];
            auto topLeft = Vector2(greenPos);
            auto bottomRight = Vector2(greenPos + shapeSize);

            auto itl = ImVec2(topLeft.x(), topLeft.y());
            auto ibr = ImVec2(bottomRight.x(), bottomRight.y());

            painter->AddRectFilled(itl, ibr, col);
        }
    }

    {
        const ImU32 col = ImColor(0.0f, 0.0f, 1.0f, 1.0f);
        auto topLeft = Vector2(_bluePos);
        auto bottomRight = Vector2(_bluePos + shapeSize);
        auto itl = ImVec2(topLeft.x(), topLeft.y());
        auto ibr = ImVec2(bottomRight.x(), bottomRight.y());

        painter->AddRect(itl, ibr, col);

        if (_blueChannel.count(_currentTime)) {
            auto bluePos = _blueChannel[_currentTime];
            auto topLeft = Vector2(bluePos);
            auto bottomRight = Vector2(bluePos + shapeSize);

            auto itl = ImVec2(topLeft.x(), topLeft.y());
            auto ibr = ImVec2(bottomRight.x(), bottomRight.y());

            painter->AddRectFilled(itl, ibr, col);
        }
    }
}


void Sequenser::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _imgui.newFrame();

    if (_playing) {
        _currentTime += 1;
    }

    if (_currentTime > _range.y()) {
        _currentTime = _range.x();
    }

    if (ImGui::GetIO().WantTextInput && !isTextInputActive()) startTextInput();
    else if (!ImGui::GetIO().WantTextInput && isTextInputActive()) stopTextInput();

    // Split events occurring past the end frame
    for (auto* e : { &_redEvents, &_greenEvents, &_blueEvents }) {
        if (e->size() > 0) {
            auto last = e->back();

            if (last.type == Event::On) {
                if (last.time > _currentTime) {
                    e->push_back({ Event::Off, _range.y() });
                    e->push_back({ Event::On, _currentTime });
                }
            }
        }
    }

    drawEditor();
    drawTransport();
    drawButtons();
    drawShapes();

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

void Sequenser::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _imgui.relayout(Vector2{ event.windowSize() } / dpiScaling(),
        event.windowSize(), event.framebufferSize());
}

void Sequenser::keyPressEvent(KeyEvent& event) {
    if (event.key() == KeyEvent::Key::Esc) this->exit();
    if (event.key() == KeyEvent::Key::Enter) redraw();
    if (event.key() == KeyEvent::Key::Backspace) _running ^= true;
    if(_imgui.handleKeyPressEvent(event)) return;
}

void Sequenser::keyReleaseEvent(KeyEvent& event) {
    if(_imgui.handleKeyReleaseEvent(event)) return;
}

void Sequenser::mousePressEvent(MouseEvent& event) {
    if(_imgui.handleMousePressEvent(event)) return;
}

void Sequenser::mouseReleaseEvent(MouseEvent& event) {
    if(_imgui.handleMouseReleaseEvent(event)) return;
}

void Sequenser::mouseMoveEvent(MouseMoveEvent& event) {
    if(_imgui.handleMouseMoveEvent(event)) return;
}

void Sequenser::mouseScrollEvent(MouseScrollEvent& event) {
    if(_imgui.handleMouseScrollEvent(event)) {
        /* Prevent scrolling the page */
        event.setAccepted();
        return;
    }
}

void Sequenser::textInputEvent(TextInputEvent& event) {
    if(_imgui.handleTextInputEvent(event)) return;
}


MAGNUM_APPLICATION_MAIN(Sequenser)