/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LibraryEntryTests
*/

#include <gtest/gtest.h>

#include "core/library/LibraryEntry.hpp"

using rtype::core::library::LibraryRegistrar;

TEST(LibraryRegistrar, FindsWhatALibraryProvides) {
    LibraryRegistrar registrar;
    int service = 4;
    EXPECT_EQ(registrar.find<int>("test.service"), nullptr);
    registrar.provide("test.service", service);
    ASSERT_EQ(registrar.find<int>("test.service"), &service);
    EXPECT_EQ(registrar.find<int>("test.other"), nullptr);
}
