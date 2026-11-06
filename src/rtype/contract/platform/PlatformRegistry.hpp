/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PlatformRegistry
*/

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "core/config/Settings.hpp"
#include "rtype/contract/platform/Export.hpp"
#include "rtype/contract/platform/IPlatform.hpp"

namespace rtype::contract::platform {

/// @brief The name a PlatformRegistry is offered under to the libraries.
inline constexpr std::string_view kPlatformRegistryService = "rtype.contract.platforms";

/// @brief Platforms by name, so the configuration picks one instead of the code.
///
/// @details Each module registers its implementations, then create() builds the one a config names. A factory receives
/// the section named after it, and turns it into its own typed configuration.
class RTYPE_CONTRACT_PLATFORM_API PlatformRegistry {
  public:
    using Factory = std::function<std::unique_ptr<IPlatform>(const core::config::Settings&)>;

    PlatformRegistry() = default;
    ~PlatformRegistry() = default;
    PlatformRegistry(const PlatformRegistry& other) = delete;
    PlatformRegistry& operator=(const PlatformRegistry& other) = delete;
    PlatformRegistry(PlatformRegistry&& other) noexcept = default;
    PlatformRegistry& operator=(PlatformRegistry&& other) noexcept = default;

    /// @return The registered names, sorted.
    [[nodiscard]] std::vector<std::string> getNames() const;

    /// @brief Registers a platform under a name, replacing any previous one with the same name.
    void add(std::string name, Factory factory);

    /// @return The platform the config names in `platform`, built from the section of that name; not started.
    /// @throws exceptions::BackendException If the name is missing or not registered, or its factory returns nothing.
    [[nodiscard]] std::unique_ptr<IPlatform> create(const core::config::Settings& config) const;

  private:
    std::map<std::string, Factory, std::less<>> _factories;  ///< Factories by name.
};
}  // namespace rtype::contract::platform
