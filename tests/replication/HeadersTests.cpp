/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HeadersTests
*/

// Includes every public header of the module.
#include <gtest/gtest.h>

#include "rtype/replication/Export.hpp"
#include "rtype/replication/Replication.hpp"

TEST(Headers, LibraryLinksAndIsNamed) { EXPECT_EQ(rtype::replication::libraryName(), "rtype-replication"); }
