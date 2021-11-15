# Next round TODO

- typedef lib::GUID -> struct lib::guid_t

- Rename `u64_table` -> `table64`. Move impl to a ".cxx".

- Remove GRUP stuff, leave only exporters and add DLL import adapter.

- `after_inflight` should be a function, not a task.

- Remove subtasks!

- Remove resource aliasing. 

- Uploader need a performance-oriented overhaul :UploaderMustBeImproved
  (also, maybe BLAS storage needs to be a part of it)
