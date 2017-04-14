#include "Enums.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/format.hpp>
#include <cassert>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

using boost::asio::ip::tcp;

namespace fcpp
{
Session::Session( TcpConnection& conn ) :
    authenticated {},
    mode          { ConnectionMode::Normal },
    connection    { conn }
{
}

tcp::socket Session::AcceptPassiveConn()
{
    assert( pasvPtr );

    tcp::socket socket( connection.io_service() );
    pasvPtr->accept( socket );
    pasvPtr.reset();
    return socket;
}

Session::~Session()
{
    try
    {
        std::cout << "~Session\n";

        std::for_each( workers.begin(), workers.end(),
                []( std::thread& worker ) {
                        worker.join();
                }
        );

        std::cout << "worker threads cleanup complete \n";
    }
    catch ( std::exception& ex )
    {
        PRINT_EX( ex );
    }
}
} //namespace ttf
