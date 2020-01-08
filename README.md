# NOT READY FOR THE PUBLIC EYE

Just started working on this, come back another time for images, docs and other goodies. :)

<br>
<br>

### Sequentity - A MIDI-like sequencer/tracker for C++ and ImGui

Written in C++ with ImGui, Magnum and EnTT.

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
- [x] **Refactor, Unify data types** Data types in Sequentity is a mixture of native, Magnum and ImGui.
- [x] **Smooth Panning and Zooming** Any change to these have a nice smoothing effect
- [x] **Drag Current Time** You can, but it won't trigger the time-changed callback

**Open Questions**

- Data for Sequentity is created by the application, and it's rather easy to mess it up. E.g. forget to assign a label or color to a given channel. Or assign two overlapping events. This could be abstracted into e.g. `Sequentity::Interface::create_event()` but it would mean either (1) making a component out of it such that we can read it from the application, or pass around an instance of Sequentity itself. Maybe Sequentity could be listening for a `CreateEvent` component with relevant data to create an event itself, and do any additional initialisation from there?
- Application data is associated with an event via a `void*`; it complicates memory management, as it involves a `new` and `delete` which is currently explicitly defined in the application. Error prone.
- `Event` components are completely standalone and carry all relevant data with it, but reside in a `Track` component for the sole purpose of identifying unique event types, for horizontal sorting during render. An alterantive is to compute uniqueness either dynamically during draw, or whenever a new event is added to a channel. Track makes typing easier and conceptualising types of events in your head, but does it come at a performance penalty?
- Interactions with Sequentity is done through methods on a class. These could instead be provided as components that are cleared up and reset per iteration; to make it more "ECS".
- **Selection** Sequentity manages the currently selected event using a raw pointer in its own `State` component, is there a better way? We couldn't store selection state in an event itself, as they aren't the ones aware of whether their owner has got them selected or not. It's outside of their responsibility. And besides, it would mean we would need to iterate over all events to deselect before selecting another one, what a waste.

**Application Questions**

Unrelated to the sequencer, but to the example implementation.

- **Event, Data and Length** Events don't know about your data. It only knows about length. So to correlate between data and where the current time cursor intersects and event the application assumes that data is a vector of equal length to the length of the event. When you e.g. click and hold, but do not move the mouse, that means a lot of duplicate data is stored.
- **Tools and Time** Tools currently have no concept of time; which means that regardless of whether you're playing, an event will continue to grow.
- **Non-entity related tools** Scrubbing affects global time and is currently a tool. However, in order to separate between press and move events, it still utilises the `Activated` and `Active` components like entity-tools. The consequence is that you can only scrub the global timeline when dragging on an entity.

<br>

### Usage

```cpp
entt::registry& registry;
Sequentity::Sequentity sequentity { registry };

entity = registry.create();
auto& track = registry.assign<Sequentity::Track>();
track.
```

<br>

### Components

### Time Authority

As a sequencer, time is central. But who's owns time? In the case of Sequentity, I wanted it to have final say on time, which makes the application a slave to it. Sequentity increments time and provides a callback for when a change to it is made. You can also monitor its state for changes directly.

```cpp
auto& state = registry.ctx<Sequentity::State>();

```

<br>

### Serialisation

All data comes in the form of components with plain-old-data, including state like panning and zooming.

<br>

### Example Application

The repo comes with an example application to test out the sequencer. In it, there are a few areas of interest.

1. Tools
1. Input Handling

Any interaction with the registry is done through "tools", which is each represented as a System (in ECS terminology). A tool, such as `TranslateTool` is called once per iteration of the main application loop but only has an affect on the `Activated`, `Active` and `Deactivated` entities.

- `Activated` is a one-off event indicating that an entity is about to be manipulated
- `Active` is reapplied each iteration with the current user input
- `Deactivated` is a one-off event indicating that the manipulation has finished

Input is generalised into 