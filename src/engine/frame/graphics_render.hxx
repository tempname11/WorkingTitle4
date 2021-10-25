#pragma once
#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void graphics_render(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Ref<engine::display::Data> display,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Use<engine::session::Data::State> session_state,
  Use<engine::display::Data::CommandPools> command_pools,
  Use<engine::display::Data::DescriptorPools> descriptor_pools,
  Own<engine::display::Data::Common> common,
  Use<engine::misc::RenderList> render_list,
  Own<engine::misc::GraphicsData> data
);

} // namespace
