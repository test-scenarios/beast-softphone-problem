#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <iostream>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace ssl = boost::asio::ssl;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;
using namespace std::literals;

int run() {
    auto host = "cpclientapi.softphone.com", endpoint = "/counterpath/socketapi/v1", port = "9002";
    net::io_context ioc;
    ssl::context ctx{ssl::context::tls_client};
    websocket::stream<ssl::stream<tcp::socket>> m_websocket{ioc, ctx};

    tcp::resolver resolver{ioc};

    const auto resolved = resolver.resolve(host, port);

    boost::asio::connect(m_websocket.next_layer().next_layer(), resolved.begin(), resolved.end());

    m_websocket.next_layer().handshake(ssl::stream_base::client);
    m_websocket.handshake(host, endpoint);

    std::string request = "GET/bringToFront\n"
                          "User-Agent: TestApp\n"
                          "Transaction-ID: AE26f998027\n"
                          "Content-Type: application/xml\n"
                          "Content-Length: 0";
    m_websocket.write(boost::asio::buffer(request));

    beast::flat_buffer m_resBuffer;
    m_websocket.read(m_resBuffer);

    std::cout << beast::buffers_to_string(m_resBuffer.data()) << std::endl;

    m_websocket.close(websocket::close_code::normal);

    return 0;
}

struct explainer {
    std::exception_ptr ep_;

    static void emit(std::ostream& os, std::string const& message, std::size_t level)
    {
        static thread_local std::string padding;
        padding.clear();
        if (level) padding += '\n';
        padding.append(level, ' ');
        os << padding << message;
    }

    static
    void process(std::ostream &os, std::exception &e, std::size_t level = 0)
    {
        emit(os, "exception: "s + e.what(), level);

        try
        {
            std::rethrow_if_nested(e);
        }
        catch(std::exception& child)
        {
            process(os, child, level + 1);
        }
        catch(...)
        {
            emit(os, "nonstandard exception", level + 1);
        }
    }

    static
    void process(std::ostream &os, std::exception_ptr ep, std::size_t level = 0)
    {
        try
        {
            std::rethrow_exception(ep);
        }
        catch(std::exception& child)
        {
            process(os, child, level);
        }
        catch(...)
        {
            emit(os, "nonstandard exception", level + 1);
        }
    }

    friend
    auto operator<<(std::ostream &os, explainer const &ex) -> std::ostream & {

        ex.process(os, ex.ep_);

        return os;
    }
};

auto explain(std::exception_ptr ep = std::current_exception()) -> explainer {
    return explainer{ep};
}

int main() {
    try {
        return run();
    }
    catch (...) {
        std::cerr << explain() << std::endl;
        return 127;
    }
}