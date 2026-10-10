/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <cassert>
#include <fstream>
#include <iostream>
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
    // `cppFunc` must stay alive as long as Luau may call the closure: once it is destroyed, the closure is
    // unbound and calling it raises a Lua error instead of reaching a dangling object.
    rtype::luau::Function cppFunc = rtype::luau::Function::create(runtime->state(), "hello_from_cpp");
    cppFunc.setHandler([](const rtype::luau::CallContext&) { std::cout << "Hello from C++!" << std::endl; });

    // Run the script: it calls `hello_from_cpp`, then defines the global function `hello_from_luau`.
    if (auto result = runtime->run(*script); !result) {
        std::cerr << "failed to run script: " << result.error().message() << std::endl;
        return 1;
    }

    // Call a Luau function from C++.
    // `hello_from_luau` only exists once the script has run, so it is looked up afterwards. `global` returns a
    // reference to nil if the global is not set, hence the check. `fromRef` takes over the reference, which keeps
    // the Luau function alive for as long as `luauFunc` exists. `call` runs it in protected mode and throws
    // `exceptions::RuntimeError` if it raises an error.
    rtype::luau::Ref luauFuncRef = runtime->global("hello_from_luau");
    assert(!luauFuncRef.isNil());
    rtype::luau::Function luauFunc = rtype::luau::Function::fromRef(std::move(luauFuncRef), "hello_from_luau");
    luauFunc.call({});

    // Call the C++ function from C++, through Luau.
    // The native closure made by `create` is an ordinary Luau value: wrapping it with `fromRef` gives a handle
    // that calls it exactly like a Luau function, without knowing it is implemented in C++. The call goes through
    // the Lua state and lands in the handler of `cppFunc`.
    // This handle does not own the binding: it keeps the closure alive, but the handler still belongs to
    // `cppFunc`. If `cppFunc` is destroyed, calling `selfFunc` throws `exceptions::RuntimeError`.
    rtype::luau::Ref selfFuncRef = runtime->global("hello_from_cpp");
    assert(!selfFuncRef.isNil());
    rtype::luau::Function selfFunc = rtype::luau::Function::fromRef(std::move(selfFuncRef), "hello_from_cpp");
    selfFunc.call({});
}
