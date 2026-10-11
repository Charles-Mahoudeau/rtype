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
    func1.setHandler([](const rtype::luau::CallContext&) -> rtype::luau::CallResult {
        std::cout << "Hello from C++!" << std::endl;
        return {};
    });

    // Run the script: it calls `hello_from_cpp`, then defines the global functions `hello_from_luau` and
    // `hello_with_args`.
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

    // Call a Luau function with arguments from C++.
    // The arguments are gathered in a `CallContext`: `addArg` appends them in order, and `call` pushes them onto
    // the Lua stack before the call, so the first one added becomes the first parameter of the Luau function.
    rtype::luau::Ref func4Ref = runtime->global("hello_with_args");
    assert(!func4Ref.isNil());
    rtype::luau::Function func4 = rtype::luau::Function::fromRef(std::move(func4Ref), "hello_with_args");
    rtype::luau::CallContext func4Context{};
    func4Context.addArg({"C++"});        // from
    func4Context.addArg({"arguments"});  // name
    auto func4Result = func4.call(func4Context);
    assert(func4Result);
}
