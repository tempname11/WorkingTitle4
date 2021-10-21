# Refactoring

- rename _all_ usage::* stuff
- remove usage::* aliases
- go through graphics_render and see if any of Vulkan objects really need Own or Use.
  (they probably don't, and then the signature will become much lighter)

- move engine/rendering/intra -> engine/intra (any think of a better name)
- move engine/rendering/pass -> engine/pass (any think of a better name)

## older notes

- split `graphics_render` into smaller parts.
- Primary LBuffer: indirect light first. :IndirectFirst
- Move ad-hoc inline constants to uniform or at least `common/constants.glsl` :MoveToUniform
- The directory `src/task` should not be there. tasks should be where they logically belong.
- `session/setup.cxx` does too much and should be split up into smaller files.

- split off `display::Data` init from the huge `session_iteration_try_rendering`.
  (And use `*data = {...};` init style.)

- Error handing for file reads is currently hairy.
  (Consider a set of read helpers that provide zero output on error and go on...)
  (... with the error then checked once at the end.)
