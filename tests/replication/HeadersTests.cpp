/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HeadersTests
*/

// Includes every public header of the module.
#include <gtest/gtest.h>

#include "rtype/replication/Export.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/Replication.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/component/NetworkId.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/event/ClientDisconnected.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/input/InputBuffer.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/input/InputMessage.hpp"
#include "rtype/replication/manifest/Manifest.hpp"
#include "rtype/replication/manifest/ManifestMessages.hpp"
#include "rtype/replication/manifest/ManifestReceiver.hpp"
#include "rtype/replication/serialization/EntityMapper.hpp"
#include "rtype/replication/serialization/FieldSerializer.hpp"

TEST(Headers, LibraryLinksAndIsNamed) { EXPECT_EQ(rtype::replication::libraryName(), "rtype-replication"); }
