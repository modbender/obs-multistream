#include "event_store.hpp"

#include "../multistream/StorePaths.hpp"

#include <obs.hpp>
#include <util/platform.h>

#include <filesystem>

namespace Events {

namespace {

// Rebuild a NormalizedEvent from its ToJson() shape. Every optional field defaults
// to empty/zero (ToJson omits them), so a follow event round-trips without stray
// keys and an older/newer file with missing fields loads cleanly.
NormalizedEvent EventFromJson(const json &j)
{
	NormalizedEvent ev;
	if (!j.is_object()) {
		return ev;
	}
	ev.id = j.value("id", std::string());
	ev.platform = j.value("platform", std::string());
	ev.type = j.value("type", std::string());
	ev.ts = j.value("ts", static_cast<int64_t>(0));
	ev.actorName = j.value("actorName", std::string());
	ev.actorColor = j.value("actorColor", std::string());
	ev.amount = j.value("amount", static_cast<int64_t>(0));
	ev.currency = j.value("currency", std::string());
	ev.tier = j.value("tier", std::string());
	ev.months = j.value("months", 0);
	ev.count = j.value("count", 0);
	ev.message = j.value("message", std::string());
	return ev;
}

} // namespace

std::string EventStore::FilePath()
{
	return MultistreamBasicPath("events.json");
}

bool EventStore::Add(const NormalizedEvent &ev)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (ev.id.empty() || ids_.count(ev.id)) {
		return false; // no id (undedupable) or already stored -> drop
	}
	ids_.insert(ev.id);
	events_.push_back(ev);
	while (events_.size() > kCap) {
		ids_.erase(events_.front().id);
		events_.pop_front();
	}
	Save();
	return true;
}

std::vector<NormalizedEvent> EventStore::List() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return std::vector<NormalizedEvent>(events_.rbegin(), events_.rend()); // newest-first
}

void EventStore::Clear()
{
	std::lock_guard<std::mutex> lock(mutex_);
	events_.clear();
	ids_.clear();
	Save();
}

void EventStore::Load()
{
	// Called from the ctor before `this` is visible to any other thread, so no lock.
	OBSDataAutoRelease root = obs_data_create_from_json_file_safe(FilePath().c_str(), "bak");
	const char *js = root ? obs_data_get_json(root) : nullptr;
	if (!js) {
		return;
	}
	json parsed;
	try {
		parsed = json::parse(js);
	} catch (...) {
		return; // a corrupt file starts the store empty rather than aborting boot
	}
	if (!parsed.is_object() || !parsed.contains("events") || !parsed["events"].is_array()) {
		return;
	}
	for (const json &item : parsed["events"]) {
		NormalizedEvent ev = EventFromJson(item);
		if (ev.id.empty() || ids_.count(ev.id)) {
			continue;
		}
		ids_.insert(ev.id);
		events_.push_back(ev);
	}
	while (events_.size() > kCap) {
		ids_.erase(events_.front().id);
		events_.pop_front();
	}
}

void EventStore::Save() const
{
	json arr = json::array();
	for (const NormalizedEvent &ev : events_) {
		arr.push_back(ev.ToJson());
	}
	json root = json{{"events", std::move(arr)}};

	OBSDataAutoRelease data = obs_data_create_from_json(root.dump().c_str());

	// Ensure the parent dir exists (it does once a profile is created, but be safe).
	std::filesystem::path dir = std::filesystem::u8path(FilePath()).parent_path();
	os_mkdirs(dir.u8string().c_str());

	obs_data_save_json_pretty_safe(data, FilePath().c_str(), "tmp", "bak");
}

} // namespace Events
