/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MiniaudioException
*/

#include "audio/miniaudio/exceptions/MiniaudioException.hpp"

#include <miniaudio.h>

#include <format>
#include <stdexcept>
#include <string>

namespace rtype::audio::miniaudio::exceptions {

MiniaudioException::MiniaudioException(const std::string& message, ma_result result)
    : std::runtime_error{
          std::format("{}: {} (code {})", message, ma_result_description(result), static_cast<int>(result))},
      _result{result} {}

ma_result MiniaudioException::getResult() const noexcept { return _result; }

}  // namespace rtype::audio::miniaudio::exceptions
