# Wumpus regression tests

From the repository root:

```sh
make -C tests/wumpus
```

The tests compile the actual `watch-faces/complication/wumpus_face.c` implementation with a small simulated Movement boundary and deterministic randomness. No firmware submodules or ARM toolchain are required. A native C compiler with AddressSanitizer and UndefinedBehaviorSanitizer support is required; sanitizers are enabled by default.

The suite checks:

- Cave topology, safe starting rooms and hazard counts.
- Context allocation and reuse, independent face instances, pause/resume during shot entry, navigation and sleep handling.
- Turns after completed actions, movement probability and preservation of stationary hazards.
- Warning rotation when the set of nearby hazards changes.
- Arrow routing through actual tunnels, random fallback paths, bat removal and self-hits.
- All victory and death conditions, including a successful final arrow and an immediate loss after the fifth miss.
- Three-second result display, LED cleanup and both LCD variants.
- 50,000 random UI events with game-state invariants checked after every event.

The `test-wumpus` CI job runs this suite independently of the firmware builds.
