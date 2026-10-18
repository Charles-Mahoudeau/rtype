/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HeadersTests
*/

// Includes every public header of the module. network-tests has no Asio include path, so this file fails to
// build if one of them includes Asio.
#include <gtest/gtest.h>

#include "rtype/network/Export.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/RandomSource.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/protocol/Sequence.hpp"
#include "rtype/network/protocol/SequenceBuffer.hpp"

TEST(Headers, PublicHeadersCompileWithoutAsio) { SUCCEED(); }
