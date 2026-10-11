/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <cassert>
#include <fstream>
#include <iostream>
#include <rtype/luau/CallContext.hpp>
#include <rtype/luau/Function.hpp>
#include <rtype/luau/Runtime.hpp>
#include <rtype/luau/ValuePack.hpp>
#include <string>

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

    auto script = runtime->load("examples/luau/FunctionBinding/script.luau");

    if (!script) {
        std::cerr << "failed to load script: " << script.error().message() << std::endl;
        return 1;
    }

    // Expose a C++ function to Luau.
    // `create` makes a native closure and stores it in the global `hello_from_cpp`, so scripts can call it like
    // any Luau function. Each call from Luau is forwarded to the handler set with `setHandler`; until a handler
    // is set, calling it raises a Lua error. The handler must therefore be set before the script runs, since the
    // script calls `hello_from_cpp` at its top level.
    // `func1` must stay alive as long as Luau may call the closure: once it is destroyed, the closure is
    // unbound and calling it raises a Lua error instead of reaching a dangling object.
    rtype::luau::Function func1 = rtype::luau::Function::create(runtime->state(), "hello_from_cpp");
    func1.setHandler([](const rtype::luau::CallContext&) -> rtype::luau::ValuePack {
        std::cout << "Hello from C++!" << std::endl;
        return {};
    });

    // Run the script: it calls `hello_from_cpp`, then defines the global functions `hello_from_luau` and
    // `apply_damage`.
    if (auto result = runtime->run(*script); !result) {
        std::cerr << "failed to run script: " << result.error().message() << std::endl;
        return 1;
    }

    // Call a Luau function from C++.
    // `hello_from_luau` only exists once the script has run, so it is looked up afterwards. `global` returns a
    // reference to nil if the global is not set, hence the check. `fromRef` takes over the reference, which keeps
    // the Luau function alive for as long as `func2` exists. `call` runs it in protected mode and returns a
    // `Result` holding an `Error` if it raises an error.
    rtype::luau::Ref func2Ref = runtime->global("hello_from_luau");
    assert(!func2Ref.isNil());
    rtype::luau::Function func2 = rtype::luau::Function::fromRef(std::move(func2Ref), "hello_from_luau");
    auto func2Result = func2.call({});
    assert(func2Result);

    // Call the C++ function from C++, through Luau.
    // The native closure made by `create` is an ordinary Luau value: wrapping it with `fromRef` gives a handle
    // that calls it exactly like a Luau function, without knowing it is implemented in C++. The call goes through
    // the Lua state and lands in the handler of `func1`.
    // This handle does not own the binding: it keeps the closure alive, but the handler still belongs to
    // `func1`. If `func1` is destroyed, calling `func3` throws `exceptions::RuntimeError`.
    rtype::luau::Ref func3Ref = runtime->global("hello_from_cpp");
    assert(!func3Ref.isNil());
    rtype::luau::Function func3 = rtype::luau::Function::fromRef(std::move(func3Ref), "hello_from_cpp");
    auto func3Result = func3.call();
    assert(func3Result);

    // Call a Luau function with arguments from C++, and read the values it returns.
    // The arguments are gathered in a `ValuePack`: `add` appends them in order, and `call` pushes them onto
    // the Lua stack before the call, so the first one added becomes the first parameter of the Luau function.
    // Each `Value` is pushed as the matching Luau type: `std::string` as a string, `double` as a number and
    // `bool` as a boolean.
    rtype::luau::Ref func4Ref = runtime->global("apply_damage");
    assert(!func4Ref.isNil());
    rtype::luau::Function func4 = rtype::luau::Function::fromRef(std::move(func4Ref), "apply_damage");
    rtype::luau::ValuePack func4Args{};
    func4Args.add({std::string{"Player"}});  // name
    func4Args.add({100.0});                  // health
    func4Args.add({30.0});                   // damage
    func4Args.add({true});                   // shielded
    auto func4Result = func4.call(func4Args);
    // The call fails if the Luau function raises an error, or if it returns a value that cannot be read yet
    // (e.g. a table or a function).
    if (!func4Result) {
        std::cerr << "failed to call apply_damage: " << func4Result.error().message() << std::endl;
        return 1;
    }

    // Every value returned by the Luau function is collected in the returned `ValuePack`, in the order of the `return`
    // statement. Luau functions may return any number of values, so check the count before reading them.
    // `as<T>()` reads a value as the given type and throws `std::bad_variant_access` on a mismatch; `tryAs<T>()`
    // returns a null pointer instead, to check the type without throwing.
    const auto values = func4Result->values();
    assert(values.size() == 3);
    const double remaining = values[0].as<double>();          // remaining health
    const bool alive = values[1].as<bool>();                  // whether the entity is still alive
    const std::string& report = values[2].as<std::string>();  // report of the hit
    std::cout << report << std::endl;
    std::cout << "remaining health: " << remaining << ", alive: " << std::boolalpha << alive << std::endl;
}
