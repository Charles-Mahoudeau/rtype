/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SharedLibrary
*/

#pragma once

#include <filesystem>
#include <string_view>

#include "rtype/engine/Export.hpp"

namespace rtype::engine::plugin {
/// @brief A shared library opened at runtime (`.so`, `.dylib`, `.dll`), closed with this object.
///
/// @details Code from a library must stay loaded for as long as anything built from it is alive. Its owner keeps it
/// longer than what it built, but some of it can outlive every owner: an exception a plugin threw, caught in `main`
/// once the Engine that loaded the plugin is destroyed, still calls its library's code (its `what()`). So a library is
/// never unloaded before the process ends (`RTLD_NODELETE`, a pinned module on Windows): closing it gives its handle
/// back, its code stays. On Windows, a library already in the process under the same file name is shared, as the
/// executable may have loaded a copy of it from another folder.
class RTYPE_ENGINE_API SharedLibrary {
  public:
    /// @param path The library's file.
    /// @throws exceptions::RuntimeError If it cannot be opened, with the system's reason.
    explicit SharedLibrary(const std::filesystem::path& path);
    ~SharedLibrary();
    SharedLibrary(const SharedLibrary& other) = delete;
    SharedLibrary& operator=(const SharedLibrary& other) = delete;
    SharedLibrary(SharedLibrary&& other) noexcept;
    SharedLibrary& operator=(SharedLibrary&& other) noexcept;

    /// @return The address of the exported symbol @p name, or nullptr if the library has none.
    [[nodiscard]] void* findSymbol(std::string_view name) const;

    /// @return The extension of a shared library on this system, with its dot.
    [[nodiscard]] static std::string_view getExtension() noexcept;

  private:
    /// @brief Closes the library, if any.
    void close() noexcept;

    void* _handle{nullptr};  ///< What the system knows the library by.
    bool _owned{false};      ///< False when it was already in the process: it is not closed here.
};
}  // namespace rtype::engine::plugin
