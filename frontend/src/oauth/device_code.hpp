#ifndef OBS_MULTISTREAM_FRONTEND_OAUTH_DEVICE_CODE_HPP_
#define OBS_MULTISTREAM_FRONTEND_OAUTH_DEVICE_CODE_HPP_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "provider.hpp"

// The OAuth 2.0 Device Authorization Grant (RFC 8628) strategy -- Twitch's auth
// path (no client secret, no proxy): POST the device endpoint, show the user a
// code, then poll the token endpoint until the user authorizes. Reusable by any
// provider whose endpoints follow the same shape; the only per-provider knobs are
// the endpoints, client id, scopes, and the scope-form field name (Twitch names
// it `scopes`; the RFC default is `scope`).
namespace OAuth {

class DeviceCodeStrategy : public AuthStrategy {
public:
	struct Config {
		std::string deviceUrl;          // device-authorization endpoint
		std::string tokenUrl;           // token endpoint
		std::string clientId;           // public client id (device flow needs no secret)
		std::vector<std::string> scopes; // requested scopes (space-joined into one form field)
		int scopeVer = 0;               // stamped into issued accounts for scope-version invalidation
		std::string scopeParam = "scopes"; // the scopes form field name (Twitch="scopes")
	};

	explicit DeviceCodeStrategy(Config config);

	bool begin(DeviceCodeStart &out, std::string &err) override;
	PollResult poll(const std::string &deviceCode, OAuthAccount &acct, std::string &err) override;
	bool refresh(OAuthAccount &acct, std::string &err) override;
	bool ensureFresh(OAuthAccount &acct, const std::string &profileUuid, std::string &err) override;
	int scopeVer() const override { return config_.scopeVer; }

private:
	// One mutex per account key, created on demand, so two concurrent ensureFresh
	// callers for the SAME account serialize (single-flight) while different
	// accounts refresh in parallel.
	std::shared_ptr<std::mutex> FlightLock(const std::string &key);

	Config config_;
	std::mutex flightMapMutex_;
	std::map<std::string, std::shared_ptr<std::mutex>> flightLocks_;
};

} // namespace OAuth

#endif // OBS_MULTISTREAM_FRONTEND_OAUTH_DEVICE_CODE_HPP_
