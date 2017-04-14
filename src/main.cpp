#include "Server.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>

int main( int, const char* [] )
{
    try
    {
        srand( static_cast< unsigned int >( time( NULL ) ) );

        constexpr uint16_t port = 8021;

        std::cout << "Starting ftp server on port " << port << "\n";

        boost::asio::io_service io_service;
        fcpp::TcpServer server { io_service, "127.0.0.1", port };
        io_service.run();

        std::cout << "done, exiting FTP server\n";
    }
    catch ( std::exception& e )
    {
        PRINT_EX( e );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
