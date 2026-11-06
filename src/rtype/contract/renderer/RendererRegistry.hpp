/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RendererRegistry
*/

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "core/config/Settings.hpp"
#include "rtype/contract/renderer/Export.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"

namespace rtype::contract::renderer {

/// @brief The name a RendererRegistry is offered under to the libraries.
inline constexpr std::string_view kRendererRegistryService = "rtype.contract.renderers";

/// @brief Renderers by name, so the configuration picks one instead of the code.
///
/// @details Each module registers its implementations, then create() builds the one a config names. A factory receives
/// the section named after it, and turns it into its own typed configuration.
class RTYPE_CONTRACT_RENDERER_API RendererRegistry {
  public:
    using Factory = std::function<std::unique_ptr<IRenderer>(const core::config::Settings&)>;

    RendererRegistry() = default;
    ~RendererRegistry() = default;
    RendererRegistry(const RendererRegistry& other) = delete;
    RendererRegistry& operator=(const RendererRegistry& other) = delete;
    RendererRegistry(RendererRegistry&& other) noexcept = default;
    RendererRegistry& operator=(RendererRegistry&& other) noexcept = default;

    /// @return The registered names, sorted.
    [[nodiscard]] std::vector<std::string> getNames() const;

    /// @brief Registers a renderer under a name, replacing any previous one with the same name.
    void add(std::string name, Factory factory);

    /// @return The renderer the config names in `renderer`, built from the section of that name; not started.
    /// @throws platform::exceptions::BackendException If the name is missing or not registered, or its factory returns
    /// nothing.
    [[nodiscard]] std::unique_ptr<IRenderer> create(const core::config::Settings& config) const;

  private:
    std::map<std::string, Factory, std::less<>> _factories;  ///< Factories by name.
};
}  // namespace rtype::contract::renderer
