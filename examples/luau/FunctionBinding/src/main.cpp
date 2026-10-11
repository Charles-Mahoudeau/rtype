/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <cmath>
#include <iostream>
#include <rtype/luau/CallContext.hpp>
#include <rtype/luau/Exceptions.hpp>
#include <rtype/luau/Function.hpp>
#include <rtype/luau/Runtime.hpp>
#include <rtype/luau/ValuePack.hpp>
#include <utility>

using namespace rtype::luau;

int main() {
    const auto runtime = Runtime::create({
        .libs = RuntimeConfig::Libs::kStandard,
        .optimizationLevel = RuntimeConfig::OptimizationLevel::kDisabled,
    });
    if (!runtime) {
        std::cerr << "failed to create Luau runtime: " << runtime.error().message() << std::endl;
        return 1;
    }
    auto script = runtime->load("examples/luau/FunctionBinding/script.luau");
    if (!script) {
        std::cerr << "failed to load script: " << script.error().message() << std::endl;
        return 1;
    }

    // Luau -> C++
    // `create` stores a native closure in the global `cpp_divide`, and every call from Luau runs the handler. It is
    // set before the script runs, since the script calls `cpp_divide` at its top level. `cppDivide` must stay alive
    // as long as Luau may call it.
    Function cppDivide = Function::create(runtime->state(), "cpp_divide");
    cppDivide.setHandler([](const CallContext& context) -> ValuePack {
        // The handler receives the arguments in order, and checks there are as many as it expects: throwing an
        // exception of the module raises a Lua error in the script.
        const auto args = context.args();
        if (args.size() != 2) {
            throw exceptions::InvalidOperation{"expects 2 arguments"};
        }
        const double dividend = args[0].as<double>();
        const double divisor = args[1].as<double>();

        // The values added to the returned pack are returned to Luau, in order.
        ValuePack results{};
        results.add({std::floor(dividend / divisor)});
        results.add({std::fmod(dividend, divisor)});
        return results;
    });

    if (auto result = runtime->run(*script); !result) {
        std::cerr << "failed to run script: " << result.error().message() << std::endl;
        return 1;
    }

    // C++ -> Luau
    // `apply_damage` is defined by the script, so it is looked up once the script has run.
    Ref applyDamageRef = runtime->global("apply_damage");
    if (applyDamageRef.isNil()) {
        std::cerr << "the script did not define apply_damage" << std::endl;
        return 1;
    }
    const Function applyDamage = Function::fromRef(std::move(applyDamageRef), "apply_damage");

    // The arguments are added in order: the first one becomes the first parameter of the Luau function.
    ValuePack args{};
    args.add({100.0});  // health
    args.add({30.0});   // damage
    args.add({true});   // shielded

    // `call` returns the values of the `return` statement in order, or an error if the function failed.
    const auto results = applyDamage.call(args);
    if (!results) {
        std::cerr << "failed to call apply_damage: " << results.error().message() << std::endl;
        return 1;
    }
    const auto values = results->values();
    if (values.size() != 2) {
        std::cerr << "apply_damage returned " << values.size() << " values instead of 2" << std::endl;
        return 1;
    }
    const double remaining = values[0].as<double>();
    const bool alive = values[1].as<bool>();
    std::cout << "C++: remaining health " << remaining << ", alive: " << std::boolalpha << alive << std::endl;
}
