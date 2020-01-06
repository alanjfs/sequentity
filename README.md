### Sequentity - A MIDI-like sequencer/tracker for C++ and ImGui

Written in C++ with ImGui, Magnum and EnTT.

**Open Questions**

- Application data is associated with an event via a `void*`; it complicates memory management, as it involves a `new` and `delete` which is currently explicitly defined in the application. Error prone.
