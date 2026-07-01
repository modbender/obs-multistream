#include "overlay_server.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "../log.hpp"
#include "../paths.hpp"      // RundirRoot
#include "overlay_store.hpp" // Overlay::Store(), Widget, WidgetUrl

#pragma comment(lib, "ws2_32.lib")

namespace Overlay {
namespace {

using json = nlohmann::json;

constexpr size_t kMaxHeaderBytes = 16 * 1024;      // 16 KB header block (mirrors mcp)
constexpr size_t kMaxAssetBytes = 8 * 1024 * 1024; // asset response cap (spec)
constexpr int kPreferredPort = 43000;
constexpr int kPortScanEnd = 43019;
constexpr DWORD kSseRecvTimeoutMs = 15000; // keepalive cadence

struct ReasonPhrase {
	int status;
	const char *phrase;
};

// Data-driven reason map (not a switch) so adding a status is a one-line edit.
constexpr std::array<ReasonPhrase, 6> kReasons = {{
	{200, "OK"},
	{403, "Forbidden"},
	{404, "Not Found"},
	{405, "Method Not Allowed"},
	{413, "Payload Too Large"},
	{500, "Internal Server Error"},
}};

const char *ReasonFor(int status)
{
	for (const auto &r : kReasons) {
		if (r.status == status) {
			return r.phrase;
		}
	}
	return "OK";
}

// Absolute path to the offline bundle root (copy of scheme.cpp::BundleRoot).
std::string BundleRoot()
{
	return RundirRoot() + "/data/obs-multistream/web";
}

// Map a file extension to a MIME type (copy of scheme.cpp::ContentTypeForPath).
std::string ContentTypeForPath(const std::string &path)
{
	static const std::vector<std::pair<std::string, std::string>> kTypes = {
		{".html", "text/html"}, {".htm", "text/html"},  {".js", "text/javascript"},
		{".mjs", "text/javascript"}, {".css", "text/css"}, {".json", "application/json"},
		{".svg", "image/svg+xml"}, {".png", "image/png"}, {".jpg", "image/jpeg"},
		{".jpeg", "image/jpeg"}, {".gif", "image/gif"}, {".ico", "image/x-icon"},
		{".woff", "font/woff"}, {".woff2", "font/woff2"}, {".ttf", "font/ttf"},
		{".wasm", "application/wasm"}, {".map", "application/json"}, {".txt", "text/plain"},
		{".mp3", "audio/mpeg"}, {".ogg", "audio/ogg"}, {".wav", "audio/wav"},
		{".webp", "image/webp"},
	};
	size_t dot = path.find_last_of('.');
	if (dot != std::string::npos) {
		std::string ext = path.substr(dot);
		std::transform(ext.begin(), ext.end(), ext.begin(),
			       [](unsigned char c) { return char(::tolower(c)); });
		for (const auto &[suffix, type] : kTypes) {
			if (ext == suffix) {
				return type;
			}
		}
	}
	return "application/octet-stream";
}

// Read one file under an absolute root, rejecting ".." (copy of scheme.cpp guard).
bool ReadFileGuarded(const std::string &root, const std::string &rel, std::string &out, std::string &ctype)
{
	if (rel.empty() || rel.find("..") != std::string::npos) {
		return false;
	}
	std::string full = root + "/" + rel;
	for (char &c : full) {
		if (c == '/') {
			c = '\\';
		}
	}
	std::ifstream f(full, std::ios::binary);
	if (!f) {
		return false;
	}
	out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
	ctype = ContentTypeForPath(rel);
	return true;
}

// Split "path?query" and return the "t" query value (may be ""). Tokens are hex, so
// no URL-decoding is needed.
std::string QueryToken(const std::string &pathWithQuery, std::string &pathOut)
{
	const size_t q = pathWithQuery.find('?');
	pathOut = q == std::string::npos ? pathWithQuery : pathWithQuery.substr(0, q);
	if (q == std::string::npos) {
		return std::string();
	}
	const std::string query = pathWithQuery.substr(q + 1);
	size_t pos = 0;
	while (pos < query.size()) {
		const size_t amp = query.find('&', pos);
		const std::string pair =
			query.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
		const size_t eq = pair.find('=');
		if (eq != std::string::npos && pair.substr(0, eq) == "t") {
			return pair.substr(eq + 1);
		}
		if (amp == std::string::npos) {
			break;
		}
		pos = amp + 1;
	}
	return std::string();
}

// Blocking best-effort write of an entire buffer; false on any send failure.
bool SendAll(SOCKET sock, const char *data, size_t len)
{
	size_t sent = 0;
	while (sent < len) {
		const int chunk = (int)std::min<size_t>(len - sent, 64 * 1024);
		const int n = send(sock, data + sent, chunk, 0);
		if (n <= 0) {
			return false;
		}
		sent += (size_t)n;
	}
	return true;
}

// Write a complete HTTP/1.1 response with Connection: close (mirrors mcp WriteResponse).
void WriteResponse(SOCKET sock, int status, const std::string &ctype, const std::string &body)
{
	std::string head = "HTTP/1.1 " + std::to_string(status) + " " + ReasonFor(status) + "\r\n";
	head += "Content-Type: " + ctype + "\r\n";
	head += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	head += "Connection: close\r\n";
	head += "Access-Control-Allow-Origin: *\r\n";
	head += "\r\n";
	const std::string out = head + body;
	SendAll(sock, out.data(), out.size());
}

// Build the served widget document (spec shell): user CSS in <style>, user HTML in
// <body>, then window.__OVERLAY__, the runtime, then the user JS.
std::string AssembleDocument(const Widget &w, int port)
{
	json fieldData = json::object();
	for (const json &f : w.fields) {
		if (f.contains("key")) {
			fieldData[f["key"].get<std::string>()] = f.value("value", json(nullptr));
		}
	}
	const json overlay = json{{"id", w.id}, {"token", w.token}, {"port", port}, {"fields", fieldData}};
	std::string doc = "<!doctype html><html><head><meta charset=\"utf-8\">\n<style>\n";
	doc += w.css;
	doc += "\n</style></head><body>\n";
	doc += w.html;
	doc += "\n<script>window.__OVERLAY__=" + overlay.dump() + ";</script>\n";
	doc += "<script src=\"/runtime.js?t=" + w.token + "\"></script>\n";
	doc += "<script>\n" + w.js + "\n</script>\n</body></html>";
	return doc;
}

} // namespace

// ---- Broadcast --------------------------------------------------------------

void OverlayServer::Broadcast(const Events::NormalizedEvent &ev)
{
	const std::string frame = "data: " + ev.ToJson().dump() + "\n\n";
	std::lock_guard<std::mutex> lock(sseMutex_);
	std::vector<std::pair<std::string, uintptr_t>> dead;
	for (auto &[wid, socks] : sockets_) {
		for (uintptr_t s : socks) {
			if (!SendAll((SOCKET)s, frame.data(), frame.size())) {
				dead.emplace_back(wid, s);
			}
		}
	}
	for (auto &[wid, s] : dead) {
		sockets_[wid].erase(s);
		closesocket((SOCKET)s);
	}
}

void OverlayServer::BroadcastTo(const std::string &widgetId, const Events::NormalizedEvent &ev)
{
	const std::string frame = "data: " + ev.ToJson().dump() + "\n\n";
	std::lock_guard<std::mutex> lock(sseMutex_);
	auto it = sockets_.find(widgetId);
	if (it == sockets_.end()) {
		return;
	}
	std::vector<uintptr_t> dead;
	for (uintptr_t s : it->second) {
		if (!SendAll((SOCKET)s, frame.data(), frame.size())) {
			dead.push_back(s);
		}
	}
	for (uintptr_t s : dead) {
		it->second.erase(s);
		closesocket((SOCKET)s);
	}
}

// ---- Lifecycle --------------------------------------------------------------

OverlayServer::~OverlayServer()
{
	Stop();
}

bool OverlayServer::BindOn(int port)
{
	SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) {
		lastError_ = "socket() failed (" + std::to_string(WSAGetLastError()) + ")";
		return false;
	}
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1 ONLY
	addr.sin_port = htons((unsigned short)port);
	if (bind(listenSock, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
		lastError_ =
			"bind(127.0.0.1:" + std::to_string(port) + ") failed (" + std::to_string(WSAGetLastError()) + ")";
		closesocket(listenSock);
		return false;
	}
	if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
		lastError_ = "listen() failed (" + std::to_string(WSAGetLastError()) + ")";
		closesocket(listenSock);
		return false;
	}
	// Read back the actual port so a port-0 (OS-ephemeral) bind reports its assignment.
	sockaddr_in bound = {};
	int len = sizeof(bound);
	if (getsockname(listenSock, (sockaddr *)&bound, &len) == 0) {
		port_ = ntohs(bound.sin_port);
	} else {
		port_ = port;
	}
	listenSocket_ = (uintptr_t)listenSock;
	return true;
}

bool OverlayServer::Start()
{
	if (running_.load()) {
		return true;
	}
	lastError_.clear();

	WSADATA wsaData;
	const int wsaRc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaRc != 0) {
		lastError_ = "WSAStartup failed (" + std::to_string(wsaRc) + ")";
		HostLog("[overlay] " + lastError_);
		return false;
	}
	wsaUp_ = true;

	const int preferred = Store().Port();
	bool bound = false;
	if (preferred > 0 && BindOn(preferred)) {
		bound = true;
	} else {
		for (int p = kPreferredPort; p <= kPortScanEnd; ++p) {
			if (p == preferred) {
				continue; // already tried above
			}
			if (BindOn(p)) {
				bound = true;
				break;
			}
		}
	}
	if (!bound) {
		lastError_ = "no free port in 43000..43019";
		HostLog("[overlay] no free port in 43000..43019");
		WSACleanup();
		wsaUp_ = false;
		return false;
	}

	portChanged_ = (port_ != Store().Port());
	Store().SetPort(port_);
	running_.store(true);
	acceptThread_ = std::thread(&OverlayServer::AcceptLoop, this);
	HostLog("[overlay] server listening on 127.0.0.1:" + std::to_string(port_) +
		(portChanged_ ? " (port changed)" : ""));
	return true;
}

bool OverlayServer::StartForTest(int port, int *boundPort)
{
	if (running_.load()) {
		if (boundPort) {
			*boundPort = port_;
		}
		return true;
	}
	lastError_.clear();

	WSADATA wsaData;
	const int wsaRc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaRc != 0) {
		lastError_ = "WSAStartup failed (" + std::to_string(wsaRc) + ")";
		HostLog("[overlay] " + lastError_);
		return false;
	}
	wsaUp_ = true;

	if (!BindOn(port)) {
		HostLog("[overlay] StartForTest bind failed: " + lastError_);
		WSACleanup();
		wsaUp_ = false;
		return false;
	}
	if (boundPort) {
		*boundPort = port_;
	}
	running_.store(true);
	acceptThread_ = std::thread(&OverlayServer::AcceptLoop, this);
	return true;
}

void OverlayServer::Stop()
{
	if (!running_.exchange(false)) {
		// Not running; still balance a stray WSAStartup / join a late accept thread.
		if (acceptThread_.joinable()) {
			acceptThread_.join();
		}
		{
			std::lock_guard<std::mutex> lock(threadsMutex_);
			for (auto &c : threads_) {
				if (c.thread.joinable()) {
					c.thread.join();
				}
			}
			threads_.clear();
		}
		if (wsaUp_) {
			WSACleanup();
			wsaUp_ = false;
		}
		return;
	}

	// Close the listen socket to unblock accept().
	if (listenSocket_ != ~uintptr_t(0)) {
		closesocket((SOCKET)listenSocket_);
		listenSocket_ = ~uintptr_t(0);
	}
	// Close every open SSE socket to unblock its recv loop, then forget them. Each
	// RunSse's DropSocket then no-ops (erase returns 0), so nothing double-closes.
	{
		std::lock_guard<std::mutex> lock(sseMutex_);
		for (auto &[wid, socks] : sockets_) {
			for (uintptr_t s : socks) {
				closesocket((SOCKET)s);
			}
		}
		sockets_.clear();
	}
	if (acceptThread_.joinable()) {
		acceptThread_.join();
	}
	// The accept thread has exited, so no more Conn threads are spawned; join them all.
	{
		std::lock_guard<std::mutex> lock(threadsMutex_);
		for (auto &c : threads_) {
			if (c.thread.joinable()) {
				c.thread.join();
			}
		}
		threads_.clear();
	}
	if (wsaUp_) {
		WSACleanup();
		wsaUp_ = false;
	}
	HostLog("[overlay] server stopped");
}

// ---- Accept + routing -------------------------------------------------------

void OverlayServer::ReapFinishedThreads()
{
	std::lock_guard<std::mutex> lock(threadsMutex_);
	for (auto it = threads_.begin(); it != threads_.end();) {
		if (it->done->load()) {
			if (it->thread.joinable()) {
				it->thread.join();
			}
			it = threads_.erase(it);
		} else {
			++it;
		}
	}
}

void OverlayServer::AcceptLoop()
{
	while (running_.load()) {
		ReapFinishedThreads();
		SOCKET client = accept((SOCKET)listenSocket_, nullptr, nullptr);
		if (client == INVALID_SOCKET) {
			if (!running_.load()) {
				break;
			}
			continue;
		}
		auto done = std::make_shared<std::atomic<bool>>(false);
		std::thread t([this, client, done]() {
			HandleConnection((uintptr_t)client);
			done->store(true);
		});
		std::lock_guard<std::mutex> lock(threadsMutex_);
		threads_.push_back(Conn{std::move(t), done});
	}
}

void OverlayServer::HandleConnection(uintptr_t clientSocket)
{
	const SOCKET sock = (SOCKET)clientSocket;

	// Read the header block until "\r\n\r\n", capping total size.
	std::string buffer;
	char temp[4096];
	size_t headerEnd = std::string::npos;
	while (true) {
		const int n = recv(sock, temp, (int)sizeof(temp), 0);
		if (n <= 0) {
			closesocket(sock);
			return;
		}
		buffer.append(temp, (size_t)n);
		headerEnd = buffer.find("\r\n\r\n");
		if (headerEnd != std::string::npos) {
			break;
		}
		if (buffer.size() > kMaxHeaderBytes) {
			WriteResponse(sock, 413, "text/plain", "header too large");
			closesocket(sock);
			return;
		}
	}

	const std::string headerBlock = buffer.substr(0, headerEnd);
	const size_t lineEnd = headerBlock.find("\r\n");
	const std::string requestLine = lineEnd == std::string::npos ? headerBlock : headerBlock.substr(0, lineEnd);

	const size_t sp1 = requestLine.find(' ');
	const size_t sp2 = sp1 == std::string::npos ? std::string::npos : requestLine.find(' ', sp1 + 1);
	if (sp1 == std::string::npos || sp2 == std::string::npos) {
		WriteResponse(sock, 405, "text/plain", "malformed request");
		closesocket(sock);
		return;
	}
	const std::string method = requestLine.substr(0, sp1);
	const std::string target = requestLine.substr(sp1 + 1, sp2 - sp1 - 1);
	if (method != "GET") {
		WriteResponse(sock, 405, "text/plain", "method not allowed");
		closesocket(sock);
		return;
	}

	std::string path;
	const std::string token = QueryToken(target, path);

	// Route table (order: most specific first). Data list, not a switch, so a new
	// top-level widget-type route is a one-line add. The handler owns socket close;
	// the SSE handler keeps the socket open for its stream lifetime.
	struct Route {
		const char *prefix;
		bool exact;
		void (OverlayServer::*handler)(uintptr_t, const std::string &, const std::string &);
	};
	static const std::array<Route, 2> kRoutes = {{
		{"/runtime.js", true, &OverlayServer::ServeRuntime},
		{"/w/", false, &OverlayServer::ServeWidget},
	}};
	for (const auto &r : kRoutes) {
		const bool match = r.exact ? (path == r.prefix) : (path.rfind(r.prefix, 0) == 0);
		if (match) {
			(this->*r.handler)(clientSocket, path, token);
			return;
		}
	}
	WriteResponse(sock, 404, "text/plain", "not found");
	closesocket(sock);
}

void OverlayServer::ServeRuntime(uintptr_t clientSocket, const std::string &, const std::string &token)
{
	const SOCKET sock = (SOCKET)clientSocket;
	// runtime.js is non-sensitive, but keep the uniform token guard: accept if the
	// token matches ANY widget (or if there are none, e.g. a fresh install).
	const std::vector<Widget> widgets = Store().List();
	bool ok = widgets.empty();
	for (const Widget &w : widgets) {
		if (w.token == token) {
			ok = true;
			break;
		}
	}
	if (!ok) {
		WriteResponse(sock, 403, "text/plain", "forbidden");
		closesocket(sock);
		return;
	}
	std::string body;
	std::string ctype;
	if (!ReadFileGuarded(BundleRoot() + "/overlay", "runtime.js", body, ctype)) {
		WriteResponse(sock, 404, "text/plain", "runtime not found");
		closesocket(sock);
		return;
	}
	WriteResponse(sock, 200, ctype, body);
	closesocket(sock);
}

void OverlayServer::ServeWidget(uintptr_t clientSocket, const std::string &path, const std::string &token)
{
	const SOCKET sock = (SOCKET)clientSocket;
	const std::string rest = path.substr(3); // after "/w/"
	const size_t slash = rest.find('/');
	const std::string id = slash == std::string::npos ? rest : rest.substr(0, slash);
	const std::string action = slash == std::string::npos ? std::string() : rest.substr(slash);
	if (id.empty()) {
		WriteResponse(sock, 404, "text/plain", "not found");
		closesocket(sock);
		return;
	}

	std::optional<Widget> w = Store().Get(id);
	if (!w) {
		WriteResponse(sock, 404, "text/plain", "no such overlay");
		closesocket(sock);
		return;
	}
	if (token != w->token) {
		WriteResponse(sock, 403, "text/plain", "forbidden");
		closesocket(sock);
		return;
	}

	if (action.empty() || action == "/") {
		WriteResponse(sock, 200, "text/html", AssembleDocument(*w, port_));
		closesocket(sock);
		return;
	}
	if (action == "/events") {
		RunSse(clientSocket, id); // owns the socket; no close here
		return;
	}
	if (action.rfind("/assets/", 0) == 0) {
		const std::string file = action.substr(std::strlen("/assets/"));
		std::string body;
		std::string ctype;
		if (!ReadFileGuarded(Store().AssetsDir(id), file, body, ctype)) {
			WriteResponse(sock, 404, "text/plain", "asset not found");
			closesocket(sock);
			return;
		}
		if (body.size() > kMaxAssetBytes) {
			WriteResponse(sock, 413, "text/plain", "asset too large");
			closesocket(sock);
			return;
		}
		WriteResponse(sock, 200, ctype, body);
		closesocket(sock);
		return;
	}
	WriteResponse(sock, 404, "text/plain", "not found");
	closesocket(sock);
}

void OverlayServer::RunSse(uintptr_t clientSocket, const std::string &widgetId)
{
	const SOCKET sock = (SOCKET)clientSocket;
	{
		std::lock_guard<std::mutex> lock(sseMutex_);
		const char *head = "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\n"
				   "Cache-Control: no-cache\r\nConnection: keep-alive\r\n"
				   "Access-Control-Allow-Origin: *\r\n\r\n";
		if (send(sock, head, (int)strlen(head), 0) <= 0) {
			closesocket(sock);
			return;
		}
		// Initial comment so EventSource fires onopen promptly.
		send(sock, ": connected\n\n", 13, 0);
		sockets_[widgetId].insert(clientSocket);
	}

	// The 15s recv timeout drives the keepalive; recv also detects a client close.
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&kSseRecvTimeoutMs, sizeof(kSseRecvTimeoutMs));
	char buf[512];
	while (running_.load()) {
		const int n = recv(sock, buf, sizeof(buf), 0);
		if (n == 0) {
			break; // client closed
		}
		if (n == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				std::lock_guard<std::mutex> lock(sseMutex_);
				auto it = sockets_.find(widgetId);
				if (it == sockets_.end() || !it->second.count(clientSocket)) {
					break; // dropped by Broadcast/Stop
				}
				if (send(sock, ": ping\n\n", 8, 0) <= 0) {
					break;
				}
				continue;
			}
			break; // real error / socket closed by Stop/Broadcast
		}
		// Ignore any client->server bytes.
	}
	DropSocket(widgetId, clientSocket);
}

void OverlayServer::DropSocket(const std::string &widgetId, uintptr_t sock)
{
	std::lock_guard<std::mutex> lock(sseMutex_);
	auto it = sockets_.find(widgetId);
	if (it == sockets_.end()) {
		return;
	}
	if (it->second.erase(sock) > 0) {
		closesocket((SOCKET)sock); // single close: only whoever erases from the set closes
	}
	if (it->second.empty()) {
		sockets_.erase(it);
	}
}

OverlayServer &Server()
{
	static OverlayServer server;
	return server;
}

} // namespace Overlay
