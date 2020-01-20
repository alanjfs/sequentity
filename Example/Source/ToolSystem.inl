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

struct CanRecord {};

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
    entt::entity target;
};


struct Data {
    int time;
    int startTime;
    int endTime;

    std::map<int, InputPosition2D> positions;
};


static const char* tooltype_to_char(Type type) {
    return type == Type::Select ? "Select" :
           type == Type::Translate ? "Translate" :
           type == Type::Rotate ? "Rotate" :
           type == Type::Scale ? "Scale" :
           type == Type::Scrub ? "Scrub" :
                                 "Unknown";
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

    auto begin = [](auto entity, const auto& info, const auto& data) {
        Registry.reset<Selected>();
        Registry.assign<Selected>(info.target);
        Registry.assign_or_replace<CanRecord>(entity);
    };

    auto preview = [](const Info& info, const Data& data) {
        Registry.assign_or_replace<Tooltip>(info.target, "Drag to translate");
    };

    auto update = [](const Info& info, const Data& data, const auto& intent) {
        const InputPosition2D& position = (*data.positions.find(intent.time)).second;

        if (!Registry.has<Intent::Move>(info.target)) {
            Registry.assign<Intent::Move>(info.target, position.delta.x(), position.delta.y());
        }

        else {
            auto& move = Registry.get<Intent::Move>(info.target);
            move.x += position.delta.x();
            move.y += position.delta.y();
        }
    };

    Registry.view<Translate, SetupIntent>().less(setup);
    Registry.view<Translate, PrimaryIntent, BeginIntent, Info, Data>().less(begin);
    Registry.view<Translate, PrimaryIntent, PreviewIntent, Info, Data>().less(preview);
    Registry.view<Translate, PrimaryIntent, Info, Data, UpdateIntent>().less(update);
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
    auto begin = [](auto entity, const auto& info, const auto& data) {
        Registry.reset<Selected>();
        Registry.assign<Selected>(info.target);
        Registry.assign_or_replace<CanRecord>(entity);
    };

    auto preview = [](const auto& info, const auto& data) {
        Registry.assign_or_replace<Tooltip>(info.target, "Drag to rotate");
    };

    auto update = [](const auto& info, const auto& data, const auto& intent) {
        const InputPosition2D& position = (*data.positions.find(intent.time)).second;

        if (!Registry.has<Intent::Rotate>(info.target)) {
            Registry.assign<Intent::Rotate>(info.target, position.delta.x());
        
        // Intent may already have been added by an event or simultaneous tool
        } else {
            auto& rotate = Registry.get<Intent::Rotate>(info.target);
            rotate.angle += position.delta.x();
        }
    };

    auto finish = [](const auto& info, const auto& data) {
    };

    Registry.view<Rotate, PrimaryIntent, Info, Data, BeginIntent>().less(begin);
    Registry.view<Rotate, PrimaryIntent, Info, Data, PreviewIntent>().less(preview);
    Registry.view<Rotate, PrimaryIntent, Info, Data, UpdateIntent>().less(update);
    Registry.view<Rotate, PrimaryIntent, Info, Data, FinishIntent>().less(finish);
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
    auto begin = [](auto entity, const auto& info, const auto& data) {
        Registry.reset<Selected>();
        Registry.assign<Selected>(info.target);
        Registry.assign_or_replace<CanRecord>(entity);
    };

    auto preview = [](const auto& info, const auto& data) {
        Registry.assign_or_replace<Tooltip>(info.target, "Drag to scale");
    };

    auto update = [](const auto& info, const auto& data, const auto& intent) {
        const auto& position = (*data.positions.find(intent.time)).second;

        if (!Registry.has<Intent::Scale>(info.target)) {
            Registry.assign<Intent::Scale>(info.target, position.delta.x());

        } else {
            auto& scale = Registry.get<Intent::Scale>(info.target);
            scale.scale += position.delta.x();
        }
    };

    auto finish = [](const auto& info, const auto& data) {
    };

    Registry.view<Scale, PrimaryIntent, Info, Data, BeginIntent>().less(begin);
    Registry.view<Scale, PrimaryIntent, Info, Data, PreviewIntent>().less(preview);
    Registry.view<Scale, PrimaryIntent, Info, Data, UpdateIntent>().less(update);
    Registry.view<Scale, PrimaryIntent, Info, Data, FinishIntent>().less(finish);
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
    auto begin = [](auto entity, const auto& info, const auto& data) {
        auto [name, color] = Registry.get<Name, Color>(info.target);

        if (!Registry.has<Sequentity::Track>(info.target)) {
            Registry.assign<Sequentity::Track>(info.target, name.text, color);
        }

        auto& track = Registry.get<Sequentity::Track>(info.target);
        auto& channel = Sequentity::PushChannel(track, info.eventType, { info.name, info.color });
        Sequentity::PushEvent(channel, {
            data.startTime,
            1,          /* length= */
            color,
            info.eventType,

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
                        event.length = data.endTime - data.startTime + 1;
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

        // TODO: This doesn't prevent against primary and secondary
        //       intents being assigned simultaneously.
        Registry.remove<CanRecord>(entity);
    };

    Registry.view<RecordIntent, CanRecord, BeginIntent, Info, Data>().less(begin);
    Registry.view<RecordIntent, CanRecord, UpdateIntent, Data>().less(update);
    Registry.view<RecordIntent, CanRecord, FinishIntent>().less(finish);
}


void SelectSystem() {
    auto begin = [](const auto& info) {
        Registry.reset<Selected>();
        Registry.assign<Selected>(info.target);
    };

    Registry.view<Scale, PrimaryIntent, Info, BeginIntent>().less(begin);
}


void ScrubSystem() {
    auto update = [](const auto& data, const auto& intent) {
        Debug() << "Scrubbing..";
        const auto& position = (*data.positions.find(intent.time)).second;
        auto& sqty = Registry.ctx<Sequentity::State>();
        sqty.current_time += position.delta.x();
    };

    Registry.view<Scale, PrimaryIntent, Data, UpdateIntent>().less(update);
}


void System() {
    TranslateSystem();
    RotateSystem();
    ScaleSystem();
    SelectSystem();
    ScrubSystem();
    RecordSystem();

    Registry.reset<SetupIntent>();
    Registry.reset<BeginIntent>();
    Registry.reset<UpdateIntent>();
    Registry.reset<FinishIntent>();
    Registry.reset<PreviewIntent>();
}

}