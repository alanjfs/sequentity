# NOT READY FOR THE PUBLIC EYE

Just started working on this, come back another time for images, docs and other goodies. :)

<br>
<br>

### Sequentity - A MIDI-like sequencer/tracker for C++ and ImGui

Written in C++ with ImGui, Magnum and EnTT.

**Todo**

- [ ] **Transport** For playing, stopping and manipulating playback range
- [ ] **Event Scaling** Sometimes, the input is good, but could use some fine-tuning
- [ ] **Event Cropping** Other times, part of the input isn't worth keeping
- [ ] **Track Folding** For when you have way too much going on
- [ ] **Event Vertical Move** Implement moving of events between channels and tracks (i.e. vertically)
- [ ] **Refactor, Unify data types** Data types in Sequentity is a mixture of native, Magnum and ImGui.
- [ ] **UX, Escape** Prevent events from escaping the current range

**Open Questions**

- Data for Sequentity is created by the application, and it's rather easy to mess it up. E.g. forget to assign a label or color to a given channel. Or assign two overlapping events. This could be abstracted into e.g. `Sequentity::Interface::create_event()` but it would mean either (1) making a component out of it such that we can read it from the application, or pass around an instance of Sequentity itself.
- Application data is associated with an event via a `void*`; it complicates memory management, as it involves a `new` and `delete` which is currently explicitly defined in the application. Error prone.
- `Event` components are completely standalone and carry all relevant data with it, but reside in a `Track` component for the sole purpose of identifying unique event types, for horizontal sorting during render. An alterantive is to compute uniqueness either dynamically during draw, or whenever a new event is added to a channel. Track makes typing easier and conceptualising types of events in your head, but does it come at a performance penalty?

**Application Questions**

Unrelated to the sequencer, but to the example implementation.

- **Event, Data and Length** Events don't know about your data. It only knows about length. So to correlate between data and where the current time cursor intersects and event the application assumes that data is a vector of equal length to the length of the event. When you e.g. click and hold, but do not move the mouse, that means a lot of duplicate data is stored.
- **Tools and Time** Tools currently have no concept of time; which means that regardless of whether you're playing, an event will continue to grow.
