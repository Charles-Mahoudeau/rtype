/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <iostream>
#include <rtype/luau/Runtime.hpp>

int main() {
    // ReSharper disable once CppTooWideScopeInitStatement
    const auto rt = rtype::luau::Runtime::create();

    if (!rt) {
        std::cerr << "Failed to create Luau runtime" << std::endl;
        return 1;
    }
    std::cout << "Hello World" << std::endl;
}
