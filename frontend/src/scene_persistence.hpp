#ifndef OBS_MULTISTREAM_FRONTEND_SCENE_PERSISTENCE_HPP_
#define OBS_MULTISTREAM_FRONTEND_SCENE_PERSISTENCE_HPP_

// Save/load of the GLOBAL (Default-canvas) scene collection -- scenes, their
// sources, and the full per-scene item layout -- to scene_collection.json under
// the shared obs-multistream/basic config dir. The new CEF frontend otherwise
// rebuilds a placeholder scene every boot and never persists user content, so
// anything added vanished on restart.
//
// Excluded from the save (restored separately or deferred): the channel 1-6
// global audio sources (re-seeded from audio_devices.json) and additional-canvas
// scoped sources (per-canvas persistence is a later phase).
namespace SceneCollection {

// Persist the global scene collection. No-op-safe; logs on failure.
void Save();

// Restore the global scene collection at boot and bind channel 0 to the saved
// current scene (falling back to the first loaded scene). Returns true when a
// collection was loaded and a scene bound; false when no file exists or it holds
// no scenes, in which case the caller builds the placeholder default scene.
bool Load();

} // namespace SceneCollection

#endif // OBS_MULTISTREAM_FRONTEND_SCENE_PERSISTENCE_HPP_
