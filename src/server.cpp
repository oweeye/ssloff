#include <asio.hpp>
#include <asio/ssl.hpp>

#include <functional>
#include <iostream>

using asio::ip::tcp;

class session : public std::enable_shared_from_this<session> {
 public:
  session(tcp::socket out_socket_,
          asio::ssl::stream<tcp::socket> in_socket,
          tcp::resolver::results_type &endpoints)
      : out_socket_(std::move(out_socket_)),
        in_socket_(std::move(in_socket)),
        endpoints_(endpoints) {}

  void start() { do_handshake(); }

 private:
  void do_connect() {
    auto self(shared_from_this());
    out_socket_.async_connect(*endpoints_,
                              [this, self](const std::error_code &error) {
                                if (!error) {
                                  do_in_read();
                                  do_out_read();
                                }
                              });
  }
  void do_handshake() {
    auto self(shared_from_this());
    in_socket_.async_handshake(asio::ssl::stream_base::server,
                               [this, self](const std::error_code &error) {
                                 if (!error) {
                                   do_connect();
                                 }
                               });
  }

  void do_in_read() {
    auto self(shared_from_this());
    in_socket_.async_read_some(
        asio::buffer(in_out_data_),
        [this, self](const std::error_code &ec, std::size_t length) {
          if (!ec) {
            do_out_write(length);
          }
        });
  }

  void do_in_write(std::size_t length) {
    auto self(shared_from_this());
    asio::async_write(
        in_socket_, asio::buffer(out_in_data_, length),
        [this, self](const std::error_code &ec, std::size_t /*length*/) {
          if (!ec) {
            // in_socket_.shutdown();
            do_out_read();
          }
        });
  }

  void do_out_write(std::size_t length) {
    auto self(shared_from_this());
    asio::async_write(
        out_socket_, asio::buffer(in_out_data_, length),
        [this, self](const std::error_code &ec, std::size_t /*length*/) {
          if (!ec) {
            do_in_read();
          }
        });
  }

  void do_out_read() {
    auto self(shared_from_this());
    out_socket_.async_read_some(
        asio::buffer(out_in_data_),
        [this, self](const std::error_code &ec, std::size_t length) {
          if (!ec) {
            do_in_write(length);
          }
        });
  }

  tcp::socket out_socket_;
  asio::ssl::stream<tcp::socket> in_socket_;
  tcp::resolver::results_type endpoints_;
  char in_out_data_[1024];
  char out_in_data_[1024];
};

class server {
 public:
  server(asio::io_context &io_context, unsigned short port,
         tcp::resolver::results_type endpoints)
      : io_context_(io_context),
        acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        context_(asio::ssl::context::sslv23),
        endpoints_(endpoints) {
    context_.set_options(asio::ssl::context::default_workarounds |
                         asio::ssl::context::no_sslv2 |
                         asio::ssl::context::single_dh_use);
    context_.set_password_callback(std::bind(&server::get_password, this));
    context_.use_certificate_chain_file("server.pem");
    context_.use_private_key_file("server.pem", asio::ssl::context::pem);
    context_.use_tmp_dh_file("dhparam.pem");
    std::cerr << "Listen on " << port << std::endl;
    do_accept();
  }

 private:
  std::string get_password() const { return ""; }

  void do_accept() {
    acceptor_.async_accept(
        [this](const std::error_code &error, tcp::socket socket) {
          if (!error) {
            std::make_shared<session>(
                tcp::socket(io_context_),
                asio::ssl::stream<tcp::socket>(std::move(socket), context_),
                endpoints_)
                ->start();
          }

          do_accept();
        });
  }

  asio::io_context &io_context_;
  tcp::acceptor acceptor_;
  asio::ssl::context context_;
  tcp::resolver::results_type endpoints_;
};

int main(int argc, char *argv[]) {
  try {
    if (argc != 4) {
      std::cerr << "Usage: ssloff <ssl_port> <out_host> <out_port>\n";
      return 1;
    }

    asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[2], argv[3]);
    for (auto iter = endpoints; iter != tcp::resolver::iterator();) {
      tcp::endpoint endpoint = *iter++;
      std::cerr << endpoint << std::endl;
    }
    server s(io_context, atoi(argv[1]), endpoints);
    io_context.run();
  } catch (std::exception &e) {
    std::cerr << "Ops, exception: " << e.what() << "\n";
  }

  return 0;
}
