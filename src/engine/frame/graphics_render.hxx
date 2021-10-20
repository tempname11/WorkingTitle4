#pragma once
#include <src/global.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>

namespace engine::frame {

void graphics_render(
  lib::task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx,
  Ref<engine::session::Data> session,
  Use<engine::session::Data::State> session_state,
  Use<engine::session::Vulkan::Core> core,
  Use<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<engine::display::Data::CommandPools> command_pools,
  Use<engine::display::Data::FrameInfo> frame_info,
  Use<engine::display::Data::DescriptorPools> descriptor_pools,
  Own<engine::display::Data::Prepass> prepass,
  Own<engine::display::Data::GPass> gpass,
  Own<engine::display::Data::LPass> lpass,
  Own<engine::rendering::pass::secondary_geometry::DData> secondary_geometry_ddata,
  Own<engine::rendering::pass::indirect_light_secondary::DData> indirect_light_secondary_ddata,
  Own<engine::rendering::pass::directional_light_secondary::DData> directional_light_secondary_ddata,
  Own<engine::rendering::pass::probe_light_update::DData> probe_light_update_ddata,
  Own<engine::rendering::pass::probe_depth_update::DData> probe_depth_update_ddata,
  Own<engine::rendering::pass::indirect_light::DData> indirect_light_ddata,
  Own<engine::display::Data::Finalpass> finalpass,
  Use<engine::display::Data::ZBuffer> zbuffer,
  Use<engine::display::Data::GBuffer> gbuffer,
  Use<engine::display::Data::LBuffer> lbuffer,
  Use<engine::rendering::intra::secondary_zbuffer::DData> zbuffer2,
  Use<engine::rendering::intra::secondary_gbuffer::DData> gbuffer2,
  Use<engine::rendering::intra::secondary_lbuffer::DData> lbuffer2,
  Use<engine::rendering::intra::probe_light_map::DData> probe_light_map,
  Use<engine::rendering::intra::probe_depth_map::DData> probe_depth_map,
  Use<engine::display::Data::FinalImage> final_image,
  Use<engine::session::Vulkan::Prepass> s_prepass,
  Use<engine::session::Vulkan::GPass> s_gpass,
  Use<engine::session::Vulkan::LPass> s_lpass,
  Own<engine::rendering::pass::secondary_geometry::SData> secondary_geometry_sdata,
  Use<engine::rendering::pass::indirect_light_secondary::SData> indirect_light_secondary_sdata,
  Use<engine::rendering::pass::directional_light_secondary::SData> directional_light_secondary_sdata,
  Use<engine::rendering::pass::probe_light_update::SData> probe_light_update_sdata,
  Use<engine::rendering::pass::probe_depth_update::SData> probe_depth_update_sdata,
  Use<engine::rendering::pass::indirect_light::SData> indirect_light_sdata,
  Use<engine::session::Vulkan::Finalpass> s_finalpass,
  Use<engine::session::Vulkan::FullscreenQuad> fullscreen_quad,
  Use<engine::misc::RenderList> render_list,
  Own<engine::misc::GraphicsData> data
);

} // namespace