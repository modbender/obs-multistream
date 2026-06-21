#include "StreamProfileStore.hpp"

#include "StorePaths.hpp"

#include <util/platform.h>
#include <util/util.hpp>

#include <filesystem>

std::string StreamProfileStore::FilePath()
{
	return MultistreamBasicPath("streams.json");
}

void StreamProfileStore::Load()
{
	profiles.clear();

	OBSDataAutoRelease root = obs_data_create_from_json_file_safe(FilePath().c_str(), "bak");
	if (root) {
		OBSDataArrayAutoRelease arr = obs_data_get_array(root, "profiles");
		const size_t count = arr ? obs_data_array_count(arr) : 0;
		for (size_t i = 0; i < count; i++) {
			OBSDataAutoRelease item = obs_data_array_item(arr, i);
			profiles.push_back(StreamProfile::FromData(item));
		}
	}

	// Guarantee exactly one primary if any profiles exist.
	if (!profiles.empty()) {
		bool hasPrimary = false;
		for (const StreamProfile &p : profiles) {
			if (p.isPrimary) {
				hasPrimary = true;
				break;
			}
		}
		if (!hasPrimary) {
			profiles.front().isPrimary = true;
		}
	}
}

void StreamProfileStore::Save() const
{
	OBSDataAutoRelease root = obs_data_create();
	OBSDataArrayAutoRelease arr = obs_data_array_create();
	for (const StreamProfile &p : profiles) {
		OBSDataAutoRelease item = p.ToData();
		obs_data_array_push_back(arr, item);
	}
	obs_data_set_array(root, "profiles", arr);

	std::filesystem::path dir = std::filesystem::u8path(FilePath()).parent_path();
	os_mkdirs(dir.u8string().c_str());

	obs_data_save_json_pretty_safe(root, FilePath().c_str(), "tmp", "bak");
}

StreamProfile *StreamProfileStore::Primary()
{
	for (StreamProfile &p : profiles) {
		if (p.isPrimary) {
			return &p;
		}
	}
	return nullptr;
}

StreamProfile *StreamProfileStore::Find(const std::string &uuid)
{
	for (StreamProfile &p : profiles) {
		if (p.uuid == uuid) {
			return &p;
		}
	}
	return nullptr;
}

StreamProfile &StreamProfileStore::Add(StreamProfile p)
{
	if (p.uuid.empty()) {
		BPtr<char> id = os_generate_uuid();
		p.uuid = id.Get();
	}
	if (profiles.empty()) {
		p.isPrimary = true; // first profile is always primary
	}
	profiles.push_back(std::move(p));
	return profiles.back();
}

bool StreamProfileStore::SetPrimary(const std::string &uuid)
{
	bool found = false;
	for (StreamProfile &p : profiles) {
		if (p.uuid == uuid) {
			found = true;
		}
	}
	if (!found) {
		return false;
	}
	for (StreamProfile &p : profiles) {
		p.isPrimary = (p.uuid == uuid);
	}
	return true;
}

void StreamProfileStore::Remove(const std::string &uuid)
{
	for (auto it = profiles.begin(); it != profiles.end(); ++it) {
		if (it->uuid == uuid) {
			bool wasPrimary = it->isPrimary;
			profiles.erase(it);
			if (wasPrimary && !profiles.empty()) {
				profiles.front().isPrimary = true;
			}
			return;
		}
	}
}
