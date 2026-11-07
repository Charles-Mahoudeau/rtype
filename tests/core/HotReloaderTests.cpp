/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HotReloaderTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/reload/ChangeSet.hpp"
#include "core/reload/FileWatcher.hpp"
#include "core/reload/HotReloader.hpp"
#include "core/reload/ReloadOutcome.hpp"
#include "core/reload/ReloadReport.hpp"
#include "core/reload/Reloadable.hpp"

using rtype::core::reload::ChangeSet;
using rtype::core::reload::FileWatcher;
using rtype::core::reload::HotReloader;
using rtype::core::reload::Reloadable;
using rtype::core::reload::ReloadOutcome;
using rtype::core::reload::ReloadReport;
using rtype::core::reload::WatchedPath;

namespace {
/// @brief An empty folder in the temporary directory, removed when the test ends.
class TemporaryFolder {
  public:
    TemporaryFolder() : _path{std::filesystem::temp_directory_path() / makeName()} {
        std::filesystem::remove_all(_path);
        std::filesystem::create_directories(_path);
    }
    ~TemporaryFolder() {
        std::error_code error;
        std::filesystem::remove_all(_path, error);
    }
    TemporaryFolder(const TemporaryFolder& other) = delete;
    TemporaryFolder& operator=(const TemporaryFolder& other) = delete;
    TemporaryFolder(TemporaryFolder&& other) = delete;
    TemporaryFolder& operator=(TemporaryFolder&& other) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return _path; }

    /// @brief Writes @p text in the file at @p relative, and gives it the modification time @p version seconds after a
    /// fixed date, so two writes in a row never look alike, whatever the clock's resolution.
    void write(const std::filesystem::path& relative, const std::string_view text, const int version = 0) const {
        const std::filesystem::path file = _path / relative;
        std::filesystem::create_directories(file.parent_path());
        {
            std::ofstream out{file, std::ios::binary};
            out << text;
        }
        std::filesystem::last_write_time(
            file, std::filesystem::file_time_type{} + std::chrono::hours{100000} + std::chrono::seconds{version});
    }

    /// @brief Removes the file at @p relative.
    void remove(const std::filesystem::path& relative) const { std::filesystem::remove(_path / relative); }

  private:
    /// @return A name that no other test of the run uses.
    static std::string makeName() {
        const auto* test = testing::UnitTest::GetInstance()->current_test_info();
        return std::string{"rtype-reload-"} + test->test_suite_name() + "-" + test->name();
    }

    std::filesystem::path _path;  ///< The folder.
};

/// @brief A feature that records what it is asked to reload.
class RecordingReloadable final : public Reloadable {
  public:
    RecordingReloadable(std::string name, std::vector<WatchedPath> watched, ReloadOutcome outcome,
                        std::vector<ChangeSet>* calls)
        : _name{std::move(name)}, _watched{std::move(watched)}, _outcome{std::move(outcome)}, _calls{calls} {}

    [[nodiscard]] std::string_view name() const noexcept override { return _name; }
    [[nodiscard]] std::vector<WatchedPath> watched() const override { return _watched; }
    [[nodiscard]] ReloadOutcome reload(const ChangeSet& changes) override {
        _calls->push_back(changes);
        if (_throws) {
            throw std::runtime_error{"boom"};
        }
        return _outcome;
    }

    void setThrows(const bool throws) noexcept { _throws = throws; }

  private:
    std::string _name;                  ///< What the log calls it.
    std::vector<WatchedPath> _watched;  ///< What it follows.
    ReloadOutcome _outcome;             ///< What reload() answers.
    std::vector<ChangeSet>* _calls;     ///< Where the calls are recorded.
    bool _throws{false};                ///< Whether reload() throws.
};

/// @brief Looks twice: the first look sees a change, the second one confirms it.
ChangeSet settle(FileWatcher& watcher) {
    const ChangeSet first = watcher.poll();
    EXPECT_TRUE(first.empty()) << "a change must be reported only once it is stable";
    return watcher.poll();
}

/// @brief Looks twice, and returns what the second look made.
std::vector<ReloadReport> settle(HotReloader& reloader) {
    EXPECT_TRUE(reloader.poll().empty());
    return reloader.poll();
}
}  // namespace

TEST(FileWatcher, ReportsNothingForFilesThatWereThereAtTheStart) {
    const TemporaryFolder folder;
    folder.write("a.luau", "a");
    FileWatcher watcher{folder.path(), {{.path = ".", .extensions = {}}}};

    EXPECT_TRUE(watcher.poll().empty());
    EXPECT_TRUE(watcher.poll().empty());
}

TEST(FileWatcher, ReportsANewFileOnceItIsStable) {
    const TemporaryFolder folder;
    FileWatcher watcher{folder.path(), {{.path = "scripts", .extensions = {}}}};
    folder.write("scripts/a.luau", "a");

    const ChangeSet changes = settle(watcher);

    EXPECT_EQ(changes.added, std::vector<std::string>{"scripts/a.luau"});
    EXPECT_TRUE(changes.modified.empty());
    EXPECT_TRUE(changes.removed.empty());
    EXPECT_TRUE(watcher.poll().empty()) << "reported once";
}

TEST(FileWatcher, ReportsAModifiedFile) {
    const TemporaryFolder folder;
    folder.write("a.luau", "a", 1);
    FileWatcher watcher{folder.path(), {{.path = ".", .extensions = {}}}};
    folder.write("a.luau", "a", 2);

    const ChangeSet changes = settle(watcher);

    EXPECT_EQ(changes.modified, std::vector<std::string>{"a.luau"});
    EXPECT_TRUE(changes.added.empty());
}

TEST(FileWatcher, ReportsARemovedFileAtOnce) {
    const TemporaryFolder folder;
    folder.write("a.luau", "a");
    FileWatcher watcher{folder.path(), {{.path = ".", .extensions = {}}}};
    folder.remove("a.luau");

    const ChangeSet changes = watcher.poll();

    EXPECT_EQ(changes.removed, std::vector<std::string>{"a.luau"});
}

TEST(FileWatcher, WaitsWhileTheFileKeepsChanging) {
    const TemporaryFolder folder;
    FileWatcher watcher{folder.path(), {{.path = ".", .extensions = {}}}};

    folder.write("a.luau", "a", 1);
    EXPECT_TRUE(watcher.poll().empty());
    folder.write("a.luau", "ab", 2);
    EXPECT_TRUE(watcher.poll().empty()) << "written again between two looks: still not stable";
    EXPECT_EQ(watcher.poll().added, std::vector<std::string>{"a.luau"});
}

TEST(FileWatcher, IgnoresAChangeThatWasUndoneBeforeItSettled) {
    const TemporaryFolder folder;
    folder.write("a.luau", "a", 1);
    FileWatcher watcher{folder.path(), {{.path = ".", .extensions = {}}}};

    folder.write("a.luau", "b", 2);
    EXPECT_TRUE(watcher.poll().empty());
    folder.write("a.luau", "a", 1);

    EXPECT_TRUE(watcher.poll().empty());
    EXPECT_TRUE(watcher.poll().empty());
}

TEST(FileWatcher, FollowsOnlyTheExtensionsAsked) {
    const TemporaryFolder folder;
    FileWatcher watcher{folder.path(), {{.path = ".", .extensions = {".luau"}}}};
    folder.write("a.luau", "a");
    folder.write("b.txt", "b");

    const ChangeSet changes = settle(watcher);

    EXPECT_EQ(changes.added, std::vector<std::string>{"a.luau"});
}

TEST(FileWatcher, FollowsAFileAndLeavesItsNeighboursAlone) {
    const TemporaryFolder folder;
    FileWatcher watcher{folder.path(), {{.path = "project.luau", .extensions = {}}}};
    folder.write("project.luau", "p");
    folder.write("other.luau", "o");

    const ChangeSet changes = settle(watcher);

    EXPECT_EQ(changes.added, std::vector<std::string>{"project.luau"});
}

TEST(FileWatcher, SortsWhatItReports) {
    const TemporaryFolder folder;
    FileWatcher watcher{folder.path(), {{.path = ".", .extensions = {}}}};
    folder.write("b.luau", "b");
    folder.write("a.luau", "a");
    folder.write("sub/c.luau", "c");

    const ChangeSet changes = settle(watcher);

    EXPECT_EQ(changes.added, (std::vector<std::string>{"a.luau", "b.luau", "sub/c.luau"}));
}

TEST(FileWatcher, FollowsAFolderThatDoesNotExistYet) {
    const TemporaryFolder folder;
    FileWatcher watcher{folder.path(), {{.path = "shaders", .extensions = {}}}};

    EXPECT_TRUE(watcher.poll().empty());
    folder.write("shaders/a.glsl", "a");

    EXPECT_EQ(settle(watcher).added, std::vector<std::string>{"shaders/a.glsl"});
}

TEST(HotReloader, CallsAFeatureWithItsChangedFilesAndReportsIt) {
    const TemporaryFolder folder;
    folder.write("scripts/a.luau", "a", 1);
    std::vector<ChangeSet> calls;
    std::vector<std::string> log;
    HotReloader reloader{folder.path(), [&log](const std::string_view line) { log.emplace_back(line); }};
    reloader.add(std::make_unique<RecordingReloadable>("scripts", std::vector<WatchedPath>{{.path = "scripts"}},
                                                       ReloadOutcome::applied({"system move", "component Ship"}),
                                                       &calls));
    folder.write("scripts/a.luau", "aa", 2);

    const std::vector<ReloadReport> reports = settle(reloader);

    ASSERT_EQ(calls.size(), 1U);
    EXPECT_EQ(calls.front().modified, std::vector<std::string>{"scripts/a.luau"});
    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports.front().source, "scripts");
    EXPECT_EQ(reports.front().outcome.status, ReloadOutcome::Status::kApplied);
    EXPECT_GE(reports.front().duration.count(), 0.0);
    EXPECT_FALSE(reports.front().needsRebuild());
    ASSERT_EQ(log.size(), 1U);
    EXPECT_TRUE(log.front().starts_with("[HotReload] scripts: applied in ")) << log.front();
    EXPECT_NE(log.front().find("modified scripts/a.luau"), std::string::npos) << log.front();
    EXPECT_TRUE(log.front().ends_with("system move, component Ship")) << log.front();
}

TEST(HotReloader, OnlyCallsTheFeatureWhoseFilesChanged) {
    const TemporaryFolder folder;
    folder.write("scripts/a.luau", "a", 1);
    folder.write("project.luau", "p", 1);
    std::vector<ChangeSet> scripts;
    std::vector<ChangeSet> project;
    HotReloader reloader{folder.path(), [](const std::string_view /*line*/) {}};
    reloader.add(std::make_unique<RecordingReloadable>("scripts", std::vector<WatchedPath>{{.path = "scripts"}},
                                                       ReloadOutcome::applied(), &scripts));
    reloader.add(std::make_unique<RecordingReloadable>("project", std::vector<WatchedPath>{{.path = "project.luau"}},
                                                       ReloadOutcome::applied(), &project));
    folder.write("project.luau", "pp", 2);

    const std::vector<ReloadReport> reports = settle(reloader);

    EXPECT_EQ(reloader.size(), 2U);
    EXPECT_TRUE(scripts.empty());
    EXPECT_EQ(project.size(), 1U);
    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports.front().source, "project");
}

TEST(HotReloader, ReportsAFeatureThatFailedAndKeepsGoing) {
    const TemporaryFolder folder;
    std::vector<ChangeSet> calls;
    std::vector<std::string> log;
    HotReloader reloader{folder.path(), [&log](const std::string_view line) { log.emplace_back(line); }};
    reloader.add(std::make_unique<RecordingReloadable>("scripts", std::vector<WatchedPath>{{.path = "."}},
                                                       ReloadOutcome::failed("line 3: syntax error"), &calls));
    folder.write("a.luau", "a");

    const std::vector<ReloadReport> reports = settle(reloader);

    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports.front().outcome.status, ReloadOutcome::Status::kFailed);
    ASSERT_EQ(log.size(), 1U);
    EXPECT_NE(log.front().find("the previous version is kept: line 3: syntax error"), std::string::npos) << log.front();
}

TEST(HotReloader, ReportsAFeatureThatNeedsARebuild) {
    const TemporaryFolder folder;
    std::vector<ChangeSet> calls;
    std::vector<std::string> log;
    HotReloader reloader{folder.path(), [&log](const std::string_view line) { log.emplace_back(line); }};
    reloader.add(std::make_unique<RecordingReloadable>("scripts", std::vector<WatchedPath>{{.path = "."}},
                                                       ReloadOutcome::needsRebuild("component Ship changed layout"),
                                                       &calls));
    folder.write("a.luau", "a");

    const std::vector<ReloadReport> reports = settle(reloader);

    ASSERT_EQ(reports.size(), 1U);
    EXPECT_TRUE(reports.front().needsRebuild());
    ASSERT_EQ(log.size(), 1U);
    EXPECT_NE(log.front().find("the game is rebuilt: component Ship changed layout"), std::string::npos) << log.front();
}

TEST(HotReloader, ReportsAnExceptionAsAFailureAndStillCallsTheOtherFeatures) {
    const TemporaryFolder folder;
    std::vector<ChangeSet> throwing;
    std::vector<ChangeSet> other;
    HotReloader reloader{folder.path(), [](const std::string_view /*line*/) {}};
    auto first = std::make_unique<RecordingReloadable>("first", std::vector<WatchedPath>{{.path = "."}},
                                                       ReloadOutcome::applied(), &throwing);
    first->setThrows(true);
    reloader.add(std::move(first));
    reloader.add(std::make_unique<RecordingReloadable>("second", std::vector<WatchedPath>{{.path = "."}},
                                                       ReloadOutcome::applied(), &other));
    folder.write("a.luau", "a");

    const std::vector<ReloadReport> reports = settle(reloader);

    ASSERT_EQ(reports.size(), 2U);
    EXPECT_EQ(reports[0].outcome.status, ReloadOutcome::Status::kFailed);
    EXPECT_EQ(reports[0].outcome.message, "exception: boom");
    EXPECT_EQ(reports[1].outcome.status, ReloadOutcome::Status::kApplied);
    EXPECT_EQ(other.size(), 1U);
}

TEST(HotReloader, LooksAtTheFilesOncePerInterval) {
    const TemporaryFolder folder;
    std::vector<ChangeSet> calls;
    HotReloader reloader{folder.path(), [](const std::string_view /*line*/) {}};
    reloader.add(std::make_unique<RecordingReloadable>("scripts", std::vector<WatchedPath>{{.path = "."}},
                                                       ReloadOutcome::applied(), &calls));
    folder.write("a.luau", "a");
    const HotReloader::Clock::time_point start = HotReloader::Clock::now();

    EXPECT_TRUE(reloader.update(start).empty()) << "too early for the first look";
    EXPECT_TRUE(reloader.update(start + HotReloader::kInterval).empty()) << "first look: not stable yet";
    EXPECT_TRUE(reloader.update(start + HotReloader::kInterval + std::chrono::milliseconds{1}).empty())
        << "too early for the next look";
    EXPECT_EQ(reloader.update(start + (2 * HotReloader::kInterval)).size(), 1U);
}

TEST(HotReloader, ForgetsItsFeaturesOnClear) {
    const TemporaryFolder folder;
    std::vector<ChangeSet> calls;
    HotReloader reloader{folder.path(), [](const std::string_view /*line*/) {}};
    reloader.add(std::make_unique<RecordingReloadable>("scripts", std::vector<WatchedPath>{{.path = "."}},
                                                       ReloadOutcome::applied(), &calls));
    reloader.clear();
    folder.write("a.luau", "a");

    EXPECT_EQ(reloader.size(), 0U);
    EXPECT_TRUE(settle(reloader).empty());
    EXPECT_TRUE(calls.empty());
}

TEST(ReloadReport, FormatsEveryKindOfChange) {
    const ReloadReport report{.source = "scripts",
                              .changes = {.added = {"a.luau"}, .modified = {"b.luau", "c.luau"}, .removed = {"d.luau"}},
                              .outcome = ReloadOutcome::applied({"system s"}),
                              .duration = std::chrono::duration<double, std::milli>{3.2}};

    EXPECT_EQ(
        report.format(),
        "[HotReload] scripts: applied in 3.2 ms (added a.luau; modified b.luau, c.luau; removed d.luau): system s");
}
