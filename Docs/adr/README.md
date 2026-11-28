# Architecture decision records

Each record explains one design decision: the problem, the options and why one was chosen. The records use the [MADR 4.0](https://adr.github.io/madr/) format.

- [0001: Run the screen reader as native C++ only](0001-native-only-runtime.md)
- [0002: Hooks copy ready data and never block the game thread](0002-non-blocking-hook-contract.md)
- [0003: Use PRISM for speech and braille](0003-prism-speech-backend.md)
- [0004: One Windows installer with a fixed scope](0004-installer-scope.md)

Records 0001 to 0004 were written after the decisions, from notes and code in this repository. Each one marks what the sources do not say.

To add a record, copy an existing one, give it the next number and add it to this list. To change a decision, write a new record and set the old record's status to `superseded by` the new one.
