# Refactoring TODO

## annoyances
- Move ad-hoc inline constants to uniform or at least `common/constants.glsl` :MoveToUniform
- The directory `src/task` should not be there. tasks should be where they logically belong.

## resource granularity
- go through graphics_render and see if any of Vulkan objects really need Own or Use.
  (they probably don't, and then the signature will become much lighter)
  (but mind how task dependencies might change!)

- Ref/Use/Own is moot for mutex-protected data, so should use Ref where possible.
  (Go through frame tasks and try to reduce these.)

## big chunks
- engine::step::* and engine::datum::* for older stuff
- split `graphics_render` into smaller parts.
- `session/setup.cxx` does too much and should be split up.

- split off `display::Data` init from the huge `session_iteration_try_rendering`.
  (And also try to use `*data = {...};` init style.)

## files

- Error handing for file reads is currently hairy.
  (Consider a set of read helpers that provide zero output on error and go on...)
  (... with the error then checked once at the end.)
