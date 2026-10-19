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
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/component/NetworkId.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/event/ClientDisconnected.hpp"

TEST(Headers, LibraryLinksAndIsNamed) { EXPECT_EQ(rtype::replication::libraryName(), "rtype-replication"); }
