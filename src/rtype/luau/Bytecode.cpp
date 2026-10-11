/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Bytecode
*/

#include "Bytecode.hpp"

#include <cstddef>
#include <utility>

#include "CHelper.hpp"

namespace rtype::luau {
// NOLINTNEXTLINE(*-avoid-c-arrays)
Bytecode::Bytecode(CPtr<char[]> data, const std::size_t size) : _data{std::move(data)}, _size{size} {}

// NOLINTNEXTLINE(*-avoid-c-arrays)
const CPtr<char[]>& Bytecode::data() const noexcept { return _data; }

std::size_t Bytecode::size() const noexcept { return _size; }
}  // namespace rtype::luau
