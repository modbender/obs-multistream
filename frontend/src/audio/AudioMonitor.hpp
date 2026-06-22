#pragma once

#include <obs.h>
#include <obs-audio-controls.h>

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>

// Owns one fader + volmeter per active audio source so the CEF/Svelte Audio
// Mixer can read/write per-source volume + mute and animate level meters.
// Modeled on the bootstrap-owned managers (CanvasRuntime): built in Start()
// after the default scene + modules, torn down in Stop() BEFORE obs_shutdown.
//
// Threading: obs_volmeter's `updated` callback fires on the AUDIO thread,
// frequently. It NEVER emits to JS directly. Each callback writes the latest
// magnitude/peak (in dB) into a mutex-guarded map and, throttled to ~30 Hz,
// marshals a single emit onto the CEF UI thread via Bridge::EmitAudioLevels,
// which snapshots the map (SnapshotLevels) and builds the payload there -- the
// 4.4.4 discipline that keeps payload construction on the UI thread.
//
// Teardown hazard: a volmeter can fire during destruction. RebuildEntries and
// the dtor remove the volmeter callback FIRST, then destroy the volmeter/fader.
class AudioMonitor {
public:
	AudioMonitor();
	~AudioMonitor();

	// (Re)build the per-source fader+volmeter set from the current active audio
	// sources (obs_enum_sources filtered to OBS_SOURCE_AUDIO + audio-active),
	// plus the global output channels 1..6 (desktop audio / mics) which are not
	// returned by obs_enum_sources. Idempotent: keeps existing entries, adds new
	// ones, drops gone ones. Runs on the UI thread (signal handlers post here).
	void Rebuild();

	// One active audio source as listed by audio.list.
	struct SourceInfo {
		std::string uuid;
		std::string name;
		float deflection = 0.0f; // fader position, 0..1
		float volumeDb = 0.0f;   // current volume in dB
		bool muted = false;
	};
	std::vector<SourceInfo> List() const;

	// Set a source's fader deflection (0..1). Returns false if the uuid has no
	// entry. Fills the resulting deflection + dB for the caller's response.
	bool SetDeflection(const std::string &uuid, float deflection, float &outDeflection, float &outDb);

	// Coalesced per-source levels (dB) drained on the UI thread by the bridge.
	struct Levels {
		float magnitude = -96.0f;
		float peak = -96.0f;
	};
	// Snapshot the coalesced levels under the lock (caller is on the UI thread).
	// Clears the pending flag so the next audio-thread callback re-arms the emit.
	std::map<std::string, Levels> SnapshotLevels();

	void ClearAll(); // detach + destroy every fader/volmeter (teardown)

private:
	// obs_volmeter_updated_t carries only `param`, not the firing volmeter/source,
	// so each entry registers the callback with its OWN context (one volmeter per
	// source). The context is heap-owned by the entry so the audio-thread callback
	// keeps a stable pointer; it is freed only after the callback is removed.
	struct CallbackCtx {
		AudioMonitor *monitor;
		std::string uuid;
	};

	struct Entry {
		std::string uuid;
		obs_fader_t *fader = nullptr;
		obs_volmeter_t *volmeter = nullptr;
		CallbackCtx *ctx = nullptr; // owned; registered as the volmeter callback param
	};

	// Build a fader+volmeter bound to `source` and register the volmeter callback.
	void AddEntry(obs_source_t *source);
	// Remove the volmeter callback FIRST, then destroy the volmeter + fader + ctx.
	void DestroyEntry(Entry &entry);

	// The volmeter `updated` callback (audio thread). `param` is the per-entry
	// CallbackCtx identifying which source fired.
	static void OnVolmeterUpdated(void *param, const float magnitude[MAX_AUDIO_CHANNELS],
				      const float peak[MAX_AUDIO_CHANNELS], const float input_peak[MAX_AUDIO_CHANNELS]);

	// Coalesce one source's levels and arm a throttled UI-thread emit.
	void CoalesceLevel(const std::string &uuid, float magnitudeDb, float peakDb);

	// entries is mutated only on the UI thread (Rebuild / teardown); the volmeter
	// callback reads it (to map volmeter -> uuid) on the audio thread, so guard it.
	mutable std::mutex entriesMutex;
	std::vector<Entry> entries;

	// Coalesced levels written by the audio-thread callback, drained on the UI
	// thread. Separate lock so a level write never blocks behind a Rebuild.
	std::mutex levelsMutex;
	std::map<std::string, Levels> levels;

	// At most one emit task is posted between drains: the first callback after a
	// drain flips this true and posts; subsequent callbacks just coalesce.
	std::atomic<bool> emitPending{false};
};
