#ifndef OBS_MULTISTREAM_FRONTEND_EVENTS_EVENT_STORE_HPP_
#define OBS_MULTISTREAM_FRONTEND_EVENTS_EVENT_STORE_HPP_

#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "event_model.hpp"

// The persisted, de-duplicated event history (Phase 9.2a). A single global file
// (<config>/obs-multistream/basic/events.json -- the same dir as streams.json,
// since events are per-account and shared across scene collections) loaded on
// construction so the dock shows history immediately. A bounded ring (cap 500,
// most-recent-wins) plus an id set for O(1) dedupe. Thread-safe: EventHub workers
// call Add() from multiple threads, so every method locks an internal mutex.
namespace Events {

class EventStore {
public:
	EventStore() { Load(); }

	// Append `ev` if its id is new; returns false (a no-op) when the id is empty or
	// already present (dedupe). On a new id: append, evict the oldest past the cap,
	// persist. Callable from any worker thread.
	bool Add(const NormalizedEvent &ev);

	// The whole history newest-first, for events.list.
	std::vector<NormalizedEvent> List() const;

	// Drop all history + persist the empty file, for events.clear.
	void Clear();

	// <config>/obs-multistream/basic/events.json.
	static std::string FilePath();

private:
	void Load();       // read events.json into events_/ids_ (called from the ctor)
	void Save() const; // write events.json atomically (caller holds mutex_)

	static constexpr size_t kCap = 500;

	mutable std::mutex mutex_;
	std::deque<NormalizedEvent> events_;  // oldest at front, newest at back
	std::unordered_set<std::string> ids_; // dedupe index over events_
};

} // namespace Events

#endif // OBS_MULTISTREAM_FRONTEND_EVENTS_EVENT_STORE_HPP_
