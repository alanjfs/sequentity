struct MoveIntent {
    float x, y;
};

struct RotateIntent {
    float angle;
};

struct ScaleIntent {
    float scale;
};


// static void IntentSystem() {
//     Registry.view<MoveIntent, Position>().each([](const auto& intent, auto& position) {
//         position.x = intent.x;
//         position.y = intent.y;
//     });

//     Registry.view<RotateIntent, Orientation>().each([](const auto& intent, auto& orientation) {
//         orientation = intent.angle;
//     });

//     Registry.view<ScaleIntent, Size>().each([](const auto& intent, auto& size) {
//         size.x = intent.scale;
//         size.y = intent.scale;
//     });
// }