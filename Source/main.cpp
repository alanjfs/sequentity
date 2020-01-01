#include <string>

#include <Magnum/Math/Color.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Platform/GlfwApplication.h>

using namespace Magnum;
using namespace Math::Literals;


class Sequenser : public Platform::Application {
    public:
        explicit Sequenser(const Arguments& arguments);

        void drawEvent() override;
        void drawButtons();
        void drawCanvas();
        void drawTransport();
        void drawShapes();
        void drawTimeline();

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

        bool _playing = false;
        Vector2i _range { 0, 100 };
        unsigned int _currentTime = 0;
        unsigned int _maxTime = 100;
        bool _showRed = false;
        bool _showGreen = false;
        bool _showBlue = false;
        Vector2i _redPos { 0, 0 };
        Vector2i _greenPos { 100, 0 };
        Vector2i _bluePos { 200, 0 };
};


Sequenser::Sequenser(const Arguments& arguments): Platform::Application{arguments,
    Configuration{}.setTitle("Sequenser")
                   .setWindowFlags(Configuration::WindowFlag::Resizable)}
{
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(this->window(),
        (mode->width / 2) - (windowSize().x() / 2),
        (mode->height / 2) - (windowSize().y() / 2)
    );

    Debug() << "Window size:" << windowSize();
    Debug() << "Framebuffer size:" << framebufferSize();
    Debug() << "DPI Scaling:" << dpiScaling();
    Debug() << "ImGui" << IMGUI_VERSION;

    this->setSwapInterval(1);  // VSync

    _imgui = ImGuiIntegration::Context(
        Vector2{ windowSize() } / dpiScaling(),
        windowSize(),
        framebufferSize()
    );

    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
    ImGui::GetIO().ConfigWindowsResizeFromEdges = true;

    /* Set up proper blending to be used by ImGui. There's a great chance
       you'll need this exact behavior for the rest of your scene. If not, set
       this only for the drawFrame() call. */
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}


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

        SmartButton("Click me", red, size); ImGui::SameLine();
        SmartButton("Green", green, size); ImGui::SameLine();
        SmartButton("Blue", blue, size);

        if (red & SmartButtonState_Pressed) {
            Debug() << "On event..";
            _showRed = true;
        }

        else if (red & SmartButtonState_Dragged) {
            Debug() << "Move event.." << delta;
            _redPos += delta;
        }

        else if (red & SmartButtonState_Released) {
            Debug() << "Off event..";
            _showRed = false;
        }

        if (green & SmartButtonState_Pressed) {
            Debug() << "On event..";
            _showGreen = true;
        }

        else if (green & SmartButtonState_Dragged) {
            Debug() << "Move event.." << Vector2(ImGui::GetIO().MouseDelta);
        }

        else if (green & SmartButtonState_Released) {
            Debug() << "Off event..";
            _showGreen = false;
        }

        if (blue & SmartButtonState_Pressed) {
            Debug() << "On event..";
            _showBlue = true;
        }

        else if (blue & SmartButtonState_Dragged) {
            Debug() << "Move event.." << Vector2(ImGui::GetIO().MouseDelta);
        }

        else if (blue & SmartButtonState_Released) {
            Debug() << "Off event..";
            _showBlue = false;
        }

    }
    ImGui::End();
}


void Sequenser::drawCanvas() {
    ImGui::Begin("Canvas", nullptr);
    {
        auto* painter = ImGui::GetWindowDrawList();
        const auto& style = ImGui::GetStyle();

        ImVec2 size = ImGui::GetWindowSize();
        auto tmlCrn = ImGui::GetWindowPos();
        auto tmlHgt = 50.0f;
        auto seqCrn = ImGui::GetWindowPos();
        seqCrn.y += tmlHgt + 20.0f;

        auto timeline = [&]() {
            auto white = ImColor(0.3f, 0.3f, 0.3f, 1.0f);

            int max = _range.y() / 10;
            for (int time = 0; time < max + 1; time++) {
                const auto coarseTime = time * 10;
                const auto spacing = 50.0f;
                const auto x = tmlCrn.x + spacing * time;
                const auto y = tmlCrn.y + tmlHgt;
                painter->AddLine(ImVec2(x, tmlCrn.y), ImVec2(x, y), white);
                painter->AddText(ImVec2(x + 5.0f, tmlCrn.y + 20.0f), white, std::to_string(coarseTime).c_str());

                for (int z = 0; z < 4; z++) {
                    const auto innerSpacing = spacing / 5.0f;
                    auto subline = innerSpacing * (z + 1);
                    painter->AddLine(ImVec2(x + subline, tmlCrn.y + 35), ImVec2(x + subline, y), white);
                }
            }
        };

        auto currentTime = [&]() {
            auto white = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
            const auto spacing = 50.0f;
            const auto minX = tmlCrn.x + _currentTime * 5.0f + 5.0f;
            const auto minY = tmlCrn.y;
            const auto maxY = minY + size.y;
            painter->AddLine(ImVec2(minX, minY), ImVec2(minX, maxY), white, 2.0f);

        };

        auto verticalGrid = [&]() {
            auto white = ImColor(0.3f, 0.3f, 0.3f, 1.0f);
            auto gray = ImColor(0.1f, 0.1f, 0.1f, 1.0f);

            for (int i = 0; i < 10 + 1; i++) {
                const auto spacing = 50.0f;
                const auto x = seqCrn.x + spacing * i;
                const auto y = seqCrn.y + size.y;
                painter->AddLine(ImVec2(x, seqCrn.y), ImVec2(x, y), white);

                for (int z = 0; z < 5; z++) {
                    const auto innerSpacing = spacing / 5.0f;
                    auto subline = innerSpacing * (z + 1);
                    painter->AddLine(ImVec2(x + subline, seqCrn.y), ImVec2(x + subline, y), gray);
                }
            }
        };

        auto horizontalGrid = [&]() {
            ImColor color { 1.0f, 1.0f, 1.0f, 0.05f };
            float lineHeight = ImGui::GetTextLineHeight();
            lineHeight += style.ItemSpacing.y;

            float scrollOffsetH = ImGui::GetScrollX();
            float scrollOffsetV = ImGui::GetScrollY();
            float scrolledOutLines = floorf(scrollOffsetV / lineHeight);
            scrollOffsetV -= lineHeight * scrolledOutLines;

            ImVec2 clipRectMin(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
            ImVec2 clipRectMax(
                clipRectMin.x + ImGui::GetWindowWidth(),
                clipRectMin.y + ImGui::GetWindowHeight()
            );

            if (ImGui::GetScrollMaxX() > 0) {
                clipRectMax.y -= style.ScrollbarSize;
            }

            painter->PushClipRect(clipRectMin, clipRectMax);

            bool isOdd = (static_cast<int>(scrolledOutLines) % 2) == 0;

            float yMin = clipRectMin.y - scrollOffsetV + ImGui::GetCursorPosY() + tmlHgt + 20.0f;
            float yMax = clipRectMax.y - scrollOffsetV + lineHeight;
            float xMin = clipRectMin.x + scrollOffsetH + ImGui::GetWindowContentRegionMin().x;
            float xMax = clipRectMin.x + scrollOffsetH + ImGui::GetWindowContentRegionMax().x;

            for (float y = yMin; y < yMax; y += lineHeight, isOdd = !isOdd) {
                if (isOdd) {
                    painter->AddRectFilled({ xMin, y - style.ItemSpacing.y + 1 }, { xMax, y + lineHeight - 1 }, color);
                }
            }

            painter->PopClipRect();
        };

        timeline();
        horizontalGrid();
        verticalGrid();
        currentTime();
    }
    ImGui::End();
}


void Sequenser::drawTransport() {
    ImGui::Begin("Transport", nullptr);
    {
        if (ImGui::Button("Play")) {
            _playing ^= true;
        }
        ImGui::SameLine();

        if (ImGui::Button("Stop")) {
            _currentTime = _range.x();
            _playing = false;
        }
        ImGui::SameLine();

        ImGui::DragInt2("Range", _range.data());
    }
    ImGui::End();
}


void Sequenser::drawShapes() {
    ImVec2 size = ImGui::GetWindowSize();
    auto painter = ImGui::GetForegroundDrawList();
    Vector2i shapeSize { 100, 100 };

    if (_showRed) {
        const ImU32 col = ImColor(1.0f, 0.0f, 0.0f, 1.0f);
        auto topLeft = _redPos;
        auto bottomRight = _redPos + shapeSize;
        painter->AddRectFilled(ImVec2(topLeft.x(), topLeft.y()),
                               ImVec2(bottomRight.x(), bottomRight.y()), col);
    }
    if (_showGreen) {
        const ImU32 col = ImColor(0.0f, 1.0f, 0.0f, 1.0f);
        auto topLeft = ImVec2(110, 10);
        auto bottomRight = ImVec2(150, 100);
        painter->AddRectFilled(topLeft, bottomRight, col);
    }
    if (_showBlue) {
        const ImU32 col = ImColor(0.0f, 0.0f, 1.0f, 1.0f);
        auto topLeft = ImVec2(160, 10);
        auto bottomRight = ImVec2(200, 100);
        painter->AddRectFilled(topLeft, bottomRight, col);
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

    drawCanvas();
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
    redraw();
}

void Sequenser::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
        event.windowSize(), event.framebufferSize());
}

void Sequenser::keyPressEvent(KeyEvent& event) {
    if (event.key() == KeyEvent::Key::Esc) this->exit();
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