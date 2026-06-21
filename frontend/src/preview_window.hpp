#ifndef OBS_MULTISTREAM_FRONTEND_PREVIEW_WINDOW_HPP_
#define OBS_MULTISTREAM_FRONTEND_PREVIEW_WINDOW_HPP_

#include <windows.h>

#include <cstdint>

// Owns an obs_display attached to a child HWND, rendering the active program
// scene (output channel 0) into it each frame via obs_render_main_texture. The
// display is its own D3D11 swapchain libobs steps independently of the CEF
// browser's renderer. Lives between ObsBootstrap::Start() and Stop().
class PreviewWindow {
public:
	explicit PreviewWindow(HWND childHwnd);

	void CreateDisplay();
	void Resize(uint32_t cx, uint32_t cy);
	void Destroy();

private:
	HWND hwnd_;
};

#endif // OBS_MULTISTREAM_FRONTEND_PREVIEW_WINDOW_HPP_
