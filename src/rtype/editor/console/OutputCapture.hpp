/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** OutputCapture
*/

#pragma once

#include <memory>
#include <thread>

#include "ConsoleLog.hpp"
#include "editor/Export.hpp"

namespace rtype::editor::console {

/// @brief Copies everything the process writes to its standard output and error into a ConsoleLog, while still
/// writing it where it went (the terminal): the scripts' `print`, the engine's messages, the renderer's.
/// @details The two file descriptors are pointed at a pipe a thread reads, as the C library (Luau's `print`), C++
/// streams and libraries all end there. It is the process's own output, so there is one capture at a time: the editor
/// owns it, and the destructor puts the descriptors back. Standard output is made line buffered while captured, so the
/// lines arrive as they are written.
class RTYPE_EDITOR_API OutputCapture {
  public:
    /// @param log Where the lines go; it must outlive the capture.
    /// @throws std::runtime_error If the pipe cannot be made: nothing is captured then.
    explicit OutputCapture(std::shared_ptr<ConsoleLog> log);
    ~OutputCapture();
    OutputCapture(const OutputCapture& other) = delete;
    OutputCapture& operator=(const OutputCapture& other) = delete;
    OutputCapture(OutputCapture&& other) = delete;
    OutputCapture& operator=(OutputCapture&& other) = delete;

  private:
    /// @brief The thread's loop: reads the pipe until it is closed, writes to the log and to the original output.
    void read();

    std::shared_ptr<ConsoleLog> _log;  ///< Where the lines go.
    int _pipeRead{-1};                 ///< The pipe's end the thread reads.
    int _pipeWrite{-1};                ///< The pipe's end the output now goes to.
    int _savedOut{-1};                 ///< The standard output before, put back by the destructor.
    int _savedError{-1};               ///< The standard error before.
    std::thread _reader;               ///< Reads the pipe.
};

}  // namespace rtype::editor::console
