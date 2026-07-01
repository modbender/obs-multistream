#include "overlay_store.hpp"

#include "../multistream/StorePaths.hpp"

#include <obs.hpp>
#include <util/platform.h>

namespace Overlay {

json Widget::ToJson() const
{
	return json{
		{"id", id},     {"token", token}, {"name", name},     {"type", type},
		{"html", html}, {"css", css},     {"js", js},         {"fields", fields},
		{"assets", assets},
	};
}

json Widget::ToListJson(int port) const
{
	return json{{"id", id}, {"name", name}, {"type", type}, {"token", token}, {"url", WidgetUrl(*this, port)}};
}

Widget Widget::FromJson(const json &j)
{
	Widget w;
	if (!j.is_object()) {
		return w;
	}
	w.id = j.value("id", std::string());
	w.token = j.value("token", std::string());
	w.name = j.value("name", std::string());
	w.type = j.value("type", std::string());
	w.html = j.value("html", std::string());
	w.css = j.value("css", std::string());
	w.js = j.value("js", std::string());
	w.fields = j.contains("fields") && j["fields"].is_array() ? j["fields"] : json::array();
	w.assets = j.contains("assets") && j["assets"].is_array() ? j["assets"] : json::array();
	return w;
}

std::string WidgetUrl(const Widget &w, int port)
{
	return "http://127.0.0.1:" + std::to_string(port) + "/w/" + w.id + "?t=" + w.token;
}

std::string OverlayStore::FilePath()
{
	return MultistreamBasicPath("overlays.json");
}

std::string OverlayStore::AssetsDir(const std::string &id)
{
	return MultistreamBasicPath(("overlays/" + id + "/assets").c_str());
}

std::vector<Widget> OverlayStore::List() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return widgets_;
}

std::optional<Widget> OverlayStore::Get(const std::string &id) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	for (const Widget &w : widgets_) {
		if (w.id == id) {
			return w;
		}
	}
	return std::nullopt;
}

int OverlayStore::Port() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return port_;
}

void OverlayStore::SetPort(int port)
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		port_ = port;
	}
	Save();
}

void OverlayStore::InjectForTest(const Widget &w)
{
	std::lock_guard<std::mutex> lock(mutex_);
	widgets_.push_back(w);
}

// Group 2: read overlays.json (port + widgets) via obs_data_create_from_json_file_safe.
void OverlayStore::Load() {}

// Group 2: write overlays.json via obs_data_save_json_pretty_safe. caller holds mutex_.
void OverlayStore::Save() const {}

OverlayStore &Store()
{
	static OverlayStore s;
	return s;
}

} // namespace Overlay
