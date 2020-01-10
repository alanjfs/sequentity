<img width=800 src=https://user-images.githubusercontent.com/47274066/72092588-8d356e00-330a-11ea-8e83-007bb59aaf45.png>

<br>
<br>

### Sequentity (currently in alpha)

A single-file, immediate-mode sequencer widget for C++17, Dear ImGui and EnTT

**Table of Contents**

- [Overview](#overview)
- [Features](#features)
- [Design Decisions](#example)
- [Todo](#todo)
- [Open Questions](#open-questions)
- Documentation
    - [Install](#install)
    - [Usage](#example)
    - [Event Handlers](#event-handlers)
    - [Sorting](#sorting)
    - [State](#state)
    - [Components](#components)
    - [Serialisation](#serialisation)
- [Example](https://github.com/alanjfs/sequentity/blob/master/Example)

<br>
<br>

### Overview

It's an sequence editor, in the spirit of MIDI authoring software like Ableton Live, Bitwig and FL Studio, where each event carry a start time, a duration and handle to your custom application data.

What makes Sequentity different, and inspired its name, is that it is built as an Entity-Component-System (ECS), and events are a combination of start time, length and custom application data; as opposed to individual events for start and end; suitable for e.g. (1) create a dynamic rigid body, (2) edit said body, whilst maintaining reference to what got created and (3) delete the body at the end of the event.

```cpp
entt::registry& registry;

auto entity = registry.create();
auto& track = registry.assign<Sequentity::Track>(entity, "My first track");
auto& channel = Sequentity::PushChannel(track, "My first channel");
auto& event = Sequentity::PushEvent(channel, 10, 5); // time, length

while (true) {
    Sequentity::EventEditor(registry);
}
```

**What can I use it for?**

If you need to record anything in your application, odds are you need to play something back. If so, you may also need to edit what got recorded, in which case you can use something like Sequentity.

I made this for recording user input in order to recreate application state exactly such that I could record once more, on-top of the previous recording; much like how muscicians record over themselves with various instruments to produce a complete song. You could theoretically decouple the clock-time aspect and use this as playback mechanism for undo/redo, similar to what ZBrush does, and save that with your scene/file. Something I intend on experimenting with!

**Is there anything similar?**

I'm sure there are, however I was only able to find one standalone example, and only a few others embedded in open source applications.

If you know any more, please let me know by filing an issue!

- https://github.com/CedricGuillemet/ImGuizmo#imsequencer
- https://github.com/ocornut/imgui/issues/772#issuecomment-238243554
- https://github.com/ocornut/imgui/issues/539#issuecomment-195507384

<br>
<br>

### Features

- [x] **Per-event application data** Attach any of your application data to an event, and retrieve it later
- [x] **Per-event, channel and track coloring** To stay organised with lots of incoming data
- [x] **Consolidated Events** Individual events with a start and length, as opposed separate events for start and end
- [x] **Overlapping Events** Author events 
- [x] **Single-file library** Distributed as [a single `.h`](https://github.com/alanjfs/sequentity/blob/master/Sequentity.h) file of `~1'000` lines for easy inclusion into your project
- [ ] **Cross-fade** Overlap the end of one event with the start of another for a cross-blend, to do e.g. linear interpolation
- [ ] **Event Priority** When events overlap, determine the order in which they are processed

![sequentitydemo1](https://user-images.githubusercontent.com/47274066/72174000-a4d72a00-33d0-11ea-8f40-f60cb7b993b7.gif) ![sequentitydemo3](https://user-images.githubusercontent.com/47274066/72174039-b91b2700-33d0-11ea-88db-d50305e06671.gif) ![sequentitydemo2](https://user-images.githubusercontent.com/47274066/72174005-a86ab100-33d0-11ea-9a50-93ef786a06db.gif) ![sequentity_zooming](https://user-images.githubusercontent.com/47274066/72174265-347cd880-33d1-11ea-96ee-ddea31bfbcec.gif) ![sequentitydemo4](https://user-images.githubusercontent.com/47274066/72174071-cb956080-33d0-11ea-9cb4-24c3974c8ada.gif) ![sequentitydemo6](https://user-images.githubusercontent.com/47274066/72174081-cf28e780-33d0-11ea-9091-de3010d3f79f.gif)

<br>
<br>

### Design Decisions

- **No class instance** ImGui widgets generally don't require an instance, and neither does Sequentity
- **Events -> Channels -> Tracks** Events are organised into these three groups
- **Integer Event Type** Leaving definition and interpretation of types to the application author
- **Integer Time** Time is represented as samples, rather than frames`*`
- **1 Entity, 1 Track** Events ultimately operate on components relative some entity
- **`void*` for application data** In search of a better alternative, as it complicates cleanup. Let me know!
- **No clock time** The application is responsible for managing the event loop

> `*` The difference being that a `sample` is a complete snapshot of your application/game state, whereas a `frame` is a (potentially fractal) point in time, e.g. `1.351f`

<br>
<br>

### Todo

These are going into GitHub issues shortly.

- [ ] **Sequentity::CreateEvent()** See if you can move some of the repetitive and unsafe logic to the library
- [ ] **Custom deleter for application data** See [suggestion here](https://github.com/alanjfs/sequentity/commit/96cff16e1520e7a73ff4a622b58d92d6083a8648#r36675122)
- [ ] **Effects**
- [ ] **Group Events** For drawing and manipulating multiple groups of entities together
- [ ] **Sequentity::CurveEditor()** For visualising and manipulating user data, like position or rotation over time
- [ ] **Sequentity::ClipEditor()** For storing and reusing events or groups, like in Ableton Live
- [ ] **Sequentity::ArrangementEditor()** For arranging multiple clips along a global timeline
- [ ] **Mini-map** Like the one in Sublime Text; for when you've got lots of events
- [ ] **Event Scaling** Sometimes, the input is good, but could use some fine-tuning
- [ ] **Event Cropping** Other times, part of the input isn't worth keeping
- [ ] **Track Folding** For when you have way too much going on
- [ ] **Track Rename** Get better organised
- [ ] **Channel Rename**
- [ ] **Event Vertical Move** Implement moving of events between channels and tracks (i.e. vertically)
- [ ] **Zoom** Panning works by holding ALT while click+dragging. Zooming needs something like that.
- [ ] **Stride** There are a few values that work, but make no sense, like `stride`
- [ ] **Bug, hot-swap tool** Translate something and switch tool without letting go
- [ ] **Bug, event at end** Click to add an event on the end frame, and it'll create one erroneously
- [ ] **Cosmetics, transitions** Duration of transitions is based on a solid 60 fps, should be relative wallclock time
- [x] **Refactor, Unify data types** Data types in Sequentity is a mixture of native, Magnum and ImGui.
- [x] **Smooth Panning and Zooming** Any change to these have a nice smoothing effect
- [x] **Drag Current Time** You can, but it won't trigger the time-changed callback

<br>
<br>

### Open Questions

I made Sequentity for another (commercial) project, but made it open source in order to seek help from the open source community. This is my first sequencer-like project and in fact my first C++ project (with <4 months of experience using the language), so I expect lots of things to be ripe for improvement.

Here are some of the things I'm actively looking for answers to and that you are welcome to strike up a dialog about in a new issue. (Thank you!)

- **Who's responsible for managing memory?** Application data is associated with an event via a `void*`; it complicates memory management, as it involves a `new` and `delete` which is currently explicitly defined in the application. Error prone.
- **How should events be authored?** Data for Sequentity is created by the application, and it's rather easy to mess it up. E.g. forget to assign a label or color to a given channel. Or assign two overlapping events. This could be abstracted into e.g. `Sequentity::Interface::create_event()` but it would mean either (1) making a component out of it such that we can read it from the application, or pass around an instance of Sequentity itself. Maybe Sequentity could be listening for a `CreateEvent` component with relevant data to create an event itself, and do any additional initialisation from there?
- **How do we manage selection?** Sequentity manages the currently selected event using a raw pointer in its own `State` component, is there a better way? We couldn't store selection state in an event itself, as they aren't the ones aware of whether their owner has got them selected or not. It's outside of their responsibility. And besides, it would mean we would need to iterate over all events to deselect before selecting another one, what a waste.

On top of these, there are some equivalent [Application Open Questions](https://github.com/alanjfs/sequentity/blob/master/Example/README.md#open-questions) for the Tools and Input handling which I would very much like your feedback on.

<br>
<br>

### Install

Sequentity is distributed as a single-file library, with the `.h` and `.cpp` files combined.

1. Copy [`Sequentity.h`](https://raw.githubusercontent.com/alanjfs/sequentity/master/Sequentity.h) into your project
2. `#include <Sequentity.h>`
4. [See below](#usage)

<br>
<br>

### Usage

Sequentity can draw events in time, and facilitate edits to be made to those events interactively by the user. It doesn't know nor care about playback, that part is up to you.

Here's how you draw.

```cpp
// Author some data
entt::registry& registry;
entity = registry.create();

// Events may carry application data and a type for you to identify it with
struct MyEventData {
    float value { 0.0f };
};

enum {
    MyEventType = 0
};

auto& track = registry.assign<Sequentity::Track>(entity); {
    track.label = "My first track";
    track.color = ImColor::HSV(0.0f, 0.5f, 0.75f);
}

auto& channel = Sequentity::PushChannel(track, MyEventType); {
    channel.label = "My first channel";
    channel.color = ImColor::HSV(0.33f, 0.5f, 0.75f);
}

// Don't forget to delete this
auto* data = new MyEventData{ 5.0f };

auto& event = Sequentity::PushEvent(channel); {
    event.time = 1;
    event.length = 50;
    event.color ImColor::HSV(0.66f, 0.5f, 0.75f);

    // Application data, Sequentity won't touch it,
    // which means it's your memory to manage
    event.type = MyEventType;
    event.data = static_cast<void*>(data);
}

// Draw it!
Sequentity::EventEditor(registry);
```

And here's how you query.

```cpp
const int time { 13 };
Sequentity::Intersect(track, time, [](const auto& event) {
    if (event.type == MyEventType) {
        auto& data = static_cast<MyEventData*>(event.data);

        // Do something with it
        event.time
        event.length
    }
});
```

The example application uses events for e.g. translations, storing a vector of integer pairs representing position. For each frame, data per entity is retrieved from the current event and correlated to a position by computing the time relative the start of an event.

<br>

#### Event Handlers

What you do with events is up to you, but I would recommend you establish so-called "event handlers" for the various types you define.

For example, if you define Translate, Rotate and Scale event types, then you would need:

1. Something to produce these
2. Something to consume these

Producers in the example applications are so-called "Tools" and operate based on user input like the current mouse position. The kind of tool isn't necessarily bound or even related to the type of event it produces. For example, a `TranslateTool` would likely generate events of type `TranslateEvent` with `TranslateEventData`, whereby you may establish an equivalent `TranslateEventHandler` to interpret this data.

```cpp
enum EventTypes_ : Sequentity::EventType {
    TranslateEvent = 0;
};

struct TranslateEventData {
    int x;
    int y;
};

void TranslateEventHandler(entt::entity entity, const Sequentity::Event& event, int time) {
    auto& position = Registry.get<Position>(entity);
    auto& data = static_cast<TranslateEventData*>(event.data);
    // ...
}
```

<br>

#### Sorting

Tracks are sorted in the order of their EnTT pool.

```cpp
Registry.sort<Sequentity::Track>([this](const entt::entity lhs, const entt::entity rhs) {
    return Registry.get<Index>(lhs) < Registry.get<Index>(rhs);
});
```

<br>

#### State

State - such as the zoom level, scroll position, current time and min/max range - is stored in your EnTT registry which is (optionally) accessible from anywhere. In the example application, it is used to draw the Transport panel with play, stop and visualisation of current time.

```cpp
auto& state = registry.ctx<Sequentity::State>();
```

When state is automatically created by Sequentity if you haven't already done so. You may want to manually create state for whatever reason, which you can do like this.

```cpp
auto& state = registry.set<Sequentity::State>();
state.current_time = 10;

// E.g.
Sequentity::EventEditor(registry);
```

To draw the event editor with the current time set to 10.

<br>

#### Components

Sequentity provides 1 ECS component, and 2 additional inner data structures.

```cpp
/**
 * @brief A Sequentity Event
 *
 */
struct Event {
    TimeType time { 0 };
    TimeType length { 0 };

    ImVec4 color { ImColor::HSV(0.0f, 0.0f, 1.0f) };

    // Map your custom data here, along with an optional type
    EventType type { EventType_Move };
    void* data { nullptr };

    /**
     * @brief Ignore start and end of event
     *
     * E.g. crop = { 2, 4 };
     *       ______________________________________
     *      |//|                              |////|
     *      |//|______________________________|////|
     *      |  |                              |    |
     *      |--|                              |----|
     *  2 cropped from start             4 cropped from end
     *
     */
    TimeType crop[2] { 0, 0 };

    /* Whether or not to consider this event */
    bool enabled { true };

    /* Events are never really deleted, just hidden from view and iterators */
    bool removed { false };

    /* Extend or reduce the length of an event */
    float scale { 1.0f };

    // Visuals, animation
    float height { 0.0f };
    float thickness { 0.0f };

};

/**
 * @brief A collection of events
 *
 */
struct Channel {
    const char* label { "Untitled channel" };

    ImVec4 color { ImColor::HSV(0.33f, 0.5f, 1.0f) };

    std::vector<Event> events;
};


/**
 * @brief A collection of channels
 *
 */
struct Track {
    const char* label { "Untitled track" };

    ImVec4 color { ImColor::HSV(0.66f, 0.5f, 1.0f) };

    bool solo { false };
    bool mute { false };

    std::unordered_map<EventType, Channel> channels;
    
    // Internal
    bool _notsoloed { false };
};
```

<br>

#### Serialisation

All data comes in the form of components with plain-old-data, including state like panning and zooming.

TODO

<br>

### Roadmap

See [Todo](#todo) for now.
