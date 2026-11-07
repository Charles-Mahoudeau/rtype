/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HttpPlugin
*/

#include "plugins/http/HttpPlugin.hpp"

#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "plugins/http/HttpCalls.hpp"
#include "plugins/http/HttpClient.hpp"
#include "plugins/http/HttpEvents.hpp"
#include "plugins/http/HttpSettings.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::http {
namespace {
/// @brief PreUpdate: advances the transfers and sends an HttpResponse for each request that ended.
void pollRequests(ecs::ResMut<HttpClient> client, ecs::EventWriter<HttpResponse> responses) {
    for (HttpResult& result : client->poll()) {
        const bool ok = result.error.empty() && result.status >= 200 && result.status < 300;
        responses.send(HttpResponse{.request = result.request,
                                    .status = result.status,
                                    .ok = ok,
                                    .body = std::move(result.body),
                                    .error = std::move(result.error)});
    }
}
}  // namespace

HttpPlugin::HttpPlugin(HttpSettings settings) : _settings{std::move(settings)} {}

std::string_view HttpPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> HttpPlugin::getDependencies() const noexcept { return {}; }

void HttpPlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    if (!resources.has<HttpClient>()) {
        resources.insert(HttpClient{_settings});
    }
    static_cast<void>(app.getWorld().getEvents().registerType<HttpResponse>());
    addHttpCalls(app);
    app.addSystem(ecs::Phase::kPreUpdate, std::string{kPollSystem}, &pollRequests);
}

}  // namespace rtype::plugins::http
