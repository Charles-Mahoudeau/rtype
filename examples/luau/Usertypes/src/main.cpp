/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <algorithm>
#include <format>
#include <iostream>
#include <map>
#include <rtype/luau/Exceptions.hpp>
#include <rtype/luau/Runtime.hpp>
#include <rtype/luau/Userdata.hpp>
#include <rtype/luau/Usertype.hpp>
#include <rtype/luau/Value.hpp>
#include <rtype/luau/ValuePack.hpp>
#include <string>
#include <string_view>
#include <utility>

using namespace rtype::luau;

// The C++ types Luau uses. A usertype wraps any type that can be destroyed: Luau stores the object by value, and
// destroys it when the script no longer uses it or the runtime is closed.
struct Player {
    std::string name;
    double health;
};

struct Settings {
    std::map<std::string, double, std::less<>> values;
};

int main() {
    auto runtime = Runtime::create({
        .libs = RuntimeConfig::Libs::kStandard,
        .optimizationLevel = RuntimeConfig::OptimizationLevel::kDisabled,
    });
    if (!runtime) {
        std::cerr << "failed to create Luau runtime: " << runtime.error().message() << std::endl;
        return 1;
    }
    auto script = runtime->load("examples/luau/Usertypes/script.luau");
    if (!script) {
        std::cerr << "failed to load script: " << script.error().message() << std::endl;
        return 1;
    }

    // `usertype` registers `Player` once per runtime, and gives a handle to describe it. The runtime keeps the
    // registration and the handlers until it is closed. Every handler takes the same kind of parameters as
    // `Function::bind` (`double` for numbers, `std::string` for strings...), and the object first when it works on one.
    // A call that does not fit raises a Lua error before the handler runs.
    auto player = runtime->usertype<Player>("Player");
    player
        // `Player.new(name, health)` in Luau. It returns the object, which Luau then owns.
        .constructor(
            [](const std::string& name, const double health) { return Player{.name = name, .health = health}; })
        // A getter without a setter makes a read-only member.
        .getter("name", [](const Player& self) { return self.name; })
        .getter("health", [](const Player& self) { return self.health; })
        .setter("health", [](Player& self, const double health) { self.health = health; })
        // A method returns nothing (`void`), a `ValuePack`, or a single value.
        .method("damage",
                [](Player& self, const double amount) {
                    self.health = std::max(self.health - amount, 0.0);
                    return self.health;
                })
        // `==` in Luau. Without `eq`, two userdata are equal only if they are the same object.
        .eq([](const Player& lhs, const Player& rhs) { return lhs.name == rhs.name; })
        // `tostring` in Luau.
        .tostring([](const Player& self) { return std::format("Player({}, {} hp)", self.name, self.health); });

    // `index` and `newIndex` handle the members that have no getter nor method, for the fields only known at runtime
    // (this is what a component made of scripts will need). A handler reports an unknown name by throwing.
    runtime->usertype<Settings>("Settings")
        .constructor([] { return Settings{}; })
        .index([](Settings& self, const std::string_view key) -> Value {
            const auto setting = self.values.find(key);
            if (setting == self.values.end()) {
                throw exceptions::RuntimeError{std::format("no setting '{}'", key)};
            }
            return Value{setting->second};
        })
        .newIndex([](Settings& self, const std::string_view key, const Value& value) {
            const auto* number = value.tryAs<double>();
            if (number == nullptr) {
                throw exceptions::RuntimeError{"settings are numbers"};
            }
            self.values.insert_or_assign(std::string{key}, *number);
        });

    // C++ -> Luau
    // `create` makes a userdata holding an object, to give to scripts. Here it becomes the global `boss`, set before
    // the script runs since the script reads it at its top level.
    auto boss = player.create(Player{.name = "Boss", .health = 500});
    if (!boss) {
        std::cerr << "failed to create the boss: " << boss.error().message() << std::endl;
        return 1;
    }
    if (const auto result = runtime->globals().set("boss", Value{*boss}); !result) {
        std::cerr << "failed to register boss: " << result.error().message() << std::endl;
        return 1;
    }

    if (auto result = runtime->run(*script); !result) {
        std::cerr << "failed to run script: " << result.error().message() << std::endl;
        return 1;
    }

    // Calling a script function with a userdata: a `Userdata` is a `Value` like a table or a function.
    auto heal = runtime->globals().function("heal");
    if (!heal) {
        std::cerr << "failed to get heal: " << heal.error().message() << std::endl;
        return 1;
    }
    ValuePack args{};
    args.add({*boss});
    args.add({50.0});
    if (const auto result = heal->call(args); !result) {
        std::cerr << "failed to call heal: " << result.error().message() << std::endl;
        return 1;
    }

    // `tryAs` gives the C++ object back, as long as a handle on the userdata is alive (here, `boss`), or `nullptr`
    // for an object of another type. The script changed it: the boss was healed from 500 to 550.
    if (const auto* object = boss->tryAs<Player>()) {
        std::cout << "C++: " << object->name << " has " << object->health << " health" << std::endl;
    }
}
