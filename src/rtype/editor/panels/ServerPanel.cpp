/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerPanel
*/

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <string>
#include <tl/expected.hpp>
#include <vector>

#include "Panels.hpp"
#include "editor/EditorState.hpp"
#include "editor/console/ConsoleLog.hpp"
#include "editor/gui/Text.hpp"
#include "editor/gui/Theme.hpp"
#include "editor/process/ClientLauncher.hpp"
#include "editor/process/Runtime.hpp"
#include "editor/server/AdminSession.hpp"
#include "editor/server/ServerPanelState.hpp"
#include "editor/server/ServerSupervisor.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/admin/WorldProtocol.hpp"
#include "rtype/network/server/Session.hpp"
#include "rtype/network/simulation/LinkSimulator.hpp"

namespace rtype::editor::panels {

namespace {
namespace admin = network::admin;

void line(const std::string& text) { ImGui::TextUnformatted(text.c_str()); }

void coloredLine(const gui::Tone tone, const std::string& text) { gui::textTone(text, tone); }

gui::Tone colorOf(const console::Level level) {
    switch (level) {
        case console::Level::kError:
            return gui::Tone::kDanger;
        case console::Level::kWarning:
            return gui::Tone::kWarning;
        default:
            return gui::Tone::kText;
    }
}

/// @return @p bytes as "12.3 KiB".
std::string sizeText(const std::uint64_t bytes) {
    constexpr double kKi = 1024.0;
    if (static_cast<double>(bytes) < kKi) {
        return std::format("{} B", bytes);
    }
    if (static_cast<double>(bytes) < kKi * kKi) {
        return std::format("{:.1f} KiB", static_cast<double>(bytes) / kKi);
    }
    return std::format("{:.1f} MiB", static_cast<double>(bytes) / (kKi * kKi));
}

/// @return @p milliseconds as "1m 05s".
std::string durationText(const std::uint64_t milliseconds) {
    const std::uint64_t seconds = milliseconds / 1000;
    return seconds >= 60 ? std::format("{}m {:02}s", seconds / 60, seconds % 60) : std::format("{}s", seconds);
}

/// @return What the user typed in @p field.
std::string textOf(const std::array<char, 256>& field) { return std::string{field.data()}; }

server::ServerOptions serverOptions(const EditorState& state) {
    const server::ServerPanelState& panel = state.serverPanel;
    return server::ServerOptions{
        .gameDirectory = std::filesystem::path{std::string{panel.gameDirectory.data()}},
        .port = static_cast<std::uint16_t>(std::clamp(panel.port, 0, 65535)),
        .extraArgs = process::splitArguments(textOf(panel.serverArgs)),
        .runtime = {},
    };
}

process::ClientOptions clientOptions(const EditorState& state) {
    const server::ServerPanelState& panel = state.serverPanel;
    return process::ClientOptions{
        .gameDirectory = std::filesystem::path{std::string{panel.gameDirectory.data()}},
        .host = std::string{panel.host.data()},
        .port = static_cast<std::uint16_t>(std::clamp(panel.port, 0, 65535)),
        .runtime = {},
    };
}

void remember(EditorState& state, const tl::expected<void, std::string>& result) {
    state.serverPanel.message = result ? std::string{} : result.error();
}

void drawControls(EditorState& state) {
    server::ServerSupervisor& supervisor = *state.supervisor;
    server::ServerPanelState& panel = state.serverPanel;
    const server::ServerStatus status = supervisor.status();
    const bool active = status == server::ServerStatus::kStarting || status == server::ServerStatus::kRunning;

    ImGui::BeginDisabled(active);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##game", "Game folder", panel.gameDirectory.data(), panel.gameDirectory.size());
    ImGui::SetNextItemWidth(100.0F);
    ImGui::InputInt("Port", &panel.port, 0);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##serverArgs", "More flags (--name value ...)", panel.serverArgs.data(),
                             panel.serverArgs.size());
    ImGui::EndDisabled();

    if (!active) {
        if (ImGui::Button("Start")) {
            remember(state, supervisor.start(serverOptions(state)));
        }
    } else {
        if (ImGui::Button("Stop")) {
            supervisor.stop();
        }
        ImGui::SameLine();
        if (ImGui::Button("Restart")) {
            remember(state, supervisor.restart());
            if (panel.restartClients && state.clients->runningCount() > 0 &&
                supervisor.status() != server::ServerStatus::kStopped) {
                remember(state, state.clients->restartAll(clientOptions(state)));
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Restart the clients too", &panel.restartClients);
    }
    if (!panel.message.empty()) {
        coloredLine(gui::Tone::kDanger, panel.message);
    }
}

void drawStatusLine(const EditorState& state) {
    server::ServerSupervisor& supervisor = *state.supervisor;
    switch (supervisor.status()) {
        case server::ServerStatus::kStopped:
            gui::textDisabled("Stopped");
            return;
        case server::ServerStatus::kStarting:
            coloredLine(gui::Tone::kWarning, std::format("Starting on port {}...", supervisor.options().port));
            return;
        case server::ServerStatus::kExited:
            coloredLine(gui::Tone::kDanger, std::format("Exited with code {}", supervisor.exitCode().value_or(0)));
            return;
        case server::ServerStatus::kRunning:
            break;
    }
    coloredLine(gui::Tone::kSuccess, std::format("Running on port {}", supervisor.options().port));
    const server::AdminSession& session = *supervisor.session();
    if (const auto& status = session.status()) {
        line(std::format("Tick {}   Entities {}   Clients {}   Up {}", status->tick, status->entityCount,
                         status->clientCount, durationText(status->uptimeMs)));
        line(std::format("Received {} ({} datagrams)   Sent {} ({} datagrams)", sizeText(status->bytesReceived),
                         status->datagramsReceived, sizeText(status->bytesSent), status->datagramsSent));
    }
    if (!session.lastError().empty()) {
        coloredLine(gui::Tone::kDanger, session.lastError());
    }
}

void drawLog(EditorState& state) {
    console::ConsoleLog& log = state.supervisor->log();
    if (ImGui::Button("Clear")) {
        log.clear();
    }
    if (ImGui::BeginChild("##serverLog", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        for (const console::Line& entry : log.getLines()) {
            coloredLine(colorOf(entry.level), entry.text);
        }
        const std::size_t added = log.getAdded();
        if (added != state.serverPanel.logSeen && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0F) {
            ImGui::SetScrollHereY(1.0F);
        }
        state.serverPanel.logSeen = added;
    }
    ImGui::EndChild();
}

void drawConnected(server::AdminSession& session) {
    const auto& clients = session.clients();
    if (!clients) {
        gui::textDisabled("Waiting for the server...");
        return;
    }
    line(std::format("{} connected", clients->total));
    if (ImGui::BeginTable("##clients", 7, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Id");
        ImGui::TableSetupColumn("Address");
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("RTT");
        ImGui::TableSetupColumn("Dropped");
        ImGui::TableSetupColumn("Errors");
        ImGui::TableSetupColumn("##kick", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();
        for (const admin::ClientEntry& entry : clients->clients) {
            ImGui::PushID(static_cast<int>(entry.client));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            line(std::to_string(entry.client));
            ImGui::TableNextColumn();
            line(entry.address.string());
            ImGui::TableNextColumn();
            line(entry.state == network::server::SessionState::kReady ? "Ready" : "Welcoming");
            ImGui::TableNextColumn();
            line(std::format("{:.1f} ms", entry.rttMs));
            ImGui::TableNextColumn();
            line(std::to_string(entry.droppedMessages));
            ImGui::TableNextColumn();
            line(std::to_string(entry.protocolErrors));
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("Kick")) {
                session.kick(entry.client);
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (clients->total > clients->clients.size()) {
        gui::textDisabled(std::format("{} more not shown", clients->total - clients->clients.size()));
    }
}

void drawNetwork(EditorState& state, server::AdminSession& session) {
    server::ServerPanelState& panel = state.serverPanel;
    if (const auto& current = session.simulation(); current && !ImGui::IsAnyItemActive()) {
        panel.latencyMs = current->latencyMs;
        panel.jitterMs = current->jitterMs;
        panel.lossPercent = current->lossPercent;
    }
    gui::textDisabled("Applied to the server's datagrams, in both directions.");
    bool changed = false;
    ImGui::SliderInt("Latency (ms)", &panel.latencyMs, 0, network::simulation::kMaxDelayMs);
    changed |= ImGui::IsItemDeactivatedAfterEdit();
    ImGui::SliderInt("Jitter (ms)", &panel.jitterMs, 0, network::simulation::kMaxDelayMs);
    changed |= ImGui::IsItemDeactivatedAfterEdit();
    ImGui::SliderFloat("Loss (%)", &panel.lossPercent, 0.0F, 100.0F, "%.1f");
    changed |= ImGui::IsItemDeactivatedAfterEdit();
    if (ImGui::Button("Off")) {
        panel.latencyMs = 0;
        panel.jitterMs = 0;
        panel.lossPercent = 0.0F;
        changed = true;
    }
    if (changed) {
        session.setSimulation(network::simulation::SimulationSettings{
            .latencyMs = static_cast<std::uint16_t>(panel.latencyMs),
            .jitterMs = static_cast<std::uint16_t>(panel.jitterMs),
            .lossPercent = panel.lossPercent,
        });
    }
}

void drawFields(server::AdminSession& session, const admin::ComponentView& component, const std::uint64_t entity) {
    for (const admin::FieldView& field : component.fields) {
        ImGui::PushID(field.name.c_str());
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        line(field.name);
        ImGui::TableNextColumn();
        gui::textDisabled(field.type);
        ImGui::TableNextColumn();
        if (field.editable && field.count == 1) {
            std::array<char, 128> buffer{};
            const std::size_t length = std::min(field.value.size(), buffer.size() - 1);
            std::copy_n(field.value.begin(), length, buffer.begin());
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::InputText("##value", buffer.data(), buffer.size());
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                session.setField(admin::SetFieldRequest{
                    .entity = entity,
                    .component = component.name,
                    .field = field.name,
                    .index = 0,
                    .value = std::string{buffer.data()},
                });
            }
        } else {
            line(field.value);
        }
        ImGui::PopID();
    }
}

void drawInspected(EditorState& state, server::AdminSession& session) {
    server::ServerPanelState& panel = state.serverPanel;
    if (!panel.entity) {
        gui::textDisabled("Select an entity.");
        return;
    }
    session.watchEntity(*panel.entity, panel.firstComponent);
    const auto& inspected = session.inspected();
    if (!inspected || inspected->entity != *panel.entity) {
        gui::textDisabled("Reading the entity...");
        return;
    }
    line(std::format("{} ({} components)", inspected->name.empty() ? "Entity" : inspected->name, inspected->total));
    ImGui::SameLine();
    if (ImGui::SmallButton("Despawn")) {
        session.despawn(*panel.entity);
        panel.entity.reset();
        return;
    }
    if (panel.firstComponent > 0 && ImGui::SmallButton("Previous components")) {
        panel.firstComponent = 0;
    }
    for (const admin::ComponentView& component : inspected->components) {
        ImGui::PushID(component.name.c_str());
        if (ImGui::CollapsingHeader(component.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen) &&
            ImGui::BeginTable("##fields", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Field");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            drawFields(session, component, *panel.entity);
            ImGui::EndTable();
        }
        ImGui::PopID();
    }
    if (inspected->next < inspected->total && ImGui::SmallButton("More components")) {
        panel.firstComponent = inspected->next;
    }
}

void drawWorld(EditorState& state, server::AdminSession& session) {
    server::ServerPanelState& panel = state.serverPanel;
    const bool paused = session.isPaused().value_or(false);
    line(paused ? "Simulation paused" : "Simulation running");
    ImGui::SameLine();
    if (ImGui::SmallButton(paused ? "Resume" : "Pause")) {
        session.run(paused ? admin::RunAction::kResume : admin::RunAction::kPause);
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!paused);
    if (ImGui::SmallButton("Step")) {
        session.run(admin::RunAction::kStep);
    }
    ImGui::EndDisabled();

    ImGui::SetNextItemWidth(160.0F);
    ImGui::InputTextWithHint("##spawnName", "Name", panel.spawnName.data(), panel.spawnName.size());
    ImGui::SameLine();
    if (ImGui::Button("Spawn")) {
        session.spawn(std::string{panel.spawnName.data()});
    }
    ImGui::Separator();

    session.watchEntities(panel.entitiesFirst);
    const auto& entities = session.entities();
    if (!entities) {
        gui::textDisabled("Reading the world...");
        return;
    }
    const auto count = static_cast<std::uint32_t>(entities->entries.size());
    line(std::format("{} entities, from {}", entities->total, entities->first));
    ImGui::SameLine();
    if (ImGui::SmallButton("<") && panel.entitiesFirst > 0) {
        panel.entitiesFirst = panel.entitiesFirst > count ? panel.entitiesFirst - std::max(count, 1U) : 0;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(">") && entities->first + count < entities->total) {
        panel.entitiesFirst = entities->first + std::max(count, 1U);
    }
    if (ImGui::BeginChild("##entities", ImVec2{0.0F, 140.0F}, ImGuiChildFlags_Borders)) {
        for (const admin::EntityEntry& entry : entities->entries) {
            const bool selected = panel.entity == entry.entity;
            const std::string label = std::format("{} ({})##{}", entry.name.empty() ? "Entity" : entry.name,
                                                  entry.componentCount, entry.entity);
            if (ImGui::Selectable(label.c_str(), selected)) {
                panel.entity = entry.entity;
                panel.firstComponent = 0;
            }
        }
    }
    ImGui::EndChild();
    drawInspected(state, session);
}

void drawLaunch(EditorState& state) {
    server::ServerPanelState& panel = state.serverPanel;
    process::ClientLauncher& clients = *state.clients;
    ImGui::SetNextItemWidth(160.0F);
    ImGui::InputText("Host", panel.host.data(), panel.host.size());
    ImGui::SetNextItemWidth(100.0F);
    ImGui::InputInt("Clients", &panel.clientCount);
    panel.clientCount = std::clamp(panel.clientCount, 1, 16);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##clientArgs", "Flags for these clients (--name p2 ...)", panel.clientArgs.data(),
                             panel.clientArgs.size());
    if (ImGui::Button("Launch")) {
        panel.message.clear();
        const std::vector<std::string> extra = process::splitArguments(textOf(panel.clientArgs));
        for (int i = 0; i < panel.clientCount; ++i) {
            if (const auto launched = clients.launch(clientOptions(state), extra); !launched) {
                panel.message = launched.error();
                break;
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Kill all")) {
        clients.killAll();
    }
    ImGui::SameLine();
    if (ImGui::Button("Restart all")) {
        remember(state, clients.restartAll(clientOptions(state)));
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove ended")) {
        clients.removeEnded();
    }
    if (!panel.message.empty()) {
        coloredLine(gui::Tone::kDanger, panel.message);
    }
    if (ImGui::BeginTable("##launched", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn("Process");
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("Flags");
        ImGui::TableSetupColumn("##kill", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();
        for (const process::ClientInfo& info : clients.list()) {
            ImGui::PushID(info.number);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            line(std::to_string(info.number));
            ImGui::TableNextColumn();
            line(info.running ? std::to_string(info.pid) : "-");
            ImGui::TableNextColumn();
            line(info.running ? "Running" : std::format("Ended ({})", info.exitCode.value_or(0)));
            ImGui::TableNextColumn();
            std::string flags;
            for (const std::string& arg : info.args) {
                flags += arg + ' ';
            }
            line(flags);
            ImGui::TableNextColumn();
            ImGui::BeginDisabled(!info.running);
            if (ImGui::SmallButton("Kill")) {
                clients.kill(info.number);
            }
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}
/// @brief A tab whose content needs the admin channel: @p draw gets the session once the server answers.
template <typename Draw>
void drawAdminTab(const char* name, server::AdminSession* session, const Draw& draw) {
    if (!ImGui::BeginTabItem(name)) {
        return;
    }
    if (session != nullptr && session->isConnected()) {
        draw(*session);
    } else {
        gui::textDisabled("The server is not running.");
    }
    ImGui::EndTabItem();
}

void drawTabs(EditorState& state) {
    if (!ImGui::BeginTabBar("##serverTabs")) {
        return;
    }
    server::AdminSession* session = state.supervisor->session();
    if (ImGui::BeginTabItem("Log")) {
        drawLog(state);
        ImGui::EndTabItem();
    }
    drawAdminTab("Players", session, [](server::AdminSession& admin) { drawConnected(admin); });
    drawAdminTab("Network", session, [&state](server::AdminSession& admin) { drawNetwork(state, admin); });
    drawAdminTab("World", session, [&state](server::AdminSession& admin) { drawWorld(state, admin); });
    if (ImGui::BeginTabItem("Clients")) {
        drawLaunch(state);
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
}
}  // namespace

server::ServerOptions serverOptionsOf(const EditorState& state) { return serverOptions(state); }

void drawServerPanel(EditorState& state) {
    // The server and the clients are watched even while the panel is closed: their output keeps coming.
    state.supervisor->update();
    state.clients->update();
    if (!state.showServer) {
        return;
    }
    if (ImGui::Begin("Server", &state.showServer)) {
        drawControls(state);
        drawStatusLine(state);
        ImGui::Separator();
        drawTabs(state);
    }
    ImGui::End();
}

}  // namespace rtype::editor::panels
