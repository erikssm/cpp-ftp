#pragma once
#include "Enums.hpp"
#include "Utils.hpp"
#include <arpa/inet.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/format.hpp>
#include <cassert>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

namespace fcpp
{

class TcpConnection;

struct Session
{
    typedef std::unique_ptr< tcp::acceptor > AcceptorPtr;

    Session() = delete;
    Session( TcpConnection& conn );
    ~Session();

    tcp::socket AcceptPassiveConn();

    bool                authenticated;
    ConnectionMode      mode;
    std::string         user;
    TcpConnection&      connection;
    AcceptorPtr         pasvPtr;
    std::vector< std::thread > workers;
};

} //namespace ttf
