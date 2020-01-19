#include <map>


namespace Input {

// All devices carry one of these
struct Device {
    std::string_view id;
};

// Keep track of which device was active last
struct LastUsedDevice {};

// Keep track of which tool is assigned to a device
struct AssignedTool {
    entt::entity entity { entt::null };
};


/**
 * @brief A mouse with 3 buttons and scroll wheel
 *
 *    _________
 *   /   _|_   \
 *  ||   | |   ||
 *  ||   |_|   ||
 *  ||____|____||
 *  |           |
 *  |           |
 *  |           |
 *  |           |
 *  |___________|
 *
 */
struct MouseDevice {
    entt::entity lastPressed { entt::null };
    entt::entity lastHovered { entt::null };

    TimeType time { 0 };
    TimeType pressTime { 0 };
    TimeType releaseTime { 0 };

    Vector2i position { 0, 0 };
    Vector2 scroll { 0.0f, 0.0f };
    Vector2 velocity { 0.0f, 0.0f };
    Vector2 acceleration { 0.0f, 0.0f };

    enum Buttons : int {
        None         = 1 << 0,
        ButtonLeft   = 1 << 1,
        ButtonMiddle = 1 << 2,
        ButtonRight  = 1 << 3,
    };

    int buttons { None };

    bool pressed { false };
    bool released { false };
    bool dragging { false };
    bool changed { false };

    // Statistics
    using Time = std::chrono::time_point<std::chrono::steady_clock>;
    Vector2 input_lag { 1'000.0f, 0.0f }; // min/max in milliseconds
    Time time_of_event { std::chrono::high_resolution_clock::now() };

    // Internal state
    std::map<TimeType, InputPosition2D> _positions;
    Vector2i _pressPosition { 0, 0 };
    Vector2i _lastPosition { 0, 0 };
    Vector2i _deltaPosition { 0, 0 };
};


/**
 * @brief A Wacom Intuos pen
 *
 */
struct WacomPenDevice {
    entt::entity lastPressed { entt::null };
};


/**
 * @brief A Wacom Intuos multi-touch device, with 10 fingers as 100hz
 *
 *      _ 
 *     | |
 *     | |
 *     | | _  _
 *     | |/ \/ \_
 *     |         \ 
 *    /|  ^   ^   |
 *   /            |
 *  |             |
 *  |             |
 *  |             |
 *   \           / 
 *    |         |
 *
 */
struct WacomTouchDevice {
    entt::entity lastPressed { entt::null };
};


/**
 * @brief An XBox controller, or compatible alternative
 * 
 *     _____________
 *    / _           \
 *   / / \  o  o  _  \
 *  /  \_/       / \  \
 * /      +      \_/   \
 * |     _________     |
 *  \   /         \   /
 *   \_/           \_/
 * 
 */
struct GamepadDevice {
    entt::entity lastPressed { entt::null };
};


/**
 * @brief Convert any device to the given tool
 *
 */
static void device_to_tool(const WacomPenDevice& device, entt::entity tool, entt::entity target, bool animation = true);
static void device_to_tool(const WacomTouchDevice& device, entt::entity tool, entt::entity target, bool animation = true);
static void device_to_tool(const GamepadDevice& device, entt::entity tool, entt::entity target, bool animation = true);
static void device_to_tool(const MouseDevice& device, entt::entity tool, entt::entity target, bool animation = true) {
    auto& data = Registry.assign_or_replace<Tool::Data>(tool); {
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

    if (device.buttons & MouseDevice::ButtonLeft) {
        Registry.assign_or_replace<Tool::PrimaryIntent>(tool);
    }

    if (device.buttons & MouseDevice::ButtonRight) {
        Registry.assign_or_replace<Tool::SecondaryIntent>(tool);
    }
}


static void MouseInputSystem() {
    auto press = [](MouseDevice& device, entt::entity tool) {
        auto& app = Registry.ctx<ApplicationState>();

        device._positions.clear();
        device._pressPosition = device.position;

        if (Registry.valid(device.lastPressed)) {
            device_to_tool(device, tool, device.lastPressed);
            Registry.assign<Tool::BeginIntent>(tool);
        }
    };

    auto drag_entity = [](MouseDevice& device, entt::entity tool) {
        auto& app = Registry.ctx<ApplicationState>();

        // May be updated by events (that we would override)
        device_to_tool(device, tool, device.lastPressed);
        Registry.assign<Tool::UpdateIntent>(tool, app.time);
    };

    auto drag_nothing = [](MouseDevice& device, entt::entity tool) {
        // E.g. a selection rectangle
    };

    auto hover = [](MouseDevice& device, entt::entity tool) {
        Registry.reset<Tooltip>();
        Registry.reset<Tool::Data>(tool);

        if (Registry.valid(device.lastHovered)) {
            auto& app = Registry.ctx<ApplicationState>();
            device_to_tool(device, tool, device.lastHovered, false);
            Registry.assign_or_replace<Tool::PreviewIntent>(tool);
        }
    };

    auto release = [](MouseDevice& device, entt::entity tool) {
        auto& app = Registry.ctx<ApplicationState>();
        Registry.assign_or_replace<Tool::FinishIntent>(tool);
    };

    auto compute_input_lag = [](MouseDevice& device) {
        // How much lag did we encounter?
        if (device.changed) {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(
                now - device.time_of_event
            ).count() * 1'000.0f;

            if (duration < device.input_lag.x()) {
                device.input_lag.x() = duration;
            }

            if (duration > device.input_lag.y()) {
                device.input_lag.y() = duration;
            }
        }
    };

    // Mappings
    Registry.view<MouseDevice, AssignedTool>().each([&](auto& device, auto& tool) {
        compute_input_lag(device);

        // NOTE: We can't trust the application to provide delta
        // positions, since inputs come in faster than we can update
        // our application. If two or more events preceed an application
        // update, then the delta will be incorrect.
        //
        // Instead, we only let the application store absolute positions, 
        // and compute deltas relative where we actually processed those
        // inputs previously.

        device._positions[device.time] = {
            device.position,
            device.position - device._pressPosition,
            device.position - device._lastPosition
        };

        if (device.pressed) {
            press(device, tool.entity);
            device.pressed = false;
            device.dragging = true;
        }

        else if (device.released) {
            release(device, tool.entity);
            device.released = false;
            device.dragging = false;
        }

        else if (device.dragging) {
            if (device.changed) {
                if (Registry.valid(device.lastPressed)) {
                    drag_entity(device, tool.entity);
                }

                else {
                    drag_nothing(device, tool.entity);
                }
            }
        }

        else {
            hover(device, tool.entity);
        }

        // In order to compute the delta between runs
        device._deltaPosition = device.position - device._lastPosition;
        device._lastPosition = device.position;

        // Lazily update intents
        device.changed = false;
    });
}


static void System() {
    MouseInputSystem();
}

}