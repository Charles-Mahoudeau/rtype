/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginFinishTests
*/

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::test::plugin_finish {
/// @brief Inserted by a plugin's build, read by another's finish.
struct Late {
    int value;
};
RTYPE_ECS_RESOURCE(Late, "test.finish.Late", value);
}  // namespace rtype::test::plugin_finish

namespace {
/// @brief Logs its build and its finish as "<name>.build" and "<name>.finish", and runs what it is given in them.
class LoggingPlugin : public rtype::ecs::Plugin {
  public:
    using Step = void (*)(rtype::ecs::App& app, std::vector<std::string>& log);

    LoggingPlugin(std::string name, std::vector<std::string>& log, Step onBuild = nullptr, Step onFinish = nullptr)
        : _name{std::move(name)}, _log{&log}, _onBuild{onBuild}, _onFinish{onFinish} {}
    ~LoggingPlugin() override = default;
    LoggingPlugin(const LoggingPlugin& other) = default;
    LoggingPlugin& operator=(const LoggingPlugin& other) = default;
    LoggingPlugin(LoggingPlugin&& other) noexcept = default;
    LoggingPlugin& operator=(LoggingPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return _name; }

    void build(rtype::ecs::App& app) override {
        _log->push_back(_name + ".build");
        if (_onBuild != nullptr) {
            _onBuild(app, *_log);
        }
    }

    void finish(rtype::ecs::App& app) override {
        _log->push_back(_name + ".finish");
        if (_onFinish != nullptr) {
            _onFinish(app, *_log);
        }
    }

  private:
    std::string _name;               ///< Its name, as logged.
    std::vector<std::string>* _log;  ///< Where it logs.
    Step _onBuild;                   ///< Run in build(), if any.
    Step _onFinish;                  ///< Run in finish(), if any.
};
}  // namespace

TEST(PluginFinish, FinishesEveryPluginOnceInOrderBeforeTheFirstFrame) {
    std::vector<std::string> log;
    rtype::ecs::App app;
    app.addPlugin(LoggingPlugin{"a", log}).addPlugin(LoggingPlugin{"b", log});
    EXPECT_EQ(log, (std::vector<std::string>{"a.build", "b.build"}));
    app.runFrame();
    app.runFrame();
    app.finishPlugins();  // already done: nothing again
    EXPECT_EQ(log, (std::vector<std::string>{"a.build", "b.build", "a.finish", "b.finish"}));
}

TEST(PluginFinish, AFinishSeesWhatLaterPluginsBuilt) {
    std::vector<std::string> log;
    rtype::ecs::App app;
    app.addPlugin(LoggingPlugin{"reader", log, nullptr, [](rtype::ecs::App& app, std::vector<std::string>& log) {
                                    const auto* late =
                                        app.getWorld().getResources().get<rtype::test::plugin_finish::Late>();
                                    log.push_back(late != nullptr ? std::to_string(late->value) : "none");
                                }});
    app.addPlugin(LoggingPlugin{"writer", log, [](rtype::ecs::App& app, std::vector<std::string>& /*log*/) {
                                    app.insertResource(rtype::test::plugin_finish::Late{.value = 7});
                                }});
    app.finishPlugins();
    EXPECT_EQ(log, (std::vector<std::string>{"reader.build", "writer.build", "reader.finish", "7", "writer.finish"}));
}

TEST(PluginFinish, FinishesAPluginAddedLaterRightAfterItsBuild) {
    std::vector<std::string> log;
    rtype::ecs::App app;
    app.finishPlugins();
    app.addPlugin(LoggingPlugin{"late", log});
    EXPECT_EQ(log, (std::vector<std::string>{"late.build", "late.finish"}));
}

TEST(PluginFinish, FinishesAPluginAddedByAFinishOnce) {
    std::vector<std::string> log;
    rtype::ecs::App app;
    app.addPlugin(LoggingPlugin{"parent", log, nullptr, [](rtype::ecs::App& app, std::vector<std::string>& log) {
                                    app.addPlugin(LoggingPlugin{"child", log});
                                }});
    app.finishPlugins();
    EXPECT_EQ(log, (std::vector<std::string>{"parent.build", "parent.finish", "child.build", "child.finish"}));
}
