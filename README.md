# NOT READY FOR THE PUBLIC EYE

Just started working on this, come back another time for images, docs and other goodies. :)

<br>
<br>

<img width=500 src=https://user-images.githubusercontent.com/47274066/72092588-8d356e00-330a-11ea-8e83-007bb59aaf45.png>

### Sequentity - An immediate-mode sequencer widget for Dear ImGui

It's a sequence editor, in the spirit of MIDI authoring software like Ableton Live, Bitwig and FL Studio, where events carry a start time, a duration and handle to your custom application data.

**Features**


- **Performance** Is performance a feature? Yes, yes it is.
- 

```cpp
while (true) {
    Sequentity::Sequentity(registry);
}
```

**Similar Projects**

- https://github.com/CedricGuillemet/ImGuizmo#imsequencer
- Your project here

**Libraries**

- ImGui (required)
- Magnum
- EnTT

**Features**

- [ ] **Selection** A `Selected` component determines whether a Track is selected

**Todo**

- [ ] **Transport** For playing, stopping and manipulating playback range
- [ ] **Group Events** For drawing and manipulating multiple groups of entities together
- [ ] **Clip Editor** For storing and reusing events or groups, like in Ableton Live
- [ ] **Curve Editor** For visualising and manipulating user data, like position or rotation over time
- [ ] **Session View** For storing and reusing events or groups
- [ ] **Event Scaling** Sometimes, the input is good, but could use some fine-tuning
- [ ] **Event Cropping** Other times, part of the input isn't worth keeping
- [ ] **Track Folding** For when you have way too much going on
- [ ] **Track Rename** Get better organised
- [ ] **Channel Rename**
- [ ] **Event Vertical Move** Implement moving of events between channels and tracks (i.e. vertically)
- [ ] **UX, Escape** Prevent events from escaping the current range
- [ ] **Zoom** Panning works by holding ALT while click+dragging. Zooming needs something like that.
- [ ] **Stride** There are a few values that work, but make no sense, like `stride`
- [ ] **Bug, hot-swap tool** Translate something and switch tool without letting go
- [ ] **Bug, event at end** Click to add an event on the end frame, and it'll create one erroneously
- [ ] **Cosmetics, transitions** Duration of transitions is based on a solid 60 fps, should be relative wallclock time
- [x] **Refactor, Unify data types** Data types in Sequentity is a mixture of native, Magnum and ImGui.
- [x] **Smooth Panning and Zooming** Any change to these have a nice smoothing effect
- [x] **Drag Current Time** You can, but it won't trigger the time-changed callback

**Open Questions**

- **How should data be created?** Data for Sequentity is created by the application, and it's rather easy to mess it up. E.g. forget to assign a label or color to a given channel. Or assign two overlapping events. This could be abstracted into e.g. `Sequentity::Interface::create_event()` but it would mean either (1) making a component out of it such that we can read it from the application, or pass around an instance of Sequentity itself. Maybe Sequentity could be listening for a `CreateEvent` component with relevant data to create an event itself, and do any additional initialisation from there?
- **Who's responsible for managing memory?** Application data is associated with an event via a `void*`; it complicates memory management, as it involves a `new` and `delete` which is currently explicitly defined in the application. Error prone.
- **How do we manage selection?** Sequentity manages the currently selected event using a raw pointer in its own `State` component, is there a better way? We couldn't store selection state in an event itself, as they aren't the ones aware of whether their owner has got them selected or not. It's outside of their responsibility. And besides, it would mean we would need to iterate over all events to deselect before selecting another one, what a waste.

**Application Questions**

Unrelated to the sequencer, but to the example implementation.

- **AQ1: Event, Data and Length** Events don't know about your data. But it does have a length. So to correlate between an e.g. `std::vector` and the intersection of time and event the application assumes data and event is of equal length. When you e.g. click and hold, but do not move the mouse, that means a lot of duplicate data is stored.
- **AQ2: Tools and Time** Tools currently have no concept of time; which means that regardless of whether you're playing, an event will continue to grow.
- **AQ3: Non-entity related tools** Scrubbing affects global time and is currently a tool. However, in order to separate between press and move events, it still utilises the `Activated` and `Active` components like entity-tools. The consequence is that you can only scrub the global timeline when dragging on an entity.
- **AQ4: Initial values** Any animatable property has an equivalent "initial" component. E.g. `Position` is also provided by `InitialPosition` component. We use that to reset the `Position` at the start frame. But, we could also have the first value of each event carry an initial value. That way, to reset, we could (1) find the first event of each channel and (2) apply that.

<br>

### Usage

Sequentity can draw events in time, and facilitate edits to be made to those events interactively by the user. It's got a sense of time, which means you're able to query it for events that overlap with the current time.

Here's how you draw.

```cpp
/* Sequentity automatically sorts */
Sequentity::Sequentity sequentity { registry };

entt::registry& registry;
entity = registry.create();

auto& track = registry.assign<Sequentity::Track>(entity); {
    track.label = "My first track";
    track.color = ImColor::HSV(0.0f, 0.5f, 0.75f);
}

Sequentity::Channel channel; {
    channel.label = "My first channel";
    channel.color = ImColor::HSV(0.33f, 0.5f, 0.75f);
}

// Events may carry application data and a type for you to identify it with
struct MyEventData {
    float value { 0.0f };
};

enum {
    MyEventType = 0
};


auto* data = new MyEventData{ 5.0f };

Sequentity::Event event; {
    event.time = 1;
    event.length = 50;
    event.color ImColor::HSV(0.66f, 0.5f, 0.75f);

    // Application data, Sequentity won't touch it,
    // which means it's your memory to manage
    event.type = MyEventType;
    event.data = static_cast<void*>(data);
}


/* Tracks carry channels which carry events */
channel.events.push_back(event);
track.channels.push_back(channel);

sequentity.draw();
```

And here's how you query.

```cpp
registry.view<Sequentity::Track>().each([](auto& track) {
    sequentity.each_overlapping([](const auto& event) {
        if (event.type == MyEventType) {
            auto& data = static_cast<MyEventData*>(event.data);

            // Do something with it
            event.time
            event.length
        }
    });
})
```

The example application uses events for e.g. translations, storing a vector of integer pairs representing position. For each frame, data per entity is retrieved from the current event and correlated to a position by computing the time relative the start of an event.

<br>

#### Sorting

Tracks are sorted in the order of their EnTT pool.

```cpp
Registry.sort<Sequentity::Track>([this](const entt::entity lhs, const entt::entity rhs) {
    return Registry.get<Index>(lhs) < Registry.get<Index>(rhs);
});
```

<br>

### State

Sequentity is a struct that you instantiate, but it doesn't hold any state. State is instead held by the `State` struct which is accessible from anywhere. In the example application, it is used to draw the Transport panel with play, stop and visualisation of current time.

```cpp
auto& state = registry.ctx<Sequentity::State>();
```

When state is first accessed, if `State` has not already been created in the given registry, a new instance will be created. You may want state created by you, in which case you can assign it prior to calling Sequentity.

```cpp
auto& state = registry.set<Sequentity::State>();
state.current_time = 10;

// E.g.
Sequentity::Sequentity sequentity;
sequentity.draw();
```

Now the current time in Sequentity is 10.

<br>

### Serialisation

All data comes in the form of components with plain-old-data, including state like panning and zooming.

TODO

<br>

### Components

### Time Authority

As a sequencer, time is central. But who's owns time? In the case of Sequentity, I wanted it to have final say on time, which makes the application a slave to it. Sequentity increments time and provides a callback for when a change to it is made. You can also monitor its state for changes directly.

```cpp
auto& state = registry.ctx<Sequentity::State>();

```

<br>

### Example Application

The repo comes with an example application to test out the sequencer. In it, there are a few areas of interest.

1. Input Handling
1. Tools

Any interaction with the registry is done through "tools", which is each represented as a System (in ECS terminology). A tool, such as `TranslateTool` is called once per iteration of the main application loop but only has an affect on the `Activated`, `Active` and `Deactivated` entities.

- `Activated` is a one-off event indicating that an entity is about to be manipulated
- `Active` is reapplied each iteration with the current user input
- `Deactivated` is a one-off event indicating that the manipulation has finished

These components are attached by the application to any entity of interest.

Input is generalised; for example, once the mouse has generated `InputPosition2D` data, it bears no connection to the fact that the information came from a mouse. It could have come from anywhere, such as iPad touch, Eye tracking or WASD keyboard keys. As such, there is also an example `InputPosition3D` component for e.g. markerless motion capture, VR controllers or something coming out of your 3d scene like a physics simulation.