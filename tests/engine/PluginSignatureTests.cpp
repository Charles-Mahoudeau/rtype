/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginSignatureTests
*/

#include <gtest/gtest.h>
#include <sodium.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/plugin/PluginSignature.hpp"

using rtype::engine::exceptions::RuntimeError;
using rtype::engine::plugin::parsePublicKey;
using rtype::engine::plugin::PublicKey;
using rtype::engine::plugin::verifyPluginSignature;

namespace {
/// @brief A key pair.
struct Keys {
    PublicKey publicKey{};
    std::array<std::uint8_t, rtype::engine::plugin::kSecretKeySize> secretKey{};

    Keys() { crypto_sign_keypair(publicKey.data(), secretKey.data()); }
};

/// @brief A library file with a root and a developer key, in a folder removed when the test ends.
class PluginSignatureTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ASSERT_GE(sodium_init(), 0);
        std::filesystem::create_directories(_directory);
        std::ofstream{_library, std::ios::binary} << "the library";
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_directory, error);
    }

    /// @brief Writes the signature of @p text by @p developer, whose certificate comes from @p root.
    void sign(const Keys& root, const Keys& developer, const std::string_view text) const {
        std::array<std::uint8_t, rtype::engine::plugin::kSignatureSize> certificate{};
        crypto_sign_detached(certificate.data(), nullptr, developer.publicKey.data(), developer.publicKey.size(),
                             root.secretKey.data());
        std::array<std::uint8_t, rtype::engine::plugin::kSignatureSize> signature{};
        const std::span bytes{reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
        crypto_sign_detached(signature.data(), nullptr, bytes.data(), bytes.size(), developer.secretKey.data());
        std::ofstream file{std::filesystem::path{_library} += ".sig", std::ios::binary};
        file.write(reinterpret_cast<const char*>(developer.publicKey.data()), developer.publicKey.size());
        file.write(reinterpret_cast<const char*>(certificate.data()), certificate.size());
        file.write(reinterpret_cast<const char*>(signature.data()), signature.size());
    }

    std::filesystem::path _directory{std::filesystem::temp_directory_path() /
                                     std::format("rtype_signature_{:08x}", std::random_device{}())};
    std::filesystem::path _library{_directory / "boss.so"};
    Keys _root;
    Keys _developer;
};
}  // namespace

TEST_F(PluginSignatureTest, AcceptsALibrarySignedByACertifiedKey) {
    sign(_root, _developer, "the library");
    EXPECT_NO_THROW(verifyPluginSignature(_library, _root.publicKey));
}

TEST_F(PluginSignatureTest, RefusesALibraryWithoutSignature) {
    EXPECT_THROW(verifyPluginSignature(_library, _root.publicKey), RuntimeError);
}

TEST_F(PluginSignatureTest, RefusesAKeyTheRootDidNotCertify) {
    const Keys other;
    sign(other, _developer, "the library");
    EXPECT_THROW(verifyPluginSignature(_library, _root.publicKey), RuntimeError);
}

TEST_F(PluginSignatureTest, RefusesALibraryChangedAfterItsSignature) {
    sign(_root, _developer, "another library");
    EXPECT_THROW(verifyPluginSignature(_library, _root.publicKey), RuntimeError);
}

TEST_F(PluginSignatureTest, RefusesAMalformedSignature) {
    std::ofstream{std::filesystem::path{_library} += ".sig", std::ios::binary} << "short";
    EXPECT_THROW(verifyPluginSignature(_library, _root.publicKey), RuntimeError);
}

TEST(PluginSignature, ParsesAPublicKey) {
    EXPECT_TRUE(parsePublicKey(std::string(64, 'a')));
    EXPECT_FALSE(parsePublicKey(""));
    EXPECT_FALSE(parsePublicKey(std::string(64, 'z')));
}
