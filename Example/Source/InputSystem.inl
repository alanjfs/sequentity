#include <map>


namespace Input {

// Hardware singletons
struct MouseDevice {
    entt::entity assignedTool { entt::null };
    entt::entity lastPressed { entt::null };
    entt::entity lastHovered { entt::null };

    TimeType time { 0 };
    TimeType pressTime { 0 };
    TimeType releaseTime { 0 };

    Vector2i position { 0, 0 };

    bool pressed { false };
    bool released { false };
    bool dragging { false };
    bool changed { false };

    const char* id { "mouse" };

    // Internal state
    std::map<TimeType, InputPosition2D> _positions;
    Vector2i _pressPosition { 0, 0 };
    Vector2i _lastPosition { 0, 0 };
    Vector2i _deltaPosition { 0, 0 };
};


struct WacomDevice {
    entt::entity lastPressed { entt::null };

    const char* id { "wacom" };
};


struct GamepadDevice {
    entt::entity lastPressed { entt::null };

    const char* id { "gamepad" };
};


static void copy_to_tool(MouseDevice device, entt::entity target, bool animation = true) {
    auto& data = Registry.assign_or_replace<Tool::Data>(device.assignedTool); {
        data.target = target;
        data.time = device.time;

        if (animation) {
            data.positions = device._positions;
            data.startTime = device.pressTime;
            data.endTime = device.releaseTime;

        } else {
            auto position = (*device._positions.find(device.time)).second;
            data.positions[device.time] = position;
            data.startTime = device.time;
            data.endTime = device.time;
        }
    }
}


static void MouseInputSystem() {
    auto press = [](MouseDevice& device) {
        auto& app = Registry.ctx<ApplicationState>();
        auto entity = device.lastPressed;

        device._pressPosition = device.position;

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
			auto data = Registry.try_get<Tool::Data>(device.assignedTool);
			if (data != nullptr) {
				Debug() << "Assigning a record intent to the tool with data starting @" << data->startTime;
			}
			Registry.assign_or_replace<Tool::RecordIntent>(device.assignedTool);
        }

        device._positions.clear();
    };

    // Mappings
    Registry.view<MouseDevice>().each([&](auto& device) {

        // NOTE: We can't trust the application to provide delta
        // _positions, since inputs come in faster than we can update
        // our application. If two or more events preceed an application
        // update, then the delta will be incorrect.
        //
        // Instead, we only let the application store absolute _positions, 
        // and compute deltas relative where we actually processed those
        // inputs last time.

        device._positions[device.time] = {
            device.position,
            device.position - device._pressPosition,
            device.position - device._lastPosition
        };

        // TODO: This should never really happen.
        //       Find out when it does and why
        if (!Registry.valid(device.assignedTool)) return;

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
        device._deltaPosition = device.position - device._lastPosition;
        device._lastPosition = device.position;
    });
}


static void System() {
    MouseInputSystem();
}

}