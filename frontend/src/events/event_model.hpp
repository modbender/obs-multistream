#ifndef OBS_MULTISTREAM_FRONTEND_EVENTS_EVENT_MODEL_HPP_
#define OBS_MULTISTREAM_FRONTEND_EVENTS_EVENT_MODEL_HPP_

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

// The normalized cross-platform live-event model (Phase 9.2a). One struct plus an
// open `type` vocabulary so the store, hub, bridge, and dock never branch per
// platform: adding an event kind is a new `type` string (and a dock icon), never a
// new code path. Follow/sub/gift/cheer/raid/superchat/membership from every
// platform's transport funnel into this shape before they reach the store or JS.
namespace Events {

using json = nlohmann::json;

struct NormalizedEvent {
	std::string id;         // stable, platform-unique; the dedupe key
	std::string platform;   // "twitch" | "youtube" | "kick"
	std::string type;       // follow|sub|resub|subgift|cheer|raid|superchat|supersticker|member
	int64_t ts = 0;         // ms since epoch (event time if the API gives one, else receipt)
	std::string actorName;
	std::string actorColor; // "" if unknown
	// Optional, per-type (omitted from JSON when empty-string / zero):
	int64_t amount = 0;   // cheer bits / superchat minor-currency units / raid viewers
	std::string currency; // superchat currency code
	std::string tier;     // sub/member tier label
	int months = 0;       // resub cumulative months
	int count = 0;        // gift count
	std::string message;  // resub/superchat user message (plain text; the dock escapes it)

	// The JS/persistence shape: always id/platform/type/ts/actorName; every other
	// field is omitted when empty-string or zero so a follow event carries no stray
	// amount/currency/... keys and the dock's optional-field summary stays simple.
	json ToJson() const
	{
		json j = json{
			{"id", id},
			{"platform", platform},
			{"type", type},
			{"ts", ts},
			{"actorName", actorName},
		};
		if (!actorColor.empty()) {
			j["actorColor"] = actorColor;
		}
		if (amount != 0) {
			j["amount"] = amount;
		}
		if (!currency.empty()) {
			j["currency"] = currency;
		}
		if (!tier.empty()) {
			j["tier"] = tier;
		}
		if (months != 0) {
			j["months"] = months;
		}
		if (count != 0) {
			j["count"] = count;
		}
		if (!message.empty()) {
			j["message"] = message;
		}
		return j;
	}
};

} // namespace Events

#endif // OBS_MULTISTREAM_FRONTEND_EVENTS_EVENT_MODEL_HPP_
