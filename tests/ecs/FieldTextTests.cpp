/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldTextTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"
#include "rtype/ecs/world/World.hpp"

using rtype::ecs::FieldInfo;
using rtype::ecs::FieldType;
using rtype::ecs::TypeTraits;

namespace {
/// @brief A component with a text of any length.
struct Note {
    std::string text;
};
RTYPE_ECS_COMPONENT(Note, "test.text.Note", text);
}  // namespace

TEST(FieldText, ReflectsAStdStringAsAText) {
    const auto& fields = TypeTraits<Note>::kFields;
    ASSERT_EQ(fields.size(), 1U);
    EXPECT_EQ(fields.front().type, FieldType::kText);
    EXPECT_NE(fields.front().list, nullptr);
}

TEST(FieldText, GivesTheBytesOfTheText) {
    const Note note{.text = "hello"};
    const FieldInfo field = TypeTraits<Note>::makeTypeInfo().fields.front();
    const std::span<const std::byte> bytes = field.list->elements(&note.text);
    ASSERT_EQ(bytes.size(), 5U);
    EXPECT_EQ(bytes.front(), std::byte{'h'});
}

TEST(FieldText, ReplacesTheTextWhateverItsLength) {
    Note note{.text = "short"};
    const FieldInfo field = TypeTraits<Note>::makeTypeInfo().fields.front();
    const std::string longer(1000, 'x');
    field.list->assign(&note.text, std::as_bytes(std::span{longer}));
    EXPECT_EQ(note.text, longer);
    field.list->assign(&note.text, {});
    EXPECT_TRUE(note.text.empty());
}

TEST(FieldText, IsAValidField) {
    rtype::ecs::World world;
    const rtype::ecs::Entity entity = world.spawn();
    world.getComponents().add(entity, Note{.text = std::string(5000, 'y')});
    EXPECT_EQ(world.getComponents().get<Note>(entity)->text.size(), 5000U);
}
