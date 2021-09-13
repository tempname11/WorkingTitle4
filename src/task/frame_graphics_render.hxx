#pragma once
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_graphics_render( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Ref<SessionData> session, \
  Use<SessionData::Vulkan::Core> core, \
  Use<engine::display::Data::SwapchainDescription> swapchain_description, \
  Use<engine::display::Data::CommandPools> command_pools, \
  Use<engine::display::Data::FrameInfo> frame_info, \
  Use<engine::display::Data::DescriptorPools> descriptor_pools, \
  Own<engine::display::Data::Prepass> prepass, \
  Own<engine::display::Data::GPass> gpass, \
  Own<engine::display::Data::LPass> lpass, \
  Own<engine::rendering::pass::indirect_light::RData> indirect_light_rdata, \
  Own<engine::display::Data::Finalpass> finalpass, \
  Use<engine::display::Data::ZBuffer> zbuffer, \
  Use<engine::display::Data::GBuffer> gbuffer, \
  Use<engine::display::Data::LBuffer> lbuffer, \
  Use<engine::display::Data::FinalImage> final_image, \
  Use<SessionData::Vulkan::Prepass> s_prepass, \
  Use<SessionData::Vulkan::GPass> s_gpass, \
  Use<SessionData::Vulkan::LPass> s_lpass, \
  Use<engine::rendering::pass::indirect_light::SData> indirect_light_sdata, \
  Use<SessionData::Vulkan::Finalpass> s_finalpass, \
  Use<SessionData::Vulkan::FullscreenQuad> fullscreen_quad, \
  Use<engine::misc::RenderList> render_list, \
  Own<engine::misc::GraphicsData> data \
)

TASK_DECL;
