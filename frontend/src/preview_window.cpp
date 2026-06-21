#include "preview_window.hpp"

#include <obs.h>

#include "log.hpp"

namespace {

obs_display_t *g_display = nullptr;

// Draw callback: fired by libobs once per frame on the render thread. cx/cy are
// the display (HWND) pixel size. Fit the base canvas into it with letterboxing
// so the composited scene keeps its aspect ratio instead of stretching.
void RenderPreview(void *, uint32_t cx, uint32_t cy)
{
	obs_video_info ovi;
	if (!obs_get_video_info(&ovi)) {
		return;
	}

	const float baseCX = float(ovi.base_width);
	const float baseCY = float(ovi.base_height);
	if (baseCX <= 0.0f || baseCY <= 0.0f || cx == 0 || cy == 0) {
		return;
	}

	const float scale = (float(cx) / baseCX < float(cy) / baseCY) ? float(cx) / baseCX : float(cy) / baseCY;
	const int drawCX = int(baseCX * scale);
	const int drawCY = int(baseCY * scale);
	const int drawX = (int(cx) - drawCX) / 2;
	const int drawY = (int(cy) - drawCY) / 2;

	gs_viewport_push();
	gs_projection_push();

	gs_ortho(0.0f, baseCX, 0.0f, baseCY, -100.0f, 100.0f);
	gs_set_viewport(drawX, drawY, drawCX, drawCY);

	obs_render_main_texture();

	gs_projection_pop();
	gs_viewport_pop();
}

} // namespace

PreviewWindow::PreviewWindow(HWND childHwnd) : hwnd_(childHwnd) {}

void PreviewWindow::CreateDisplay()
{
	if (g_display) {
		return;
	}

	RECT rc;
	GetClientRect(hwnd_, &rc);

	gs_init_data init = {};
	init.cx = uint32_t(rc.right - rc.left);
	init.cy = uint32_t(rc.bottom - rc.top);
	init.format = GS_BGRA;
	init.zsformat = GS_ZS_NONE;
	init.window.hwnd = hwnd_; // child HWND passthrough (gs_window.hwnd is void*)

	g_display = obs_display_create(&init, 0x000000);

	HostLog(std::string("[obs] obs_display_create -> ") + (g_display ? "OK" : "NULL"));

	if (g_display) {
		obs_display_add_draw_callback(g_display, RenderPreview, nullptr);
		HostLog("[obs] preview draw callback registered");
	}
}

void PreviewWindow::Resize(uint32_t cx, uint32_t cy)
{
	if (g_display) {
		obs_display_resize(g_display, cx, cy);
	}
}

void PreviewWindow::Destroy()
{
	if (g_display) {
		obs_display_remove_draw_callback(g_display, RenderPreview, nullptr);
		obs_display_destroy(g_display);
		g_display = nullptr;
		HostLog("[obs] preview display destroyed");
	}
}
