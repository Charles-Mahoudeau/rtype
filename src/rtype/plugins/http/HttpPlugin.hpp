/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HttpPlugin
*/

#pragma once

#include <span>
#include <string_view>

#include "plugins/http/Export.hpp"
#include "plugins/http/HttpSettings.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::http {

/// @brief HTTPS requests for scripts: inserts the HttpClient resource, registers the HttpResponse event and the
/// `rtype.http.request` and `rtype.http.cancel` engine calls (docs/Http.md).
///
/// @details One system, "rtype.Http.Poll", in PreUpdate: it advances the transfers without waiting and sends an
/// HttpResponse for each request that ended. A game adds the plugin where its requests are made, the server in a
/// networked game, so that its secrets (http.secrets) never reach a client. It requires no other plugin.
class RTYPE_PLUGIN_HTTP_API HttpPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Http";             ///< The plugin's name.
    static constexpr std::string_view kPollSystem = "rtype.Http.Poll";  ///< Advances the transfers.

    explicit HttpPlugin(HttpSettings settings = {});
    ~HttpPlugin() override = default;
    HttpPlugin(const HttpPlugin& other) = default;
    HttpPlugin& operator=(const HttpPlugin& other) = default;
    HttpPlugin(HttpPlugin&& other) noexcept = default;
    HttpPlugin& operator=(HttpPlugin&& other) noexcept = default;

    /// @return "rtype.Http".
    [[nodiscard]] std::string_view getName() const noexcept override;
    /// @return No plugin: it needs none.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;
    /// @brief Inserts the client, registers the event and the calls, and adds the poll system.
    void build(ecs::App& app) override;

  private:
    HttpSettings _settings;  ///< The `http` section of the project.
};

}  // namespace rtype::plugins::http
