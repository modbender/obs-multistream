#include "OutputBindingStore.hpp"

#include "StorePaths.hpp"

#include <util/platform.h>
#include <util/util.hpp>

#include <filesystem>

std::string OutputBindingStore::FilePath()
{
	return MultistreamBasicPath("output_bindings.json");
}

nlohmann::json OutputBindingStore::ToJson() const
{
	OBSDataAutoRelease root = obs_data_create();
	OBSDataArrayAutoRelease arr = bindings.ToDataArray();
	obs_data_set_array(root, "output_bindings", arr);

	const char *js = obs_data_get_json(root);
	return js ? nlohmann::json::parse(js) : nlohmann::json::object();
}

void OutputBindingStore::FromJson(const nlohmann::json &j)
{
	bindings = OutputBindings{};

	if (j.is_object()) {
		OBSDataAutoRelease root = obs_data_create_from_json(j.dump().c_str());
		if (root) {
			OBSDataArrayAutoRelease arr = obs_data_get_array(root, "output_bindings");
			bindings = OutputBindings::FromDataArray(arr);
		}
	}
}

void OutputBindingStore::Load()
{
	OBSDataAutoRelease root = obs_data_create_from_json_file_safe(FilePath().c_str(), "bak");
	const char *js = root ? obs_data_get_json(root) : nullptr;
	FromJson(js ? nlohmann::json::parse(js) : nlohmann::json::object());
}

void OutputBindingStore::Save() const
{
	OBSDataAutoRelease root = obs_data_create_from_json(ToJson().dump().c_str());

	std::filesystem::path dir = std::filesystem::u8path(FilePath()).parent_path();
	os_mkdirs(dir.u8string().c_str());

	obs_data_save_json_pretty_safe(root, FilePath().c_str(), "tmp", "bak");
}
