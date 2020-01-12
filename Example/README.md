![sequentitydemo](https://user-images.githubusercontent.com/2152766/72178092-0fd92e80-33da-11ea-9806-2d336cf596e5.gif)

### Sequentity Example Application

The is an example application to test out Sequentity. In it, there are a few areas of interest.

1. Input Handling
1. Tools

It should build out of the box but I haven't tested it elsewhere just yet. Till then, here's a distribution (Windows).

- [Download for Windows (1 mb)](https://github.com/alanjfs/sequentity/releases/download/0.1-alpha/sequentity-0.1-alpha-windows.zip)

<br>

**Todo**

- [ ] **Reset using first frame** Rather than storing a separate "initial" component
- [ ] **Relative changes using previous event** Such that we can move one translate-event ahead of another one

<br>

### Input Handling

Any interaction with the registry is done through "tools", which is each represented as a System (in ECS terminology). A tool, such as `TranslateTool` is called once per iteration of the main application loop but only has an affect on the `Activated`, `Active` and `Deactivated` entities.

- `Activated` is a one-off event indicating that an entity is about to be manipulated
- `Active` is reapplied each iteration with the current user input
- `Deactivated` is a one-off event indicating that the manipulation has finished

These components are attached by the application to any entity of interest.

Input is generalised; for example, once the mouse has generated `InputPosition2D` data, it bears no connection to the fact that the information came from a mouse. It could have come from anywhere, such as iPad touch, Eye tracking or WASD keyboard keys. As such, there is also an example `InputPosition3D` component for e.g. markerless motion capture, VR controllers or something coming out of your 3d scene like a physics simulation.

<br>

### Open Questions

Unrelated to the sequencer, but to the example implementation.

- **AQ1: Event, Data and Length** Events don't know about your data. But it does have a length. So to correlate between an e.g. `std::vector` and the intersection of time and event the application assumes data and event is of equal length. When you e.g. click and hold, but do not move the mouse, that means a lot of duplicate data is stored.
- **AQ2: Tools and Time** Tools currently have no concept of time; which means that regardless of whether you're playing, an event will continue to grow.
- **AQ3: Non-entity related tools** Scrubbing affects global time and is currently a tool. However, in order to separate between press and move events, it still utilises the `Activated` and `Active` components like entity-tools. The consequence is that you can only scrub the global timeline when dragging on an entity.
- **AQ4: Initial values** Any animatable property has an equivalent "initial" component. E.g. `Position` is also provided by `InitialPosition` component. We use that to reset the `Position` at the start frame. But, we could also have the first value of each event carry an initial value. That way, to reset, we could (1) find the first event of each channel and (2) apply that.
