#include <vector>
#include <array>
#include <map>

struct Tooltip { const char* text; };

namespace Tool {

enum class Type : std::uint8_t {
    None = 0,
    Select,
    DragSelect,
    LassoSelect,

    Translate,
    Rotate,
    Scale,

    Scrub,
};


// Possible event types
enum EventType : Sequentity::EventType {
    InvalidEvent = 0,  // Catch uninitialised types

    SelectEvent,
    LassoSelectEvent,
    DragSelectEvent,

    TranslateEvent,
    RotateEvent,
    ScaleEvent,

    ScrubEvent,
};


enum State : std::uint8_t {
    ToolState_None = 0,
    ToolState_Activated,
    ToolState_Active,
    ToolState_Deactivated
};


// Tool stage intentions
struct SetupIntent {};
struct BeginIntent {};
struct UpdateIntent { int time; };
struct PreviewIntent {};
struct FinishIntent {};
struct RecordIntent {};


// Tool mode intentions
struct PrimaryIntent {};    // E.g. left mouse button
struct SecondaryIntent {};  // E.g. right mouse button
struct TertiaryIntent {};   // E.g. middle mouse button


// Possible tools
struct Select {};
struct Scrub {};
struct Translate {};
struct Rotate {};
struct Scale {};


struct Info {
    const char* name;
    ImVec4 color;
    Type type;
    EventType eventType;
};


struct Data {
    entt::entity target;

    int time;
    int startTime;
    int endTime;

    std::map<int, InputPosition2D> positions;
};


static const char* tooltype_to_char(Type type) {
    return type == Type::Select ? "Type::Select" :
           type == Type::Translate ? "Type::Translate" :
           type == Type::Rotate ? "Type::Rotate" :
           type == Type::Scale ? "Type::Scale" :
           type == Type::Scrub ? "Type::Scrub" :
                                 "Type::Unknown";
}


static const char* eventtype_to_char(EventType type) {
    return type == SelectEvent ? "SelectEvent" :
           type == TranslateEvent ? "TranslateEvent" :
           type == RotateEvent ? "RotateEvent" :
           type == ScaleEvent ? "ScaleEvent" :
           type == ScrubEvent ? "ScrubEvent" :
                                "UnknownEvent";
}


/**
 * @brief Translate an entity
 *
 *      __________ 
 *     |          |
 *     |          | ----------->   
 *     |          |
 *     |__________|
 *
 *
 */
void TranslateSystem() {
    auto setup = []() {
        Debug() << "Setting up Translate tool!";
    };

    auto begin = [](const Data& data) {
        Registry.reset<Selected>();
        Registry.assign<Selected>(data.target);

        // Carry on during update
        Registry.assign<UpdateIntent>(data.target, data.time);
    };

    auto preview = [](const Data& data) {
        Registry.assign_or_replace<Tooltip>(data.target, "Drag to translate");
    };

    auto update = [](const Data& data, const auto& intent) {
        const InputPosition2D& position = (*data.positions.find(intent.time)).second;

        if (!Registry.has<Intent::Move>(data.target)) {
            Registry.assign<Intent::Move>(data.target, position.delta.x(), position.delta.y());
        }

        else {
            auto& move = Registry.get<Intent::Move>(data.target);
            move.x += position.delta.x();
            move.y += position.delta.y();
        }
    };

    auto finish = [](const auto& data) {
    };

    Registry.view<Translate, SetupIntent>().less(setup);
    Registry.view<Translate, PrimaryIntent, BeginIntent, Data>().less(begin);
    Registry.view<Translate, PrimaryIntent, PreviewIntent, Data>().less(preview);
    Registry.view<Translate, PrimaryIntent, Data, UpdateIntent>().less(update);
    Registry.view<Translate, PrimaryIntent, FinishIntent, Data>().less(finish);
}


/**
 * @brief Rotate an entity
 *                  __
 *      __________     \
 *     |          |     v
 *     |          |   
 *     |          |
 *     |__________|
 *  ^
 *   \___
 *
 */
void RotateSystem() {
    auto begin = [](const auto& data) {
        Registry.reset<Selected>();
        Registry.assign<Selected>(data.target);

        // Carry on during update
        Registry.assign<UpdateIntent>(data.target, data.time);
    };

    auto preview = [](const auto& data) {
        Registry.assign_or_replace<Tooltip>(data.target, "Drag to rotate");
    };

    auto update = [](const auto& data, const auto& intent) {
        const InputPosition2D& position = (*data.positions.find(intent.time)).second;

        if (!Registry.has<Intent::Rotate>(data.target)) {
            Registry.assign<Intent::Rotate>(data.target, position.delta.x());
        
        // Intent may already have been added by an event or simultaneous tool
        } else {
            auto& rotate = Registry.get<Intent::Rotate>(data.target);
            rotate.angle += position.delta.x();
        }
    };

    auto finish = [](const auto& data) {
    };

    Registry.view<Rotate, PrimaryIntent, Data, BeginIntent>().less(begin);
    Registry.view<Rotate, PrimaryIntent, Data, PreviewIntent>().less(preview);
    Registry.view<Rotate, PrimaryIntent, Data, UpdateIntent>().less(update);
    Registry.view<Rotate, PrimaryIntent, Data, FinishIntent>().less(finish);
}


/**
 * @brief Scale an entity
 *
 *   \              /
 *    \ __________ /
 *     |          |
 *     |          |
 *     |          |
 *     |__________|
 *    /            \
 *   /              \
 *
 */
void ScaleSystem() {
    auto begin = [](const auto& data) {
        Registry.reset<Selected>();
        Registry.assign<Selected>(data.target);

        // Carry on during update
        Registry.assign<UpdateIntent>(data.target, data.time);
    };

    auto preview = [](const auto& data) {
        Registry.assign_or_replace<Tooltip>(data.target, "Drag to scale");
    };

    auto update = [](const auto& data, const auto& intent) {
        const auto& position = data.positions.at(intent.time);

        if (!Registry.has<Intent::Scale>(data.target)) {
            Registry.assign<Intent::Scale>(data.target, position.delta.x());

        } else {
            auto& scale = Registry.get<Intent::Scale>(data.target);
            scale.scale += position.delta.x();
        }
    };

    auto finish = [](const auto& data) {
    };

    Registry.view<Scale, PrimaryIntent, Data, BeginIntent>().less(begin);
    Registry.view<Scale, PrimaryIntent, Data, PreviewIntent>().less(preview);
    Registry.view<Scale, PrimaryIntent, Data, UpdateIntent>().less(update);
    Registry.view<Scale, PrimaryIntent, Data, FinishIntent>().less(finish);
}


/**
 * @brief Record a tool
 *
 *      __________ 
 *     |          |
 *     |          | . . . . o
 *     |          |
 *     |__________|
 *
 *
 */
void RecordSystem() {
    auto begin = [](auto entity, const auto& meta, const auto& data) {
        auto [name, color] = Registry.get<Name, Color>(data.target);

        if (!Registry.has<Sequentity::Track>(data.target)) {
            Registry.assign<Sequentity::Track>(data.target, name.text, color);
        }

        auto& track = Registry.get<Sequentity::Track>(data.target);
        auto& channel = Sequentity::PushChannel(track, meta.eventType, { meta.name, meta.color });
        auto& event = Sequentity::PushEvent(channel, {
            data.startTime,
            1,          /* length= */
            color,
            meta.eventType,

            // Leave a breadcrumb for update and finish to latch onto
            entity
        });
    };

    auto update = [](auto entity, const auto& intent, const auto& data) {

        // TODO: Find some better way of getting hold of the event
        //       created in `begin()`
        Registry.view<Sequentity::Track>().each([&](auto& track) {
            for (auto& [type, channel] : track.channels) {
                for (auto& event : channel.events) {
                    if (event.payload == entity) {
                        event.length = data.endTime - data.startTime;
                    }
                }
            }
        });
    };

    auto finish = [](auto entity) {
        Registry.view<Sequentity::Track>().each([&](auto& track) {
            for (auto& [type, channel] : track.channels) {
                for (auto& event : channel.events) {
                    if (event.payload == entity) {

                        // Store a copy of this tool in the event
                        event.payload = Registry.create();

                        // IMPORTANT: Must stomp *after* accessing `event` as the
                        //            reference may dangle following this call
                        Registry.stomp(event.payload, entity, Registry);
                    }
                }
            }
        });
    };

    Registry.view<RecordIntent, BeginIntent, Info, Data>().less(begin);
    Registry.view<RecordIntent, UpdateIntent, Data>().less(update);
    Registry.view<RecordIntent, FinishIntent>().less(finish);
}


void System() {
    TranslateSystem();
    RotateSystem();
    ScaleSystem();
    // SelectSystem();
    // ScrubSystem();
    RecordSystem();

    Registry.reset<SetupIntent>();
    Registry.reset<BeginIntent>();
    Registry.reset<UpdateIntent>();
    Registry.reset<FinishIntent>();
    Registry.reset<PreviewIntent>();

    Registry.reset<PrimaryIntent>();
    Registry.reset<SecondaryIntent>();
}

}