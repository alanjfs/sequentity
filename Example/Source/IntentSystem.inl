struct MoveIntent {
    int x, y;
};

struct RotateIntent {
    int angle;
};

struct ScaleIntent {
    int scale;
};


static void IntentSystem() {
    Registry.view<MoveIntent, Position>().each([](const auto& intent, auto& position) {
        position += Position{ intent.x, intent.y };
    });

    Registry.view<RotateIntent, Orientation>().each([](const auto& intent, auto& orientation) {
        orientation += intent.angle;
    });

    Registry.view<ScaleIntent, Size>().each([](const auto& intent, auto& size) {
        size.x() += intent.scale;
        size.y() += intent.scale;

        if (size.x() < 5) size.x() = 5;
        if (size.y() < 5) size.y() = 5;
    });

    // All intents have been taking into account
    Registry.reset<MoveIntent>();
    Registry.reset<RotateIntent>();
    Registry.reset<ScaleIntent>();
}