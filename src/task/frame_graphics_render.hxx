#pragma once
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_graphics_render( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  Ref<SessionData> session, \
  Use<SessionData::Vulkan::Core> core, \
  Use<RenderingData::SwapchainDescription> swapchain_description, \
  Use<RenderingData::CommandPools> command_pools, \
  Use<RenderingData::FrameInfo> frame_info, \
  Use<RenderingData::DescriptorPools> descriptor_pools, \
  Use<RenderingData::Prepass> prepass, \
  Use<RenderingData::GPass> gpass, \
  Use<RenderingData::LPass> lpass, \
  Use<RenderingData::Finalpass> finalpass, \
  Use<RenderingData::ZBuffer> zbuffer, \
  Use<RenderingData::GBuffer> gbuffer, \
  Use<RenderingData::LBuffer> lbuffer, \
  Use<RenderingData::FinalImage> final_image, \
  Use<SessionData::Vulkan::Prepass> s_prepass, \
  Use<SessionData::Vulkan::GPass> s_gpass, \
  Use<SessionData::Vulkan::LPass> s_lpass, \
  Use<SessionData::Vulkan::Finalpass> s_finalpass, \
  Use<SessionData::Vulkan::FullscreenQuad> fullscreen_quad, \
  Use<engine::misc::RenderList> render_list, \
  Own<engine::misc::GraphicsData> data \
)

TASK_DECL;
