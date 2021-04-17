#define ASIO_STANDALONE
#include <iostream>
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

std::vector<char> buffer(1 * 1024);
void grab_some_data(asio::ip::tcp::socket& socket) {
    socket.async_read_some(asio::buffer(buffer.data(), buffer.size()),[&](std::error_code ec, std::size_t length) {
        if (!ec) {
            std::cout << "\n\nRead " << length << " bytes\n\n";

            for (int i = 0; i < length; i++)
                std::cout << buffer[i];

            grab_some_data(socket);
        }
    });
}

int main() {
    asio::error_code ec;
    asio::io_context context;

    asio::io_context::work idle_work(context);
    std::thread thread = std::thread([&]() { context.run(); });

    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("93.184.216.34", ec), 80);
    asio::ip::tcp::socket socket(context);
    socket.connect(endpoint, ec);

    if (!ec)
        std::cout << "Connected!" << std::endl;
    else
        std::cout << "Failed to connect\n" << ec.message() << std::endl;

    if (socket.is_open()) {
        grab_some_data(socket);

        std::string request =
                "GET /index.html HTTP/1.1\r\n"
                "Host:example.com\r\n"
                "Connection: close\r\n\r\n";

        socket.write_some(asio::buffer(request.data(), request.size()), ec);

//        using namespace std::chrono_literals;
//        std::this_thread::sleep_for(1s);
        context.stop();
        if (thread.joinable())
            thread.join();
    }

    return 0;
}
