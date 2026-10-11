/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <fstream>
#include <iostream>
#include <rtype/luau/Runtime.hpp>

int main() {
    // ReSharper disable once CppTooWideScopeInitStatement
    constexpr rtype::luau::RuntimeConfig config{
        .libs = rtype::luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = rtype::luau::RuntimeConfig::OptimizationLevel::kDisabled,
    };
    const auto runtime = rtype::luau::Runtime::create(config);

    if (!runtime) {
        std::cerr << "failed to create Luau runtime: " << runtime.error().message() << std::endl;
        return 1;
    }

    auto script = runtime->load("examples/luau/HelloWorld/script.luau");

    if (!script) {
        std::cerr << "failed to load script: " << script.error().message() << std::endl;
        return 1;
    }

    if (auto result = runtime->run(*script); !result) {
        std::cerr << "failed to run script: " << result.error().message() << std::endl;
        return 1;
    }
}
