/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HttpException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace rtype::plugins::http::exceptions {
/// @brief Exception thrown when a request cannot be sent: its URL, method or headers are invalid, its host is not
/// allowed, it names a secret the settings do not list, or the HTTP library could not start it. A request that is sent
/// and fails (no network, a timeout, an error status) is not an exception: it ends with an HttpResponse.
class RTYPE_HEADER_ONLY_API HttpException : public std::runtime_error {
  public:
    explicit HttpException(const std::string& message) : std::runtime_error{message} {}
    ~HttpException() override = default;
    HttpException(const HttpException& other) = default;
    HttpException& operator=(const HttpException& other) = default;
    HttpException(HttpException&& other) noexcept = default;
    HttpException& operator=(HttpException&& other) noexcept = default;
};
}  // namespace rtype::plugins::http::exceptions
