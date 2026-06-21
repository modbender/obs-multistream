#include "obs_bootstrap.hpp"

#include <obs.h>
#include <obs-frontend-internal.hpp>
#include <util/base.h>

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <string>

#include "frontend_callbacks.hpp"
#include "log.hpp"
#include "paths.hpp"

namespace {

// Route libobs/plugin blog() output to stderr so plugin lifecycle logging (e.g.
// obs-browser's "frontend owns CEF" line) is captured alongside the host's own.
void ObsLogHandler(int level, const char *format, va_list args, void *)
{
	char buf[4096];
	vsnprintf(buf, sizeof(buf), format, args);
	HostLog(std::string("[obs:log] ") + buf);
	(void)level;
}

// The frontend-api shim. Ownership is handed to libobs via
// obs_frontend_set_callbacks_internal; libobs deletes it on obs_shutdown. We
// keep a non-owning pointer to fan lifecycle events (FINISHED_LOADING).
FrontendCallbacks *g_frontend = nullptr;

// Test scene + browser source bound to output channel 0 (4.1.2 proof).
obs_scene_t *g_scene = nullptr;

// Curated single-plugin load: obs-browser only. Mirrors the spike PoC.
bool LoadObsBrowser()
{
	const std::string root = RundirRoot();
	const std::string module_path = root + "/obs-plugins/64bit/obs-browser.dll";
	const std::string data_path = root + "/data/obs-plugins/obs-browser/";

	obs_module_t *module = nullptr;
	const int rv = obs_open_module(&module, module_path.c_str(), data_path.c_str());
	if (rv != MODULE_SUCCESS || !module) {
		HostLog("[obs] obs_open_module(obs-browser) failed, code=" + std::to_string(rv));
		return false;
	}
	if (!obs_init_module(module)) {
		HostLog("[obs] obs_init_module(obs-browser) failed");
		return false;
	}
	HostLog("[obs] obs-browser module loaded");
	return true;
}

// Build the 4.1.2 test scene: one browser source on a self-contained data: URL
// (no network), bound to output channel 0 so it ticks and composites into the
// preview.
void CreateTestScene()
{
	obs_data_t *settings = obs_data_create();
	obs_data_set_string(
		settings, "url",
		"data:text/html,<html><body style='margin:0'>"
		"<div style='width:100vw;height:100vh;background:%23ff00ff'></div></body></html>");
	obs_data_set_int(settings, "width", 1280);
	obs_data_set_int(settings, "height", 720);

	obs_source_t *source = obs_source_create("browser_source", "fe-test-web", settings, nullptr);
	obs_data_release(settings);
	if (!source) {
		HostLog("[obs] obs_source_create(browser_source) failed");
		return;
	}
	HostLog("[obs] browser source created (fe-test-web)");

	g_scene = obs_scene_create("fe-test-scene");
	if (!g_scene) {
		HostLog("[obs] obs_scene_create failed");
		obs_source_release(source);
		return;
	}

	obs_scene_add(g_scene, source);
	obs_source_release(source); // scene owns the create-ref now

	obs_set_output_source(0, obs_scene_get_source(g_scene));
	HostLog("[obs] test scene bound to output channel 0");
}

} // namespace

bool ObsBootstrap::Start()
{
	// obs-browser checks this in its guarded path to skip CefInitialize (the
	// frontend already owns the single CEF context). Set before module load.
	SetEnvironmentVariableW(L"OBS_FRONTEND_OWNS_CEF", L"1");

	base_set_log_handler(ObsLogHandler, nullptr);

	if (!obs_startup("en-US", nullptr, nullptr)) {
		HostLog("[obs] obs_startup failed");
		return false;
	}
	HostLog("[obs] obs_startup ok");

	const std::string root = RundirRoot();
	obs_add_data_path((root + "/data/libobs/").c_str());

	obs_video_info ovi = {};
	ovi.graphics_module = "libobs-d3d11";
	ovi.fps_num = 60;
	ovi.fps_den = 1;
	ovi.base_width = 1920;
	ovi.base_height = 1080;
	ovi.output_width = 1920;
	ovi.output_height = 1080;
	ovi.output_format = VIDEO_FORMAT_NV12;
	ovi.colorspace = VIDEO_CS_709;
	ovi.range = VIDEO_RANGE_PARTIAL;
	ovi.adapter = 0;
	ovi.gpu_conversion = true;
	ovi.scale_type = OBS_SCALE_BICUBIC;

	const int rv = obs_reset_video(&ovi);
	if (rv != OBS_VIDEO_SUCCESS) {
		HostLog("[obs] obs_reset_video failed, code=" + std::to_string(rv));
		return false;
	}
	HostLog("[obs] obs_reset_video ok (1920x1080@60, D3D11)");

	obs_audio_info oai = {};
	oai.samples_per_sec = 48000;
	oai.speakers = SPEAKERS_STEREO;
	if (!obs_reset_audio(&oai)) {
		HostLog("[obs] obs_reset_audio failed");
		return false;
	}
	HostLog("[obs] obs_reset_audio ok (48kHz stereo)");

	// Register the frontend-api shim before loading modules so obs-browser's
	// obs_module_load (which calls obs_frontend_add_event_callback) resolves
	// against it. libobs takes ownership and deletes it on obs_shutdown.
	g_frontend = new FrontendCallbacks();
	obs_frontend_set_callbacks_internal(g_frontend);
	HostLog("[obs] frontend-api shim registered");

	if (!LoadObsBrowser()) {
		HostLog("[obs] obs-browser load failed -- continuing without it");
	}

	obs_post_load_modules();
	HostLog("[obs] core up (obs-browser curated load)");

	// Lifecycle signal obs-browser's registered handler expects post-load.
	if (g_frontend) {
		g_frontend->on_event(OBS_FRONTEND_EVENT_FINISHED_LOADING);
	}

	CreateTestScene();

	return true;
}

void ObsBootstrap::TeardownScene()
{
	if (!g_scene) {
		return;
	}

	// Unbind from the output channel first so nothing ticks/renders it.
	obs_set_output_source(0, nullptr);

	// Removing + releasing the scene cascades to the browser source's destroy,
	// which only POSTS `delete this` to TID_UI (and posts a CEF CloseBrowser).
	// The caller pumps CefDoMessageLoopWork() after this so those drain before
	// CefShutdown.
	obs_source_t *scene_source = obs_scene_get_source(g_scene);
	obs_source_remove(scene_source);
	obs_scene_release(g_scene);
	g_scene = nullptr;
	HostLog("[obs] test scene released");
}

void ObsBootstrap::Stop()
{
	// Deferred source destruction can cascade across the destruction-task
	// thread; drain in a loop until no more work is spawned before
	// obs_shutdown, mirroring the Qt frontend's ClearSceneData.
	while (obs_wait_for_destroy_queue()) {
	}

	obs_shutdown();

	// Same counter the legacy frontend prints. Nonzero == fixed libobs static
	// residuals (no per-run growth), not host-introduced leaks.
	HostLog("[obs] leaks: " + std::to_string(bnum_allocs()));
	HostLog("[obs] shutdown complete");
}
