/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <GLFW/glfw3.h>

#include "platform/Window.hpp"

int main() {
    // Create a window with the specified width, height, and title
    const rtype::vulkan::platform::Window window(800, 600, "Vulkan Window Example");

    // Poll for window events until the user closes the window
    while (glfwWindowShouldClose(window.getHandle()) == GLFW_FALSE) {
        glfwPollEvents();
    }
    return 0;
}
