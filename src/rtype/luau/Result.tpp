/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Result
*/

#pragma once

namespace rtype::luau {
template <typename DestT, typename SrcT>
Result<DestT> forwardError(Result<SrcT> from) {
    return tl::unexpected{std::move(from).error()};
}
}  // namespace rtype::luau
