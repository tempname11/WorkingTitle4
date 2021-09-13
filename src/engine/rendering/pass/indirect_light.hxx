#pragma once
#include <vulkan/vulkan.h>
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include "indirect_light/data.hxx"

namespace engine::rendering::pass::indirect_light {

void init_sdata(
  SData *out,
  Use<SessionData::Vulkan::Core> core
);

void deinit_sdata(
  SData *it,
  Use<SessionData::Vulkan::Core> core
);

void init_rdata(
  RData *out,
  SData *sdata,
  Use<SessionData::Vulkan::Core> core
);

void deinit_rdata(
  RData *it,
  Use<SessionData::Vulkan::Core> core
);

} // namespace