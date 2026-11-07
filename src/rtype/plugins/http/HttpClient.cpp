/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HttpClient
*/

#include "plugins/http/HttpClient.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "plugins/http/HttpSettings.hpp"
#include "plugins/http/detail/CurlClient.hpp"

namespace rtype::plugins::http {

HttpClient::HttpClient(HttpSettings settings) : _client{std::make_unique<detail::CurlClient>(std::move(settings))} {}

HttpClient::~HttpClient() = default;

HttpClient::HttpClient(HttpClient&& other) noexcept = default;

HttpClient& HttpClient::operator=(HttpClient&& other) noexcept = default;

RequestId HttpClient::send(const HttpRequest& request) { return _client->send(request); }

bool HttpClient::cancel(const RequestId request) { return _client->cancel(request); }

std::vector<HttpResult> HttpClient::poll() { return _client->poll(); }

std::size_t HttpClient::getPendingCount() const noexcept { return _client->getPendingCount(); }

}  // namespace rtype::plugins::http
