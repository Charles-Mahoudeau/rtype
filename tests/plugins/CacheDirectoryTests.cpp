/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CacheDirectoryTests
*/

#include <gtest/gtest.h>

#include <filesystem>

#include "core/system/CacheDirectory.hpp"

TEST(CacheDirectory, IsAnAbsolutePathOrNothing) {
    const std::filesystem::path directory = rtype::core::system::getCacheDirectory();
    EXPECT_TRUE(directory.empty() || directory.is_absolute());
}

#if !defined(_WIN32) && !defined(__APPLE__)
#include <cstdlib>
#include <optional>
#include <string>

namespace {
/// @brief Sets an environment variable for one test, and puts the previous value back.
class ScopedEnvironment {
  public:
    ScopedEnvironment(const char* name, const char* value) : _name{name} {
        if (const char* previous = std::getenv(name)) {
            _previous = previous;
        }
        setenv(name, value, 1);
    }
    ~ScopedEnvironment() {
        if (_previous) {
            setenv(_name, _previous->c_str(), 1);
        } else {
            unsetenv(_name);
        }
    }
    ScopedEnvironment(const ScopedEnvironment& other) = delete;
    ScopedEnvironment& operator=(const ScopedEnvironment& other) = delete;
    ScopedEnvironment(ScopedEnvironment&& other) = delete;
    ScopedEnvironment& operator=(ScopedEnvironment&& other) = delete;

  private:
    const char* _name;                     ///< The variable.
    std::optional<std::string> _previous;  ///< Its value before, if it had one.
};
}  // namespace

TEST(CacheDirectory, FollowsXdgCacheHome) {
    const ScopedEnvironment xdg{"XDG_CACHE_HOME", "/tmp/rtype-cache"};
    EXPECT_EQ(rtype::core::system::getCacheDirectory(), std::filesystem::path{"/tmp/rtype-cache"});
}

TEST(CacheDirectory, IgnoresARelativeXdgCacheHome) {
    const ScopedEnvironment xdg{"XDG_CACHE_HOME", "relative/cache"};
    const ScopedEnvironment home{"HOME", "/home/player"};
    EXPECT_EQ(rtype::core::system::getCacheDirectory(), std::filesystem::path{"/home/player/.cache"});
}
#endif

#ifdef __APPLE__
TEST(CacheDirectory, IsTheLibraryCachesOnMacOs) {
    EXPECT_EQ(rtype::core::system::getCacheDirectory().filename(), "Caches");
}
#endif
