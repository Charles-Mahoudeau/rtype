/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <cstddef>
#include <format>
#include <iostream>
#include <rtype/luau/CallContext.hpp>
#include <rtype/luau/Exceptions.hpp>
#include <rtype/luau/Function.hpp>
#include <rtype/luau/FunctionRef.hpp>
#include <rtype/luau/Runtime.hpp>
#include <rtype/luau/Value.hpp>
#include <rtype/luau/ValuePack.hpp>
#include <string>
#include <utility>

using namespace rtype::luau;

namespace {
/// @brief Describes a Value as its Luau type followed by its content, e.g. `number 42`.
std::string describe(const Value& value) {
    if (value.is<Value::Nil>()) {
        return "nil";
    }
    if (const auto* boolean = value.tryAs<bool>()) {
        return std::format("boolean {}", *boolean);
    }
    if (const auto* number = value.tryAs<double>()) {
        return std::format("number {}", *number);
    }
    if (const auto* vector = value.tryAs<Value::Vector>()) {
        return std::format("vector {}, {}, {}", vector->x, vector->y, vector->z);
    }
    if (const auto* string = value.tryAs<std::string>()) {
        return std::format("string {}", *string);
    }
    if (value.is<FunctionRef>()) {
        return "function";
    }
    return "table";
}
}  // namespace

int main() {
    const auto runtime = Runtime::create({
        .libs = RuntimeConfig::Libs::kStandard,
        .optimizationLevel = RuntimeConfig::OptimizationLevel::kDisabled,
    });
    if (!runtime) {
        std::cerr << "failed to create Luau runtime: " << runtime.error().message() << std::endl;
        return 1;
    }
    auto script = runtime->load("examples/luau/AdvancedFunctionBinding/script.luau");
    if (!script) {
        std::cerr << "failed to load script: " << script.error().message() << std::endl;
        return 1;
    }

    // Luau -> C++
    // `create` makes a native closure, which is stored in the global `cpp_reverse` so the script calls it like any
    // Luau function. The script calls it at its top level, so the handler is set before the script runs: until
    // then, calling it raises a Lua error. `cppReverse` must outlive every call from Luau: once it is destroyed,
    // calling the closure raises a Lua error instead of reaching a dangling object.
    Function cppReverse = Function::create(runtime->state(), "cpp_reverse");
    cppReverse.setHandler([](const CallContext& context) -> ValuePack {
        // `args()` gives the arguments of the call, in order, as Values. An argument of a type that cannot be read
        // yet (e.g. a table) raises a Lua error before the handler runs, so every argument here is readable.
        const auto args = context.args();
        // Throwing an exception of the module raises a Lua error, prefixed with the function name, which the
        // script can catch with `pcall`.
        if (args.empty()) {
            throw exceptions::InvalidOperation{"expects at least one argument"};
        }
        for (std::size_t i = 0; i < args.size(); ++i) {
            std::cout << std::format("C++ received argument #{}: {}", i + 1, describe(args[i])) << std::endl;
        }
        // The values of the returned pack are returned to Luau, in order: a handler can return any number of them.
        ValuePack reversed{};
        for (auto it = args.rbegin(); it != args.rend(); ++it) {
            reversed.add(*it);
        }
        return reversed;
    });
    if (const auto result = runtime->globals().set("cpp_reverse", Value{cppReverse.ref()}); !result) {
        std::cerr << "failed to register cpp_reverse: " << result.error().message() << std::endl;
        return 1;
    }

    // Run the script: it calls `cpp_reverse`, then defines the global function `luau_reverse`.
    if (auto result = runtime->run(*script); !result) {
        std::cerr << "failed to run script: " << result.error().message() << std::endl;
        return 1;
    }

    // C++ -> Luau
    // `luau_reverse` only exists once the script has run, so it is looked up afterwards. `globals().get` returns nil
    // if the global is not set, hence the check on the type. `fromRef` gives the Function a handle on the Luau
    // function, which keeps it alive for as long as `luauReverse` exists.
    const auto luauReverseValue = runtime->globals().get("luau_reverse");
    const FunctionRef* luauReverseRef = luauReverseValue ? luauReverseValue->tryAs<FunctionRef>() : nullptr;
    if (luauReverseRef == nullptr) {
        std::cerr << "the script did not define luau_reverse" << std::endl;
        return 1;
    }
    const Function luauReverse = Function::fromRef(*luauReverseRef, "luau_reverse");

    // The arguments are gathered in a ValuePack: `add` appends them in order, and `call` pushes each one as the
    // matching Luau type. Every type C++ can push is passed: nil, boolean, number, vector and string.
    ValuePack args{};
    args.add({Value::Nil{}});
    args.add({true});
    args.add({42.0});
    args.add({Value::Vector{1.0F, 2.0F, 3.0F}});
    args.add({std::string{"text"}});

    // `call` runs the function in protected mode, and returns every value it returned in a ValuePack, in the order
    // of its `return` statement. It fails if the function raises an error, or if it returns a value that cannot be
    // read yet (e.g. a thread).
    const auto results = luauReverse.call(args);
    if (!results) {
        std::cerr << "failed to call luau_reverse: " << results.error().message() << std::endl;
        return 1;
    }
    // A Luau function may return any number of values, so check the count before reading them. `as<T>()` reads a
    // value as the given type and throws `std::bad_variant_access` on a mismatch; `tryAs<T>()` returns a null
    // pointer instead, to check the type without throwing (see `describe`).
    const auto values = results->values();
    for (std::size_t i = 0; i < values.size(); ++i) {
        std::cout << std::format("C++ got back value #{}: {}", i + 1, describe(values[i])) << std::endl;
    }
    if (values.size() == args.size()) {
        std::cout << "C++ read the first value as a string: " << values[0].as<std::string>() << std::endl;
    }

    // An error raised by the Luau function comes back as a failed Result, with the Luau error message.
    if (const auto failed = luauReverse.call(); !failed) {
        std::cout << "C++ caught: " << failed.error().message() << std::endl;
    }
}
