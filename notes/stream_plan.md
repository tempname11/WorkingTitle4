# Cleanup before stream! OUCH OUCH OUCH

- remove secondary* passes
- remove g2, z2
- remove old barriers annd their files (l2)
- remove VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT from lbuffer
- remove probe_depth!
- #if 0
- rename probe_light_update -> probe_collect
- rename secondary_gbuffer_texel_size (or even use ImageSize?)

# Stream plan

- we do not need (inflight-count) of everything. what can we get rid of?
