/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LocalHttpServer
*/

#pragma once

#include <asio/buffer.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/address.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/streambuf.hpp>
#include <asio/write.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <functional>
#include <istream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace rtype::test::plugins {

/// @brief A plain HTTP server on 127.0.0.1, on a free port, for the http plugin's tests: no test reaches the internet.
/// It serves one connection at a time on its own thread, answers with what its handler returns, then closes.
class LocalHttpServer {
  public:
    /// @brief A request the server received.
    struct Received {
        std::string method;   ///< "GET", "POST"...
        std::string path;     ///< "/echo".
        std::string headers;  ///< Every header line, as sent, each ending with "\r\n".
        std::string body;     ///< The body, read up to its Content-Length.
    };

    /// @brief What the server answers.
    struct Reply {
        int status{200};                     ///< The status.
        std::string body;                    ///< The body.
        std::chrono::milliseconds delay{0};  ///< Waits this long before answering.
    };

    using Handler = std::function<Reply(const Received&)>;

    explicit LocalHttpServer(Handler handler)
        : _handler{std::move(handler)},
          _acceptor{_context, asio::ip::tcp::endpoint{asio::ip::make_address("127.0.0.1"), 0}} {
        _port = _acceptor.local_endpoint().port();
        _thread = std::thread{[this] { serve(); }};
    }

    ~LocalHttpServer() {
        _stopping = true;
        // A connection to itself wakes the blocking accept, which then sees it is stopping.
        try {
            asio::io_context context;
            asio::ip::tcp::socket wake{context};
            wake.connect(asio::ip::tcp::endpoint{asio::ip::make_address("127.0.0.1"), _port});
        } catch (const std::exception&) {  // NOLINT(bugprone-empty-catch): already stopped
        }
        _thread.join();
    }

    LocalHttpServer(const LocalHttpServer& other) = delete;
    LocalHttpServer& operator=(const LocalHttpServer& other) = delete;
    LocalHttpServer(LocalHttpServer&& other) = delete;
    LocalHttpServer& operator=(LocalHttpServer&& other) = delete;

    /// @return `http://127.0.0.1:<port><path>`.
    [[nodiscard]] std::string url(const std::string& path) const {
        return std::format("http://127.0.0.1:{}{}", _port, path);
    }

    /// @return The requests received so far, in order.
    [[nodiscard]] std::vector<Received> getReceived() const {
        const std::scoped_lock lock{_mutex};
        return _received;
    }

  private:
    void serve() {
        while (!_stopping) {
            asio::ip::tcp::socket socket{_context};
            asio::error_code error;
            _acceptor.accept(socket, error);
            if (error || _stopping) {
                continue;
            }
            try {
                answer(socket);
            } catch (const std::exception&) {  // NOLINT(bugprone-empty-catch): the client hung up; serve the next
            }
        }
    }

    void answer(asio::ip::tcp::socket& socket) {
        asio::streambuf buffer;
        const std::size_t headEnd = asio::read_until(socket, buffer, "\r\n\r\n");
        std::string head{asio::buffers_begin(buffer.data()),
                         asio::buffers_begin(buffer.data()) + static_cast<std::ptrdiff_t>(headEnd)};
        buffer.consume(headEnd);
        Received received;
        const std::size_t firstLine = head.find("\r\n");
        const std::string requestLine = head.substr(0, firstLine);
        const std::size_t space = requestLine.find(' ');
        received.method = requestLine.substr(0, space);
        received.path = requestLine.substr(space + 1, requestLine.find(' ', space + 1) - space - 1);
        received.headers = head.substr(firstLine + 2);
        std::size_t length = 0;
        for (const std::string name : {"Content-Length: ", "content-length: "}) {
            if (const std::size_t at = received.headers.find(name); at != std::string::npos) {
                length = std::stoul(received.headers.substr(at + name.size()));
            }
        }
        if (buffer.size() < length) {
            asio::read(socket, buffer, asio::transfer_exactly(length - buffer.size()));
        }
        received.body.assign(asio::buffers_begin(buffer.data()),
                             asio::buffers_begin(buffer.data()) + static_cast<std::ptrdiff_t>(length));
        {
            const std::scoped_lock lock{_mutex};
            _received.push_back(received);
        }
        const Reply reply = _handler(received);
        if (reply.delay.count() > 0) {
            std::this_thread::sleep_for(reply.delay);
        }
        const std::string response =
            std::format("HTTP/1.1 {} Test\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}", reply.status,
                        reply.body.size(), reply.body);
        asio::write(socket, asio::buffer(response));
    }

    Handler _handler;
    asio::io_context _context;
    asio::ip::tcp::acceptor _acceptor;
    std::uint16_t _port{0};
    std::thread _thread;
    std::atomic<bool> _stopping{false};
    mutable std::mutex _mutex;
    std::vector<Received> _received;
};

}  // namespace rtype::test::plugins
