/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// aylo-sign: makes the keys and signatures of the game plugins.
//   aylo-sign keygen <name>                   writes <name>.pub and <name>.key
//   aylo-sign certify <root.key> <dev.pub>    writes <dev.pub>.cert, the root's signature of a developer's key
//   aylo-sign sign <dev.key> <dev.pub> <library>  writes <library>.sig, with the certificate <dev.pub>.cert

#include <sodium/core.h>
#include <sodium/crypto_sign.h>
#include <sodium/utils.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "rtype/engine/plugin/PluginSignature.hpp"

namespace {
namespace plugin = rtype::engine::plugin;

constexpr int kErrorCode = 84;  ///< What the program returns on an error.

using SecretKey = std::array<std::uint8_t, plugin::kSecretKeySize>;
using Signature = std::array<std::uint8_t, plugin::kSignatureSize>;

/// @return The bytes of the file at @p path.
std::vector<std::uint8_t> readBytes(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        throw std::runtime_error{std::format("Cannot read '{}'", path.generic_string())};
    }
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

/// @brief Writes @p bytes in hexadecimal to @p path.
void writeHex(const std::filesystem::path& path, const std::span<const std::uint8_t> bytes) {
    std::string text((bytes.size() * 2) + 1, '\0');
    sodium_bin2hex(text.data(), text.size(), bytes.data(), bytes.size());
    text.pop_back();
    std::ofstream{path} << text << '\n';
}

/// @return The @p N bytes written in hexadecimal in the file at @p path.
template <std::size_t N>
std::array<std::uint8_t, N> readHex(const std::filesystem::path& path) {
    std::ifstream file{path};
    std::string text;
    file >> text;
    std::array<std::uint8_t, N> bytes{};
    std::size_t length = 0;
    if (sodium_hex2bin(bytes.data(), bytes.size(), text.data(), text.size(), nullptr, &length, nullptr) != 0 ||
        length != N) {
        throw std::runtime_error{std::format("'{}' does not hold {} bytes", path.generic_string(), N)};
    }
    return bytes;
}

void generateKeys(const std::string& name) {
    plugin::PublicKey publicKey{};
    SecretKey secretKey{};
    crypto_sign_keypair(publicKey.data(), secretKey.data());
    writeHex(name + ".pub", publicKey);
    const std::filesystem::path secret = name + ".key";
    writeHex(secret, secretKey);
    std::filesystem::permissions(secret, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
    sodium_memzero(secretKey.data(), secretKey.size());
    std::cout << std::format("Wrote {}.pub and {}.key: keep the second one secret.\n", name, name);
}

void certify(const std::filesystem::path& rootKey, const std::filesystem::path& developer) {
    const auto secretKey = readHex<plugin::kSecretKeySize>(rootKey);
    const auto publicKey = readHex<plugin::kPublicKeySize>(developer);
    Signature certificate{};
    crypto_sign_detached(certificate.data(), nullptr, publicKey.data(), publicKey.size(), secretKey.data());
    std::filesystem::path path = developer;
    path += ".cert";
    writeHex(path, certificate);
}

void sign(const std::filesystem::path& developerKey, const std::filesystem::path& developer,
          const std::filesystem::path& library) {
    const auto secretKey = readHex<plugin::kSecretKeySize>(developerKey);
    const auto publicKey = readHex<plugin::kPublicKeySize>(developer);
    std::filesystem::path certificatePath = developer;
    certificatePath += ".cert";
    const auto certificate = readHex<plugin::kSignatureSize>(certificatePath);
    const std::vector<std::uint8_t> bytes = readBytes(library);
    Signature signature{};
    crypto_sign_detached(signature.data(), nullptr, bytes.data(), bytes.size(), secretKey.data());

    std::filesystem::path path = library;
    path += plugin::kSignatureExtension;
    std::ofstream file{path, std::ios::binary};
    for (const std::span<const std::uint8_t> part :
         {std::span<const std::uint8_t>{publicKey}, std::span<const std::uint8_t>{certificate},
          std::span<const std::uint8_t>{signature}}) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): a stream writes chars, these are bytes.
        file.write(reinterpret_cast<const char*>(part.data()), static_cast<std::streamsize>(part.size()));
    }
}
}  // namespace

int main(const int argc, char** argv) {
    try {
        const std::span arguments{argv, static_cast<std::size_t>(argc)};
        const std::vector<std::string> args{arguments.begin(), arguments.end()};
        const std::string command = args.size() > 1 ? args.at(1) : "";
        if (sodium_init() < 0) {
            throw std::runtime_error{"Cannot start libsodium"};
        }
        if (command == "keygen" && args.size() == 3) {
            generateKeys(args.at(2));
        } else if (command == "certify" && args.size() == 4) {
            certify(args.at(2), args.at(3));
        } else if (command == "sign" && args.size() == 5) {
            sign(args.at(2), args.at(3), args.at(4));
        } else {
            std::cerr << "Usage: aylo-sign keygen <name> | certify <root.key> <dev.pub> | "
                         "sign <dev.key> <dev.pub> <library>\n";
            return kErrorCode;
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return kErrorCode;
    }
}
