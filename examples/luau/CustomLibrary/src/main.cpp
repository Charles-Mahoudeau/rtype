/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <algorithm>
#include <cmath>
#include <iostream>
#include <rtype/luau/Function.hpp>
#include <rtype/luau/LibraryBuilder.hpp>
#include <rtype/luau/Runtime.hpp>
#include <rtype/luau/Table.hpp>
#include <rtype/luau/Value.hpp>
#include <rtype/luau/ValuePack.hpp>

using namespace rtype::luau;

int main() {
    auto runtime = Runtime::create({
        .libs = RuntimeConfig::Libs::kStandard,
        .optimizationLevel = RuntimeConfig::OptimizationLevel::kDisabled,
    });
    if (!runtime) {
        std::cerr << "failed to create Luau runtime: " << runtime.error().message() << std::endl;
        return 1;
    }
    auto script = runtime->load("examples/luau/CustomLibrary/script.luau");
    if (!script) {
        std::cerr << "failed to load script: " << script.error().message() << std::endl;
        return 1;
    }

    // A library is a table of functions. The builder creates the table, and `function` adds a native function to it
    // under the given name: it is bound like any other Function (see the FunctionBinding example). The Functions
    // must stay alive as long as Luau may call them.
    LibraryBuilder builder{*runtime};

    builder.setName("mathx");

    builder.function("clamp").bind([](const double value, const double min, const double max) {
        ValuePack results{};
        results.add({std::clamp(value, min, max)});
        return results;
    });

    builder.function("divide").bind([](const double dividend, const double divisor) {
        ValuePack results{};
        results.add({std::floor(dividend / divisor)});
        results.add({std::fmod(dividend, divisor)});
        return results;
    });

    const Library library = builder.build();

    // `build` hands over the finished table. Setting it as a global makes it reachable from every script, as `math`
    // or `string` are.
    if (auto result = runtime->globals().registerLibrary(library); !result) {
        std::cerr << "failed to register mathx: " << result.error().message() << std::endl;
        return 1;
    }

    if (auto result = runtime->run(*script); !result) {
        std::cerr << "failed to run script: " << result.error().message() << std::endl;
        return 1;
    }
}
