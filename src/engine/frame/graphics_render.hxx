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
  Ref<engine::session::Vulkan::Core> core,
  Ref<engine::display::Data::SwapchainDescription> swapchain_description,
  Use<engine::display::Data::CommandPools> command_pools,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Use<engine::display::Data::DescriptorPools> descriptor_pools,
  Own<engine::display::Data::Prepass> prepass,
  Own<engine::display::Data::GPass> gpass,
  Own<engine::display::Data::LPass> lpass,
  Own<engine::step::probe_measure::DData> probe_measure_ddata,
  Own<engine::step::probe_collect::DData> probe_collect_ddata,
  Own<engine::step::indirect_light::DData> indirect_light_ddata,
  Own<engine::display::Data::Finalpass> finalpass,
  Use<engine::display::Data::ZBuffer> zbuffer,
  Use<engine::display::Data::GBuffer> gbuffer,
  Use<engine::display::Data::LBuffer> lbuffer,
  Use<engine::datum::probe_radiance::DData> probe_radiance,
  Use<engine::datum::probe_irradiance::DData> probe_irradiance,
  Use<engine::datum::probe_attention::DData> probe_attention,
  Use<engine::display::Data::FinalImage> final_image,
  Use<engine::session::Vulkan::Prepass> s_prepass,
  Use<engine::session::Vulkan::GPass> s_gpass,
  Use<engine::session::Vulkan::LPass> s_lpass,
  Own<engine::step::probe_measure::SData> probe_measure_sdata,
  Use<engine::step::probe_collect::SData> probe_collect_sdata,
  Use<engine::step::indirect_light::SData> indirect_light_sdata,
  Use<engine::session::Vulkan::Finalpass> s_finalpass,
  Use<engine::session::Vulkan::FullscreenQuad> fullscreen_quad,
  Use<engine::misc::RenderList> render_list,
  Own<engine::misc::GraphicsData> data
);

} // namespace
