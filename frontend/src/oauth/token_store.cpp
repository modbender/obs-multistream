#include "token_store.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include <windows.h>
#include <wincrypt.h>
#include <dpapi.h>

#include <util/platform.h>

#include "../log.hpp"

namespace OAuth {

namespace {

json AccountToJson(const OAuthAccount &a)
{
	return json{
		{"providerId", a.providerId}, {"access", a.access},
		{"refresh", a.refresh},       {"userId", a.userId},
		{"login", a.login},           {"displayName", a.displayName},
		{"expireTime", a.expireTime}, {"scopeVer", a.scopeVer},
	};
}

OAuthAccount AccountFromJson(const json &j)
{
	OAuthAccount a;
	if (!j.is_object()) {
		return a;
	}
	a.providerId = j.value("providerId", std::string());
	a.access = j.value("access", std::string());
	a.refresh = j.value("refresh", std::string());
	a.userId = j.value("userId", std::string());
	a.login = j.value("login", std::string());
	a.displayName = j.value("displayName", std::string());
	a.expireTime = j.value("expireTime", static_cast<int64_t>(0));
	a.scopeVer = j.value("scopeVer", 0);
	return a;
}

// DPAPI-wrap `plain` for the current user. Returns false (and leaves `out`
// untouched) on failure.
bool ProtectBytes(const std::string &plain, std::vector<unsigned char> &out)
{
	DATA_BLOB in;
	in.cbData = static_cast<DWORD>(plain.size());
	in.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(plain.data()));

	DATA_BLOB blob = {};
	if (!CryptProtectData(&in, L"obs-multistream oauth", nullptr, nullptr, nullptr, 0, &blob)) {
		return false;
	}
	out.assign(blob.pbData, blob.pbData + blob.cbData);
	LocalFree(blob.pbData);
	return true;
}

// DPAPI-unwrap `wrapped`. Returns false on failure (corrupt blob / host change).
bool UnprotectBytes(const std::vector<unsigned char> &wrapped, std::string &plain)
{
	DATA_BLOB in;
	in.cbData = static_cast<DWORD>(wrapped.size());
	in.pbData = const_cast<BYTE *>(wrapped.data());

	DATA_BLOB blob = {};
	if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &blob)) {
		return false;
	}
	plain.assign(reinterpret_cast<const char *>(blob.pbData), blob.cbData);
	LocalFree(blob.pbData);
	return true;
}

} // namespace

std::string TokenStore::FilePath()
{
	char buf[512];
	if (os_get_config_path(buf, sizeof(buf), "obs-multistream/oauth_tokens.json") <= 0) {
		return std::string();
	}
	return std::string(buf);
}

void TokenStore::EnsureLoadedLocked()
{
	if (loaded_) {
		return;
	}
	loaded_ = true; // mark loaded regardless: a failed/missing read = empty store

	const std::string path = FilePath();
	if (path.empty()) {
		return;
	}

	std::ifstream f(std::filesystem::u8path(path), std::ios::binary);
	if (!f) {
		return; // first run: no file yet
	}
	const std::vector<unsigned char> wrapped((std::istreambuf_iterator<char>(f)),
						 std::istreambuf_iterator<char>());
	if (wrapped.empty()) {
		return;
	}

	std::string plain;
	if (!UnprotectBytes(wrapped, plain)) {
		HostLog("[oauth] token store unwrap failed (corrupt or host changed); starting empty");
		return;
	}

	const json root = json::parse(plain, nullptr, false);
	if (root.is_discarded() || !root.is_object()) {
		HostLog("[oauth] token store JSON unparseable; starting empty");
		return;
	}
	for (auto it = root.begin(); it != root.end(); ++it) {
		accounts_[it.key()] = AccountFromJson(it.value());
	}
}

void TokenStore::SaveLocked()
{
	const std::string path = FilePath();
	if (path.empty()) {
		HostLog("[oauth] token store path unresolved; not saving");
		return;
	}

	json root = json::object();
	for (const auto &entry : accounts_) {
		root[entry.first] = AccountToJson(entry.second);
	}
	const std::string plain = root.dump();

	std::vector<unsigned char> wrapped;
	if (!ProtectBytes(plain, wrapped)) {
		HostLog("[oauth] token store DPAPI protect failed; not saving");
		return;
	}

	const std::filesystem::path fsPath = std::filesystem::u8path(path);
	os_mkdirs(fsPath.parent_path().u8string().c_str());

	std::ofstream f(fsPath, std::ios::binary | std::ios::trunc);
	if (!f) {
		HostLog("[oauth] token store open-for-write failed");
		return;
	}
	f.write(reinterpret_cast<const char *>(wrapped.data()), static_cast<std::streamsize>(wrapped.size()));
}

std::optional<OAuthAccount> TokenStore::Get(const std::string &profileUuid)
{
	const std::lock_guard<std::mutex> guard(mutex_);
	EnsureLoadedLocked();
	auto it = accounts_.find(profileUuid);
	if (it == accounts_.end()) {
		return std::nullopt;
	}
	return it->second;
}

void TokenStore::Put(const std::string &profileUuid, const OAuthAccount &account)
{
	const std::lock_guard<std::mutex> guard(mutex_);
	EnsureLoadedLocked();
	accounts_[profileUuid] = account;
	SaveLocked();
}

void TokenStore::Remove(const std::string &profileUuid)
{
	const std::lock_guard<std::mutex> guard(mutex_);
	EnsureLoadedLocked();
	if (accounts_.erase(profileUuid) > 0) {
		SaveLocked();
	}
}

std::map<std::string, OAuthAccount> TokenStore::All()
{
	const std::lock_guard<std::mutex> guard(mutex_);
	EnsureLoadedLocked();
	return accounts_;
}

TokenStore &Tokens()
{
	static TokenStore store;
	return store;
}

} // namespace OAuth
