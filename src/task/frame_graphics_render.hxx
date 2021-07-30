#pragma once
#include <src/global.hxx>
#include <src/engine/session.hxx>
#include <src/engine/rendering.hxx>
#include <src/engine/misc.hxx>
#include "task.hxx"

#undef TASK_DECL
#define TASK_DECL void frame_graphics_render( \
  task::Context<QUEUE_INDEX_NORMAL_PRIORITY> *ctx, \
  usage::Some<SessionData::Vulkan::Core> core, \
  usage::Some<RenderingData::SwapchainDescription> swapchain_description, \
  usage::Some<RenderingData::CommandPools> command_pools, \
  usage::Some<RenderingData::FrameInfo> frame_info, \
  usage::Some<RenderingData::DescriptorPools> descriptor_pools, \
  usage::Some<RenderingData::Prepass> prepass, \
  usage::Some<RenderingData::GPass> gpass, \
  usage::Some<RenderingData::LPass> lpass, \
  usage::Some<RenderingData::Finalpass> finalpass, \
  usage::Some<RenderingData::ZBuffer> zbuffer, \
  usage::Some<RenderingData::GBuffer> gbuffer, \
  usage::Some<RenderingData::LBuffer> lbuffer, \
  usage::Some<RenderingData::FinalImage> final_image, \
  usage::Some<SessionData::Vulkan::Prepass> s_prepass, \
  usage::Some<SessionData::Vulkan::GPass> s_gpass, \
  usage::Some<SessionData::Vulkan::LPass> s_lpass, \
  usage::Some<SessionData::Vulkan::Finalpass> s_finalpass, \
  usage::Some<SessionData::Vulkan::Textures> textures, \
  usage::Some<SessionData::Vulkan::FullscreenQuad> fullscreen_quad, \
  usage::Full<engine::misc::RenderList> render_list, \
  usage::Full<engine::misc::GraphicsData> data \
)

TASK_DECL;
