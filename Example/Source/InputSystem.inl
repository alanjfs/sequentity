#include <map>


// Hardware singletons
struct MouseDevice {
    entt::entity assignedTool { entt::null };
    entt::entity lastPressed { entt::null };
    entt::entity lastHovered { entt::null };

    int time { 0 };
    int startTime { 0 };
    int endTime { 0 };

    std::map<int, Tool::InputPosition2D> positions;

    bool pressed { false };
    bool released { false };
    bool dragging { false };
    bool changed { false };

    const char* id { "mouse" };

    // Internal state
    Position _position { 0, 0 };
    Position _pressPosition { 0, 0 };
    Position _lastPosition { 0, 0 };
    Position _deltaPosition { 0, 0 };
};


struct WacomDevice {
    entt::entity lastPressed { entt::null };
    Tool::InputPosition2D data;

    const char* id { "wacom" };
};


struct GamepadDevice {
    entt::entity lastPressed { entt::null };
    Tool::InputPosition2D data;

    const char* id { "gamepad" };
};


static void copy_to_tool(MouseDevice device, entt::entity target, bool animation = true) {
    auto& data = Registry.assign_or_replace<Tool::Data>(device.assignedTool); {
        data.target = target;
        data.time = device.time;
        data.startTime = device.startTime;
        data.endTime = device.endTime;

        if (animation) {
            data.positions = device.positions;

        } else {
            data.positions[device.time] = device.positions[device.time];
            data.startTime = data.time;
            data.endTime = data.time;
        }
    }
}


static void MouseInputSystem() {
    auto press = [](MouseDevice& device) {
        auto& app = Registry.ctx<ApplicationState>();
        auto entity = device.lastPressed;

        device._pressPosition = device._position;

        if (Registry.valid(entity)) {
            assert(!Registry.has<Tool::BeginIntent>(device.assignedTool));
            Registry.assign<Tool::BeginIntent>(device.assignedTool);
            copy_to_tool(device, device.lastPressed);
        }
    };

    auto drag_entity = [](MouseDevice& device) {
        auto& app = Registry.ctx<ApplicationState>();
        auto entity = device.lastPressed;

        // May be updated by events (that we would override)
        Registry.assign<Tool::UpdateIntent>(device.assignedTool, app.time);
        copy_to_tool(device, device.lastPressed);
    };

    auto drag_nothing = [](MouseDevice& device) {
        // E.g. a selection rectangle
    };

    auto hover = [](MouseDevice& device) {
        Registry.reset<Tooltip>();
        Registry.reset<Tool::Data>(device.assignedTool);

        auto entity = device.lastHovered;

        if (Registry.valid(entity)) {
            auto& app = Registry.ctx<ApplicationState>();
            Registry.assign_or_replace<Tool::PreviewIntent>(device.assignedTool);
            copy_to_tool(device, device.lastHovered, false);
        }
    };

    auto release = [](MouseDevice& device) {
        auto& app = Registry.ctx<ApplicationState>();

        if (app.recording) {
            Registry.assign_or_replace<Tool::RecordIntent>(device.assignedTool);
        }
    };


    // Mappings
    Registry.view<MouseDevice>().each([&](auto& device) {

        // TODO: This should never really happen.
        //       Find out when it does and why
        if (!Registry.valid(device.assignedTool)) return;

        // Update internal data
        device._position = device.positions[device.time].absolute;

        if (device.pressed) {
            press(device);
            device.pressed = false;
            device.dragging = true;
        }

        else if (device.released) {
            release(device);
            device.released = false;
            device.dragging = false;
        }

        else if (device.dragging) {
            if (device.changed) {
                if (Registry.valid(device.lastPressed)) {
                    drag_entity(device);
                }

                else {
                    drag_nothing(device);
                }
            }
        }

        else {
            hover(device);
        }

        // Lazily update intents
        device.changed = false;

        // In order to compute the delta between runs
        device._deltaPosition = device._position - device._lastPosition;
        device._lastPosition = device._position;
    });
}


static void InputSystem() {
    MouseInputSystem();
}
