# Refactoring TODO

## resource granularity
- Ref/Use/Own is moot for mutex-protected data, so should use Ref where possible.
  (Go through frame tasks and try to reduce these.)

- Ref<Core> is now probably better served by Ref<Session>

## big chunks
- engine::step::* and engine::datum::* for older stuff
- split `graphics_render` into smaller parts.
- `session/setup.cxx` does too much and should be split up.

- split off `display::init` from `try_rendering`.
  (And also try to use `*data = {...};` style there.)

## files

- Error handing for file reads is currently hairy.
  (Consider a set of read helpers that provide zero output on error and go on...)
  (... with the error then checked once at the end.)
