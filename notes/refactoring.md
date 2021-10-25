# Refactoring TODO

## big chunks
- engine::step::* and engine::datum::* for older stuff
- split `graphics_render` into smaller parts.
- `session/setup.cxx` does too much and should be split up.

## files

- Error handing for file reads is currently hairy.
  (Consider a set of read helpers that provide zero output on error and go on...)
  (... with the error then checked once at the end.)
