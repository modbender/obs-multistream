#ifndef OBS_MULTISTREAM_FRONTEND_CHAT_THIRD_PARTY_EMOTES_HPP_
#define OBS_MULTISTREAM_FRONTEND_CHAT_THIRD_PARTY_EMOTES_HPP_

#include <functional>
#include <string>
#include <unordered_map>

// Third-party emote resolver (7TV + BetterTTV + FrankerFaceZ) for chat overlays
// (Phase 9). Given a channel identity, it fetches each provider's global + channel
// emote sets and returns a `code -> image URL` lookup the chat transport uses to
// substitute plain-text words with emote fragments.
namespace Chat {

// The host platform the third-party providers key their channel sets off. Only
// Twitch is wired today; the enum keeps the surface extensible (7TV/BTTV also serve
// YouTube/Kick users behind the same /users/<platform>/<id> shape).
enum class EmotePlatform {
	Twitch,
};

// Fetch the merged third-party emote map for one channel. `login` is the channel's
// platform login (FFZ keys rooms by login); `userId` is the platform's numeric user
// id (7TV + BTTV key channel sets by id). Either may be empty -- an empty `userId`
// simply skips the id-keyed channel sets while still fetching every global set and
// the FFZ room-by-login set.
//
// BLOCKING: issues several HTTP GETs in sequence, so it MUST run on a worker thread,
// never the UI thread. Every provider/set fetch is best-effort: a failure is logged
// and skipped, yielding fewer emotes rather than an error. `canceled` is polled
// before each GET so a Stop() during teardown returns promptly with whatever partial
// map was built (an empty map is fine -- the caller's post-pass tolerates it).
std::unordered_map<std::string, std::string> FetchThirdPartyEmotes(EmotePlatform platform, const std::string &login,
								   const std::string &userId,
								   const std::function<bool()> &canceled);

} // namespace Chat

#endif // OBS_MULTISTREAM_FRONTEND_CHAT_THIRD_PARTY_EMOTES_HPP_
