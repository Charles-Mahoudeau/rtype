/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CurlClient
*/

#pragma once

#include <curl/curl.h>
#include <curl/multi.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "plugins/http/HttpClient.hpp"
#include "plugins/http/HttpSettings.hpp"

namespace rtype::plugins::http::detail {

/// @brief What HttpClient does, with libcurl: a multi handle polled without waiting, one easy handle per transfer.
/// Private to the plugin, so curl never reaches a public header.
class CurlClient {
  public:
    explicit CurlClient(HttpSettings settings);
    ~CurlClient();
    CurlClient(const CurlClient& other) = delete;
    CurlClient& operator=(const CurlClient& other) = delete;
    CurlClient(CurlClient&& other) = delete;
    CurlClient& operator=(CurlClient&& other) = delete;

    /// @see HttpClient::send()
    RequestId send(const HttpRequest& request);
    /// @see HttpClient::cancel()
    bool cancel(RequestId request);
    /// @see HttpClient::poll()
    [[nodiscard]] std::vector<HttpResult> poll();
    /// @see HttpClient::getPendingCount()
    [[nodiscard]] std::size_t getPendingCount() const noexcept;

  private:
    /// @brief A request checked by send(): what a transfer needs, its secrets already in its headers.
    struct Checked {
        RequestId id{0};
        std::string method;
        std::string url;
        std::vector<std::string> headers;
        std::string body;
        long timeoutMs{0};  // NOLINT(google-runtime-int): curl takes a long
        std::int32_t priority{0};
        std::chrono::steady_clock::time_point deadline;  ///< send() + the timeout: queue time counts.
    };

    /// @brief One transfer in flight. Its address is given to curl, so it stays where it is.
    struct Transfer {
        Checked request;
        CURL* easy{nullptr};
        curl_slist* headers{nullptr};
        std::string response;
        std::size_t limit{0};
        bool tooLarge{false};
        std::array<char, CURL_ERROR_SIZE> error{};
    };

    /// @return @p request, checked: its method, its URL and its host, its headers with their secrets.
    /// @throws exceptions::HttpException If one of them is invalid.
    [[nodiscard]] Checked check(const HttpRequest& request) const;
    /// @return @p headers, one per line, each checked and with its `{{env:NAME}}` replaced.
    [[nodiscard]] std::vector<std::string> checkHeaders(const std::string& headers) const;
    /// @brief Starts @p request: an easy handle, added to the multi handle.
    void start(Checked request);
    /// @brief Starts queued requests while fewer than maxConcurrent are in flight; one that cannot start ends with an
    /// error, given by the next poll().
    void startQueued();
    /// @brief Ends queued requests whose deadline passed, given by the next poll().
    void expireQueued();
    /// @brief Removes @p transfer from the multi handle and frees it.
    void release(Transfer& transfer) noexcept;
    /// @return The result of @p transfer, which curl finished with @p code.
    [[nodiscard]] static HttpResult finish(const Transfer& transfer, CURLcode code);

    /// @brief Appends what curl received to the response, and stops a response over its limit.
    static std::size_t write(char* data, std::size_t size, std::size_t count, void* user);

    HttpSettings _settings;                                  ///< What may be reached, and the limits.
    CURLM* _multi{nullptr};                                  ///< Every transfer in flight.
    std::map<RequestId, std::unique_ptr<Transfer>> _active;  ///< In flight, by id.
    std::deque<Checked> _queue;                              ///< Waiting for a free slot, in order.
    RequestId _next{1};                                      ///< The id of the next request; 0 is none.
    std::vector<HttpResult> _failed;  ///< Queued requests that could not start, given by the next poll().
};

}  // namespace rtype::plugins::http::detail
