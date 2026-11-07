/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HttpCalls
*/

#include "plugins/http/HttpCalls.hpp"

#include <array>
#include <string>
#include <string_view>

#include "plugins/http/HttpClient.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::http {
namespace {
RequestResult request(ecs::ResMut<HttpClient> client, const RequestArguments& arguments) {
    return RequestResult{.request = client->send(HttpRequest{.method = std::string{arguments.method.view()},
                                                             .url = arguments.url,
                                                             .headers = arguments.headers,
                                                             .body = arguments.body,
                                                             .timeout = arguments.timeout})};
}

CancelResult cancel(ecs::ResMut<HttpClient> client, const CancelArguments& arguments) {
    return CancelResult{.cancelled = client->cancel(arguments.request)};
}
}  // namespace

void addHttpForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "HttpForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addHttpForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addHttpCalls(ecs::App& app) {
    addHttpForms(app.getPluginRegistry());
    app.addCall(std::string{kRequestCall}, &request);
    app.addCall(std::string{kCancelCall}, &cancel);
}

}  // namespace rtype::plugins::http
