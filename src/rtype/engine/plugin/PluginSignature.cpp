/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginSignature
*/

#include "PluginSignature.hpp"

#include <sodium/core.h>
#include <sodium/crypto_sign.h>
#include <sodium/utils.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <string_view>
#include <vector>

#include "rtype/engine/Exceptions.hpp"

namespace rtype::engine::plugin {
namespace {
static_assert(kPublicKeySize == crypto_sign_PUBLICKEYBYTES);
static_assert(kSecretKeySize == crypto_sign_SECRETKEYBYTES);
static_assert(kSignatureSize == crypto_sign_BYTES);

/// @return The bytes of the file at @p path.
/// @throws exceptions::RuntimeError If it cannot be opened.
[[nodiscard]] std::vector<std::uint8_t> readBytes(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        throw exceptions::RuntimeError{std::format("Cannot read '{}'", path.generic_string())};
    }
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

/// @return The signature file of @p library, read.
/// @throws exceptions::RuntimeError If it is missing or has the wrong size.
[[nodiscard]] PluginSignature readSignature(const std::filesystem::path& library) {
    std::filesystem::path path = library;
    path += kSignatureExtension;
    if (!std::filesystem::exists(path)) {
        throw exceptions::RuntimeError{std::format("Plugin library '{}' is not signed", library.generic_string())};
    }
    const std::vector<std::uint8_t> bytes = readBytes(path);
    PluginSignature signature;
    if (bytes.size() != sizeof(signature)) {
        throw exceptions::RuntimeError{std::format("The signature of '{}' is malformed", library.generic_string())};
    }
    const auto start = bytes.begin();
    const auto certificate = start + kPublicKeySize;
    const auto body = certificate + kSignatureSize;
    std::copy(start, certificate, signature.developer.begin());
    std::copy(certificate, body, signature.certificate.begin());
    std::copy(body, bytes.end(), signature.signature.begin());
    return signature;
}
}  // namespace

std::optional<PublicKey> parsePublicKey(const std::string_view text) {
    PublicKey key{};
    std::size_t length = 0;
    if (text.size() != key.size() * 2 ||
        sodium_hex2bin(key.data(), key.size(), text.data(), text.size(), nullptr, &length, nullptr) != 0 ||
        length != key.size()) {
        return std::nullopt;
    }
    return key;
}

void verifyPluginSignature(const std::filesystem::path& library, const PublicKey& root) {
    if (sodium_init() < 0) {
        throw exceptions::RuntimeError{"Cannot start the signature checks"};
    }
    const PluginSignature signature = readSignature(library);
    if (crypto_sign_verify_detached(signature.certificate.data(), signature.developer.data(),
                                    signature.developer.size(), root.data()) != 0) {
        throw exceptions::RuntimeError{
            std::format("Plugin library '{}' is signed by a key the engine does not trust", library.generic_string())};
    }
    const std::vector<std::uint8_t> bytes = readBytes(library);
    if (crypto_sign_verify_detached(signature.signature.data(), bytes.data(), bytes.size(),
                                    signature.developer.data()) != 0) {
        throw exceptions::RuntimeError{
            std::format("Plugin library '{}' does not match its signature", library.generic_string())};
    }
}
}  // namespace rtype::engine::plugin
