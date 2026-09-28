/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <iostream>

#include "platform/Window.hpp"

#include <GLFW/glfw3.h>

int main(int argc, char **argv)
{
    // Create a window with the specified width, height, and title
    rtype::vulkan::platform::Window window(800, 600, "Vulkan Window Example");

    while (true)
    {
        // Poll for window events
        glfwPollEvents();

        // Check if the window should close
        if (glfwWindowShouldClose(window.getHandle()))
        {
            break;
        }
    }
    return 0;
}
