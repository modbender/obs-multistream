#ifndef OBS_MULTISTREAM_FRONTEND_OAUTH_REGISTRY_HPP_
#define OBS_MULTISTREAM_FRONTEND_OAUTH_REGISTRY_HPP_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "provider.hpp"

// The provider registry: a map<id, StreamProvider> populated once at boot. The
// generic `oauth.*` / `streamMeta.*` bridge methods dispatch through it so adding
// a platform is one module + one registry line, with no per-platform branches in
// the bridge. Task 3 ships the empty framework; Task 4 registers Twitch.
namespace OAuth {

class ProviderRegistry {
public:
	// Register `provider` under its id(). Replaces any existing entry with the
	// same id (last registration wins).
	void Register(std::unique_ptr<StreamProvider> provider);

	// The provider with `id`, or nullptr if none is registered.
	StreamProvider *Get(const std::string &id) const;

	// All registered providers (registration is at boot, before any reads).
	std::vector<StreamProvider *> All() const;

private:
	std::map<std::string, std::unique_ptr<StreamProvider>> providers_;
};

// Process-wide registry accessor (Meyers singleton), mirroring the other
// frontend stores' free-accessor shape.
ProviderRegistry &Registry();

// Populate the registry at boot. Called once from ObsBootstrap::Start(). Task 3
// registers nothing (the framework only); Task 4 adds the Twitch provider here.
void BootProviders();

} // namespace OAuth

#endif // OBS_MULTISTREAM_FRONTEND_OAUTH_REGISTRY_HPP_
