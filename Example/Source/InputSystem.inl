
// Hardware singletons
static struct MouseDevice_ {
    const char* id { "mouse" };

    entt::entity assignedTool { entt::null };

    Position position { 0, 0 };
    Position pressPosition { 0, 0 };
    Position lastPosition { 0, 0 };
    Position deltaPosition { 0, 0 };

    bool pressed { false };
    bool dragging { false };
    bool released { false };
    bool changed { false };

    entt::entity lastPressed { entt::null };
    entt::entity lastHovered { entt::null };

    Tool::InputPosition2D data;
} MouseDevice[1];


static struct WacomDevice_ {
    const char* id { "wacom" };
    entt::entity lastPressed { entt::null };
    Tool::InputPosition2D data;
} WacomDevice;


static struct GamepadDevice_ {
    const char* id { "gamepad" };
    entt::entity lastPressed { entt::null };
    Tool::InputPosition2D data;
} GamepadDevice;


static void MouseInputSystem() {
    auto press = [](MouseDevice_& device) {
        auto& app = Registry.ctx<ApplicationState>();
        auto entity = device.lastPressed;

        device.pressPosition = device.position;

        if (Registry.valid(entity)) {
            assert(!Registry.has<Tool::BeginIntent>(device.assignedTool));
            Registry.assign<Tool::BeginIntent>(device.assignedTool);

            auto& data = Registry.assign_or_replace<Tool::Data>(device.assignedTool);
            data.target = entity;
            data.time = app.time;
            data.startTime = app.time;
            data.input = device.data;
            data.inputs[app.time] = device.data;
        }
    };

    auto drag_entity = [](MouseDevice_& device) {
        auto& app = Registry.ctx<ApplicationState>();
        auto entity = device.lastPressed;

        // May be updated by events (that we would override)
        Registry.assign<Tool::UpdateIntent>(device.assignedTool);

        // Update data
        auto& data = Registry.get<Tool::Data>(device.assignedTool);
        data.time = app.time;
        data.endTime = app.time;
        data.input = device.data;
        data.inputs[app.time] = device.data;
    };

    auto drag_nothing = [](MouseDevice_& device) {
        // E.g. a selection rectangle
    };

    auto hover = [](MouseDevice_& device) {
        Registry.reset<Tooltip>();
        Registry.reset<Tool::Data>(device.assignedTool);

        auto entity = device.lastHovered;

        if (Registry.valid(entity)) {
            Registry.assign_or_replace<Tool::PreviewIntent>(device.assignedTool);

            auto& app = Registry.ctx<ApplicationState>();
            auto& data = Registry.assign_or_replace<Tool::Data>(device.assignedTool);
            data.target = entity;
            data.time = app.time;
            data.startTime = app.time;
            data.input = device.data;
            data.inputs[app.time] = device.data;
        }
    };

    auto release = [](MouseDevice_& device) {
        auto& app = Registry.ctx<ApplicationState>();
        auto entity = device.lastPressed;

        if (app.recording) {
            Registry.assign_or_replace<Tool::RecordIntent>(device.assignedTool);
        }
    };


    // Mappings
    for (auto& device : MouseDevice) {
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
        device.deltaPosition = device.position - device.lastPosition;
        device.lastPosition = device.position;
    }
}


static void InputSystem() {
    MouseInputSystem();
}
