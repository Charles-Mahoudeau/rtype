/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CurlClient
*/

#include "plugins/http/detail/CurlClient.hpp"

#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/multi.h>
#include <curl/system.h>
#include <curl/urlapi.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plugins/http/HttpClient.hpp"
#include "plugins/http/HttpSettings.hpp"
#include "plugins/http/exceptions/HttpException.hpp"

namespace rtype::plugins::http::detail {
namespace {
using exceptions::HttpException;

constexpr std::array<std::string_view, 6> kMethods{"GET", "POST", "PUT", "PATCH", "DELETE", "HEAD"};
constexpr std::string_view kSecretStart = "{{env:";
constexpr std::string_view kSecretEnd = "}}";
using Clock = std::chrono::steady_clock;
constexpr long kMaxConnectMs = 10000;  // NOLINT(google-runtime-int): curl takes a long

/// @brief Frees a curl URL handle.
struct UrlDeleter {
    void operator()(CURLU* url) const noexcept { curl_url_cleanup(url); }
};

/// @brief Frees a string curl allocated.
struct CurlStringDeleter {
    void operator()(char* text) const noexcept { curl_free(text); }
};

/// @return @p text in lowercase.
[[nodiscard]] std::string toLower(std::string text) {
    std::ranges::transform(text, text.begin(),
                           [](const unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    return text;
}

/// @return The part @p part of @p url, or an empty string.
[[nodiscard]] std::string getPart(CURLU* url, const CURLUPart part) {
    char* value = nullptr;
    if (curl_url_get(url, part, &value, 0) != CURLUE_OK || value == nullptr) {
        return {};
    }
    const std::unique_ptr<char, CurlStringDeleter> owned{value};
    return std::string{owned.get()};
}

/// @return The value of the environment variable @p name, or nothing if it is not set.
[[nodiscard]] std::optional<std::string> readEnvironment(const std::string& name) {
#ifdef _WIN32
    char* value = nullptr;
    std::size_t length = 0;
    // _dupenv_s rather than getenv, which MSVC deprecates: it copies the value, freed by the unique_ptr.
    if (_dupenv_s(&value, &length, name.c_str()) != 0 || value == nullptr) {
        return std::nullopt;
    }
    const std::unique_ptr<char, decltype(&std::free)> owned{value, &std::free};
    return std::string{owned.get()};
#else
    const char* value = std::getenv(name.c_str());  // NOLINT(concurrency-mt-unsafe): read once, on the main thread
    if (value == nullptr) {
        return std::nullopt;
    }
    return std::string{value};
#endif
}

/// @return Whether @p name is an HTTP header name: letters, digits and `-`.
[[nodiscard]] bool isHeaderName(const std::string_view name) {
    return !name.empty() && std::ranges::all_of(name, [](const unsigned char letter) {
        return std::isalnum(letter) != 0 || letter == '-' || letter == '_';
    });
}

/// @brief Sets an option of @p easy.
/// @throws HttpException If curl refuses it.
template <typename Value>
void setOption(CURL* easy, const CURLoption option, Value value) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): curl_easy_setopt is a C variadic function
    if (curl_easy_setopt(easy, option, value) != CURLE_OK) {
        throw HttpException{"The HTTP client could not set up a request"};
    }
}
}  // namespace

CurlClient::CurlClient(HttpSettings settings) : _settings{std::move(settings)} {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        throw HttpException{"The HTTP client could not start libcurl"};
    }
    _multi = curl_multi_init();
    if (_multi == nullptr) {
        curl_global_cleanup();
        throw HttpException{"The HTTP client could not start libcurl"};
    }
}

CurlClient::~CurlClient() {
    for (auto& [id, transfer] : _active) {
        release(*transfer);
    }
    _active.clear();
    curl_multi_cleanup(_multi);
    curl_global_cleanup();
}

RequestId CurlClient::send(const HttpRequest& request) {
    Checked checked = check(request);
    checked.id = _next;
    _next = _next == RequestId{0xFFFFFFFF} ? 1 : _next + 1;
    const RequestId id = checked.id;
    if (_active.size() < _settings.maxConcurrent) {
        start(std::move(checked));
    } else {
        if (_queue.size() >= _settings.maxQueued) {
            _failed.push_back(HttpResult{.request = id, .error = std::string{kQueueFull}});
        } else {
            // After the last request of at least this priority: FIFO among equals.
            const auto position =
                std::ranges::upper_bound(_queue, checked.priority, std::ranges::greater{}, &Checked::priority);
            _queue.insert(position, std::move(checked));
        }
    }
    return id;
}

bool CurlClient::cancel(const RequestId request) {
    if (const auto found = _active.find(request); found != _active.end()) {
        release(*found->second);
        _active.erase(found);
        startQueued();
        return true;
    }
    const auto queued = std::ranges::find(_queue, request, &Checked::id);
    if (queued == _queue.end()) {
        return false;
    }
    _queue.erase(queued);
    return true;
}

std::vector<HttpResult> CurlClient::poll() {
    expireQueued();
    std::vector<HttpResult> results = std::exchange(_failed, {});
    int running = 0;
    curl_multi_perform(_multi, &running);
    int left = 0;
    while (const CURLMsg* message = curl_multi_info_read(_multi, &left)) {
        if (message->msg != CURLMSG_DONE) {
            continue;
        }
        Transfer* transfer = nullptr;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): curl_easy_getinfo is a C variadic function
        curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &transfer);
        if (transfer == nullptr) {
            continue;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access): curl's C API; `result` is the member of CURLMSG_DONE
        results.push_back(finish(*transfer, message->data.result));
        const RequestId id = transfer->request.id;
        release(*transfer);
        _active.erase(id);
    }
    startQueued();
    return results;
}

std::size_t CurlClient::getPendingCount() const noexcept { return _active.size() + _queue.size(); }

CurlClient::Checked CurlClient::check(const HttpRequest& request) const {
    Checked checked;
    checked.method = request.method.empty() ? std::string{"GET"} : request.method;
    std::ranges::transform(checked.method, checked.method.begin(),
                           [](const unsigned char letter) { return static_cast<char>(std::toupper(letter)); });
    if (std::ranges::find(kMethods, checked.method) == kMethods.end()) {
        throw HttpException{
            std::format("Unknown HTTP method '{}' (expected GET, POST, PUT, PATCH, DELETE or HEAD)", request.method)};
    }
    if ((checked.method == "GET" || checked.method == "HEAD") && !request.body.empty()) {
        throw HttpException{std::format("A {} request has no body", checked.method)};
    }

    const std::unique_ptr<CURLU, UrlDeleter> url{curl_url()};
    if (url == nullptr || curl_url_set(url.get(), CURLUPART_URL, request.url.c_str(), 0) != CURLUE_OK) {
        throw HttpException{std::format("'{}' is not a URL", request.url)};
    }
    const std::string scheme = toLower(getPart(url.get(), CURLUPART_SCHEME));
    if (scheme != "https" && !(scheme == "http" && _settings.allowHttp)) {
        throw HttpException{std::format("'{}': only https:// URLs are allowed{}", request.url,
                                        scheme == "http" ? " (http.allowHttp is false)" : "")};
    }
    const std::string host = toLower(getPart(url.get(), CURLUPART_HOST));
    if (std::ranges::find(_settings.allowedHosts, host) == _settings.allowedHosts.end()) {
        throw HttpException{std::format("'{}': the host '{}' is not in http.allowedHosts", request.url, host)};
    }
    checked.url = request.url;
    checked.headers = checkHeaders(request.headers);
    checked.body = request.body;

    const double seconds = request.timeout > 0.0 && std::isfinite(request.timeout)
                               ? std::min(request.timeout, _settings.timeout)
                               : _settings.timeout;
    checked.timeoutMs = std::max(1L, std::lround(seconds * 1000.0));
    checked.priority = request.priority;
    checked.deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{checked.timeoutMs};
    return checked;
}

std::vector<std::string> CurlClient::checkHeaders(const std::string& headers) const {
    std::vector<std::string> lines;
    std::string_view rest{headers};
    while (!rest.empty()) {
        const std::size_t end = rest.find('\n');
        std::string line{rest.substr(0, end)};
        rest = end == std::string_view::npos ? std::string_view{} : rest.substr(end + 1);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        const std::size_t colon = line.find(':');
        if (colon == std::string::npos || !isHeaderName(std::string_view{line}.substr(0, colon)) ||
            line.contains('\r')) {
            throw HttpException{std::format("'{}' is not a header (expected 'Name: value')", line)};
        }
        std::string resolved;
        std::size_t position = 0;
        while (true) {
            const std::size_t start = line.find(kSecretStart, position);
            if (start == std::string::npos) {
                resolved += line.substr(position);
                break;
            }
            const std::size_t stop = line.find(kSecretEnd, start + kSecretStart.size());
            if (stop == std::string::npos) {
                throw HttpException{"A header has '{{env:' without its closing '}}'"};
            }
            const std::string name = line.substr(start + kSecretStart.size(), stop - start - kSecretStart.size());
            if (std::ranges::find(_settings.secrets, name) == _settings.secrets.end()) {
                throw HttpException{std::format("The secret '{}' is not in http.secrets", name)};
            }
            const std::optional<std::string> value = readEnvironment(name);
            if (!value) {
                throw HttpException{std::format("The secret '{}' is not set in the environment", name)};
            }
            resolved += line.substr(position, start - position);
            resolved += *value;
            position = stop + kSecretEnd.size();
        }
        lines.push_back(std::move(resolved));
    }
    return lines;
}

void CurlClient::start(Checked request) {
    auto transfer = std::make_unique<Transfer>();
    transfer->request = std::move(request);
    transfer->limit = _settings.maxResponseBytes;
    transfer->easy = curl_easy_init();
    if (transfer->easy == nullptr) {
        throw HttpException{"The HTTP client could not start a request"};
    }
    try {
        CURL* easy = transfer->easy;
        const Checked& checked = transfer->request;
        setOption(easy, CURLOPT_URL, checked.url.c_str());
        setOption(easy, CURLOPT_PROTOCOLS_STR, _settings.allowHttp ? "http,https" : "https");
        setOption(easy, CURLOPT_FOLLOWLOCATION, 0L);
        setOption(easy, CURLOPT_NOSIGNAL, 1L);
        setOption(easy, CURLOPT_TIMEOUT_MS, checked.timeoutMs);
        setOption(easy, CURLOPT_CONNECTTIMEOUT_MS, std::min(checked.timeoutMs, kMaxConnectMs));
        setOption(easy, CURLOPT_USERAGENT, "aylo-engine");
        setOption(easy, CURLOPT_ERRORBUFFER, transfer->error.data());
        setOption(easy, CURLOPT_WRITEFUNCTION, &CurlClient::write);
        setOption(easy, CURLOPT_WRITEDATA, transfer.get());
        setOption(easy, CURLOPT_PRIVATE, transfer.get());
        if (checked.method == "HEAD") {
            setOption(easy, CURLOPT_NOBODY, 1L);
        } else if (checked.method != "GET") {
            setOption(easy, CURLOPT_CUSTOMREQUEST, checked.method.c_str());
            setOption(easy, CURLOPT_POSTFIELDS, checked.body.c_str());
            setOption(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(checked.body.size()));
        }
        // No "Expect: 100-continue" wait before a body.
        transfer->headers = curl_slist_append(transfer->headers, "Expect:");
        for (const std::string& header : checked.headers) {
            curl_slist* appended = curl_slist_append(transfer->headers, header.c_str());
            if (appended == nullptr) {
                throw HttpException{"The HTTP client could not set up a request"};
            }
            transfer->headers = appended;
        }
        setOption(easy, CURLOPT_HTTPHEADER, transfer->headers);
        if (curl_multi_add_handle(_multi, easy) != CURLM_OK) {
            throw HttpException{"The HTTP client could not start a request"};
        }
    } catch (...) {
        curl_slist_free_all(transfer->headers);
        curl_easy_cleanup(transfer->easy);
        throw;
    }
    const RequestId id = transfer->request.id;
    _active.emplace(id, std::move(transfer));
}

void CurlClient::startQueued() {
    while (!_queue.empty() && _active.size() < _settings.maxConcurrent) {
        Checked next = std::move(_queue.front());
        _queue.pop_front();
        const RequestId id = next.id;
        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(next.deadline - Clock::now());
        if (left.count() <= 0) {
            _failed.push_back(HttpResult{.request = id, .error = "timeout"});
            continue;
        }
        // The transfer gets what is left of the request's time, not a fresh timeout.
        next.timeoutMs = static_cast<long>(left.count());  // NOLINT(google-runtime-int)
        try {
            start(std::move(next));
        } catch (const HttpException& error) {
            _failed.push_back(HttpResult{.request = id, .error = error.what()});
        }
    }
}

void CurlClient::expireQueued() {
    const auto now = Clock::now();
    for (auto queued = _queue.begin(); queued != _queue.end();) {
        if (queued->deadline <= now) {
            _failed.push_back(HttpResult{.request = queued->id, .error = "timeout"});
            queued = _queue.erase(queued);
        } else {
            ++queued;
        }
    }
}

void CurlClient::release(Transfer& transfer) noexcept {
    if (transfer.easy != nullptr) {
        curl_multi_remove_handle(_multi, transfer.easy);
        curl_easy_cleanup(transfer.easy);
        transfer.easy = nullptr;
    }
    curl_slist_free_all(transfer.headers);
    transfer.headers = nullptr;
}

HttpResult CurlClient::finish(const Transfer& transfer, const CURLcode code) {
    HttpResult result{.request = transfer.request.id};
    if (code == CURLE_OK) {
        long status = 0;  // NOLINT(google-runtime-int): curl gives a long
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): curl_easy_getinfo is a C variadic function
        curl_easy_getinfo(transfer.easy, CURLINFO_RESPONSE_CODE, &status);
        result.status = static_cast<std::int32_t>(status);
        result.body = transfer.response;
        return result;
    }
    if (transfer.tooLarge) {
        result.error = std::format("The response is larger than http.maxResponseBytes ({} bytes)", transfer.limit);
    } else if (transfer.error.front() != '\0') {
        result.error = std::string{transfer.error.data()};
    } else {
        result.error = std::string{curl_easy_strerror(code)};
    }
    return result;
}

std::size_t CurlClient::write(char* data, const std::size_t size, const std::size_t count, void* user) {
    auto* transfer = static_cast<Transfer*>(user);
    const std::size_t bytes = size * count;
    if (transfer->response.size() + bytes > transfer->limit) {
        transfer->tooLarge = true;
        return 0;  // less than given: curl stops the transfer
    }
    transfer->response.append(data, bytes);
    return bytes;
}

}  // namespace rtype::plugins::http::detail
