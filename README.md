<p align=center><img width=700 src=https://user-images.githubusercontent.com/2152766/72180447-1c13ba80-33df-11ea-99f0-1dc1dae2f60c.png></p>
<p align=center>A single-file, immediate-mode sequencer widget for C++17, Dear ImGui and EnTT</p>
<p align=center><img width=800 src=https://user-images.githubusercontent.com/47274066/72092588-8d356e00-330a-11ea-8e83-007bb59aaf45.png></p>

<br>

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

### Try it

You could build it, or you can download a pre-built version and fool around.

- [`sequentity-0.1-alpha-windows.zip` (1.0 mb)](https://github.com/alanjfs/sequentity/releases/download/0.1-alpha/sequentity-0.1-alpha-windows.zip)

**Quickstart**

| Key | Description
|:----------|:---------
| **QWER** | Switch between tools
| **Click + Drag** | Manipulate the coloured squares
| **Click + Drag** | Move events around in time
| **Alt + Click + Drag** | Pan in the Event Editor
| **Delete** | Delete all events
| **Space** | Play and pause
| **F1** | Toggle the ImGui performance window
| **F2** | Toggle the Theme Editor
| **Backspace** | (debug) Pause the rendering loop
| **Enter** | (debug) Redraw one frame

<br>
<br>

### Overview

It's an sequence editor, in the spirit of MIDI authoring software like Ableton Live, Bitwig and FL Studio, where each event carry a start time, a duration and handle to your custom application data.

> **Heads up!**
>
> This is a work in progress, alpha at best, and is going through changes (that you are welcome to participate in!)

What makes Sequentity different, and inspired its name, is that it is built as an Entity-Component-System (ECS), and events are a combination of start time, length and custom application data; as opposed to individual events for start and end; suitable for e.g. (1) create a dynamic rigid body, (2) edit said body, whilst maintaining reference to what got created and (3) delete the body at the end of the event.

```cpp
entt::registry& registry;

auto entity = registry.create();
auto& track = registry.assign<Sequentity::Track>(entity, "My first track");
auto& channel = Sequentity::PushChannel(track, MyEventType, "My first channel");
auto& event = Sequentity::PushEvent(channel, 10, 5); // time, length

while (true) {
    ImGui::Begin("Event Editor");
    Sequentity::EventEditor(registry);
    ImGui::End();
}
```

**What can I use it for?**

If you need to record anything in your application, odds are you need to play something back. If so, you may also need to edit what got recorded, in which case you can use something like Sequentity.

I made this for recording user input in order to recreate application state exactly such that I could record once more, on-top of the previous recording; much like how musicians record over themselves with various instruments to produce a complete song. You could theoretically decouple the clock-time aspect and use this as playback mechanism for undo/redo, similar to what ZBrush does, and save that with your scene/file. Something I intend on experimenting with!

**Goals**

- Build upon the decades of UI/UX design found in DAWs like Ableton Live and Bitwig
- Visualise 1-100'000 events simultaneosuly, with LOD if necessary
- No more than 1 ms per call on an Intel-level GPU
- Fine-grained edits to properties of individual events up close
- Coarse-grained bulk-edits to thousands of events from afar

**Is there anything similar?**

I'm sure there are, however I was only able to find one standalone example, and only a few others embedded in open source applications.

If you know any more, please let me know by filing an issue!

- [ImSequencer](https://github.com/CedricGuillemet/ImGuizmo#imsequencer)
- [LumixEngine](https://github.com/ocornut/imgui/issues/772#issuecomment-238243554)
- [@citruslee snippet](https://github.com/ocornut/imgui/issues/539#issuecomment-195507384)
- Your suggestion here

Finally, there are others with a similar interface but different implementation and goal.

- [Helio Workstation](https://github.com/helio-fm/helio-workstation)
- Your suggestion here

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
- [ ] **Group Events** For drawing and manipulating multiple groups of entities together
- [ ] **Mini-map** Like the one in Sublime Text; for when you've got lots of events
- [ ] **Event Scaling** Sometimes, the input is good, but could use some fine-tuning
- [ ] **Event Cropping** Other times, part of the input isn't worth keeping
- [ ] **Track Folding** For when you have way too much going on
- [ ] **Track, Channel and Event Renaming** Get better organised
- [ ] **Custom Event Tooltip** Add a reminder for yourself or others about what an event is all about
- [ ] **Event Vertical Move** Implement moving of events between tracks (i.e. vertically)
- [ ] **Zoom** Panning works by holding ALT while click+dragging. Zooming needs something like that.
- [ ] **One-off events** Some things happen instantaneously

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

- **Start, End and Beyond** Events are currently authored and stored in memory like they appear in the editor; but in your typical MIDI editor the original events don't look like this. Instead, events are standalone, immutable. An editor, like Cubase, then draws each consecutive start and end pair as a single bar for easy selection and edits. But do they store it in memory like this? I found it challenging to keep events coming in from the application together. For example, if I click and drag with the mouse, and then click with my Wacom tabled whilst still holding down the mouse, I would get a new click event in the midst of drag events, without any discernable way to distinguish the origin of each move event. MIDI doesn't have this problem, as an editor typically pre-selects from which device to expect input. But I would very much like to facilitate multiple mice, simultaneous Wacom tablets, eye trackers and anything capable of generating interesting events.
- **How do we manage selection?** Sequentity manages the currently selected event using a raw pointer in its own `State` component, is there a better way? We couldn't store selection state in an event itself, as they aren't the ones aware of whether their owner has got them selected or not. It's outside of their responsibility. And besides, it would mean we would need to iterate over all events to deselect before selecting another one, what a waste.

On top of these, there are some equivalent [Application Open Questions](https://github.com/alanjfs/sequentity/blob/master/Example/README.md#open-questions) for the Tools and Input handling which I would very much like your feedback on.

<br>
<br>

### Install

Sequentity is distributed as a single-file library, with the `.h` and `.cpp` files combined.

1. Copy [`Sequentity.h`](https://raw.githubusercontent.com/alanjfs/sequentity/master/Sequentity.h) into your project
2. `#define SEQUENTITY_IMPLEMENTATION` in *one* of your `.cpp` files
2. `#include <Sequentity.h>`
4. [See below](#usage)

**Dependencies**

- [ImGui](https://github.com/ocornut/imgui) Which is how drawing and user input is managed
- [EnTT](https://github.com/skypjack/entt) An ECS framework, this is where and how data is stored.

<br>
<br>

### Usage

Sequentity can draw events in time, and facilitate edits to be made to those events interactively by the user. It doesn't know nor care about playback, that part is up to you.

<details><summary>New to <b><a href="https://github.com/skypjack/entt">EnTT</a></b>?</summary>

### An EnTT Primer

Here's what you need to know about [EnTT](https://github.com/skypjack/entt) in order to use Sequentity.

1. EnTT (pronounced "entity") is an ECS framework
2. ECS stands for Entity-Component-System
3. `Entities` are identifiers for "things" in your application, like a character, a sound or UI element
4. `Components` carry the data for those things, like the `Color`, `Position` or `Mesh`
5. `Systems` operate on that data in some way, such as adding `+1` to `Position.x` each frame

It works like this.

```cpp
// You create a "registry"
auto registry = entt::registry;

// Along with an entity
auto entity = registry.create();

// Add some data..
struct Position {
    float x { 0.0f };
    float y { 0.0f };
};
registry.assign<Position>(entity, 5.0f, 1.0f);  // 2nd argument onwards passed to constructor

// ..and then iterate over that data
registry.view<Position>().each([](auto& position) {
    position.x += 1.0f;
});
```

A "registry" is what keeps track of what entities have which components assigned, and "systems" can be as simple as a free function. I like to think of each loop as its own system, like that one up there iterating over positions. Single reponsibility, and able to perform complex operations that involve multiple components.

Speaking of which, here's how you combine components.

```cpp
registry.view<Position, Color>().each([](auto& position, const auto& color) {
    position.x += color.r;
});
```

This function is called on every entity with both a position and color, and combines the two.

Sequentity then is just another component.

```cpp
registry.assign<Sequentity::Track>(entity);
```

This component then stores all of the events related to this entity. When the entity is deleted, the `Track` is deleted alongside it, taking all of the events of this entity with it.

```cpp
registry.destroy(entity);
```

You could also keep the entity, but erase the track.

```cpp
registry.remove<Sequentity::Track>(entity);
```

And when you're fed up with entities and want to go home, then just:

```cpp
registry.clear();
```

And that's about it as far as Sequentity goes, have a look at the [EnTT Wiki](https://github.com/skypjack/entt/wiki) along with [my notes](https://github.com/alanjfs/entt/wiki) for more about EnTT. Have fun!

</details>

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

auto& event = Sequentity::PushEvent(channel); {
    event.time = 1;
    event.length = 50;
    event.color ImColor::HSV(0.66f, 0.5f, 0.75f);
}

// Draw it!
Sequentity::EventEditor(registry);
```

And here's how you query.

```cpp
const int time { 13 };
Sequentity::Intersect(track, time, [](const auto& event) {
    if (event.type == MyEventType) {

        // Do something interesting
        event.time;
        event.length;
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

<img width=600 src=https://user-images.githubusercontent.com/2152766/72179629-39e02000-33dd-11ea-929f-1196c2eed2f5.png>
