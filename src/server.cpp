#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

class SerialSession : public std::enable_shared_from_this<SerialSession> {
  websocket::stream<tcp::socket> ws_;
  asio::posix::stream_descriptor
      serial_port_; // Changed to ASIO stream descriptor
  beast::flat_buffer ws_buffer_;
  std::array<char, 1024> serial_buffer_;

public:
  explicit SerialSession(tcp::socket sock, asio::io_context &ioc,
                         const std::string &serial_device)
      : ws_(std::move(sock)), serial_port_(ioc) {
    // Open and configure serial port
    int fd = open(serial_device.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) {
      throw std::runtime_error("Failed to open serial port");
    }

    struct termios tty;
    tcgetattr(fd, &tty);
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_lflag &= ~ICANON; // Disable canonical mode
    tty.c_lflag &= ~ECHO;   // Disable echo
    tty.c_cc[VMIN] = 1;     // Read at least 1 character
    tty.c_cc[VTIME] = 0;    // No timeout
    tcsetattr(fd, TCSANOW, &tty);

    serial_port_.assign(fd);
  }

  void run() {
    ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
      if (ec)
        return;
      self->read_ws();
      self->read_serial();
    });
  }

  void read_ws() {
    ws_.async_read(
        ws_buffer_, [self = shared_from_this()](beast::error_code ec,
                                                size_t bytes_transferred) {
          if (ec)
            return;
          asio::write(self->serial_port_, asio::buffer(beast::buffers_to_string(
                                              self->ws_buffer_.data())));
          self->ws_buffer_.consume(bytes_transferred);
          self->read_ws();
        });
  }

  void read_serial() {
    serial_port_.async_read_some(
        asio::buffer(serial_buffer_),
        [self = shared_from_this()](beast::error_code ec, size_t bytes_read) {
          if (ec)
            return;
          self->ws_.async_write(asio::buffer(self->serial_buffer_, bytes_read),
                                [self](beast::error_code ec, size_t) {
                                  if (!ec)
                                    self->read_serial();
                                });
        });
  }
};

int main() {
  asio::io_context ioc;
  tcp::acceptor acceptor(ioc, {tcp::v4(), 8080});
  std::string serial_device = "/dev/ttyAMA0"; // Corrected device path

  std::function<void()> accept_connection = [&]() {
    acceptor.async_accept([&](beast::error_code ec, tcp::socket sock) {
      if (!ec) {
        std::make_shared<SerialSession>(std::move(sock), ioc, serial_device)
            ->run();
      }
      accept_connection(); // Continue accepting new connections
    });
  };

  accept_connection(); // Start accepting connections
  ioc.run();
}
