namespace Intent {


struct Move {
    int x, y;
};

struct Rotate {
    int angle;
};

struct Scale {
    int scale;
};


struct SortTracks {};


static void System() {
    Registry.view<Move, Position>().each([](const auto& intent, auto& position) {
        position += Position{ intent.x, intent.y };
    });

    Registry.view<Rotate, Orientation>().each([](const auto& intent, auto& orientation) {
        orientation += intent.angle;
    });

    Registry.view<Scale, Size>().each([](const auto& intent, auto& size) {
        size.x() += intent.scale;
        size.y() += intent.scale;

        if (size.x() < 5) size.x() = 5;
        if (size.y() < 5) size.y() = 5;
    });

    Registry.view<SortTracks>().less([]() {
        Registry.sort<Sequentity::Track>([](const entt::entity lhs, const entt::entity rhs) {
            return Registry.get<Index>(lhs) < Registry.get<Index>(rhs);
        });
    });

    // All intents have been taking into account
    Registry.reset<Move>();
    Registry.reset<Rotate>();
    Registry.reset<Scale>();
    Registry.reset<SortTracks>();
}

}