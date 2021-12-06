#pragma once
#include <ode/ode.h>

namespace engine::system::ode {

struct Impl;
void register_body(Impl* it, dBodyID body);
void init(Impl* out);
void deinit(Impl *it);
void update(Impl* it, double elapsed_sec);

}
