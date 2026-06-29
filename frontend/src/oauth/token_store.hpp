#ifndef OBS_MULTISTREAM_FRONTEND_OAUTH_TOKEN_STORE_HPP_
#define OBS_MULTISTREAM_FRONTEND_OAUTH_TOKEN_STORE_HPP_

#include <map>
#include <mutex>
#include <optional>
#include <string>

#include "provider.hpp"

// The OAuth token store: per-stream-profile OAuthAccount records persisted to
// <config>/obs-multistream/oauth_tokens.json, DPAPI-wrapped at rest
// (CryptProtectData) so the access/refresh tokens are not on disk in plaintext.
// Touched from both the CEF UI thread (bridge methods) and detached OAuth worker
// threads, so every accessor is mutex-guarded. A corrupt/undecryptable file
// (host changed, tampering) is logged and treated as empty -- never fatal.
namespace OAuth {

class TokenStore {
public:
	std::optional<OAuthAccount> Get(const std::string &profileUuid);
	void Put(const std::string &profileUuid, const OAuthAccount &account);
	void Remove(const std::string &profileUuid);
	std::map<std::string, OAuthAccount> All();

	// <config>/obs-multistream/oauth_tokens.json (the DPAPI blob, not JSON text).
	static std::string FilePath();

private:
	void EnsureLoadedLocked();
	void SaveLocked();

	std::mutex mutex_;
	bool loaded_ = false;
	std::map<std::string, OAuthAccount> accounts_;
};

// Process-wide token store accessor (Meyers singleton), mirroring the other
// frontend stores' free-accessor shape.
TokenStore &Tokens();

} // namespace OAuth

#endif // OBS_MULTISTREAM_FRONTEND_OAUTH_TOKEN_STORE_HPP_
