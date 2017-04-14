#pragma once
#include "Enums.hpp"
#include "Ftp.hpp"
#include "Session.hpp"
#include "Utils.hpp"
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <string>

using boost::asio::ip::tcp;

namespace fcpp
{

namespace
{
void PrintCommand( std::string cmd )
{
    StringRepl( cmd, "\r", "<CR>" );
    StringRepl( cmd, "\n", "<LF>" );

    std::cout << cmd << "\n";
}
}

class TcpConnection : public boost::enable_shared_from_this< TcpConnection >
{
public:
    typedef boost::shared_ptr< TcpConnection > pointer;

    TcpConnection( const TcpConnection& ) = delete;
    TcpConnection& operator=( const TcpConnection& ) = delete;

    ~TcpConnection()
    {
        std::cout << "~TcpConnection \n";
    }

    static pointer create( boost::asio::io_service& io_service )
    {
        return pointer( new TcpConnection { io_service } );
    }

    pointer get()
    {
        return shared_from_this();
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    boost::asio::io_service& io_service()
    {
        return socket_.get_io_service();
    }

    void start()
    {
        boost::asio::async_write(
                socket_,
                boost::asio::buffer( "220 welcome\r\n" ),
                boost::bind( &TcpConnection::HandleWrite, shared_from_this(),
                             boost::asio::placeholders::error )
        );
    }

    template < ReplyType type = ReplyType::WaitResponse >
    void SendReply( std::string reply )
    {
        std::lock_guard< std::mutex > { mutex_ };

        reply += "\r\n";
        PrintCommand( "  --> " + reply );

        if( ReplyType::WaitResponse == type )
        {
            boost::asio::async_write(
                socket_,
                boost::asio::buffer( reply ),
                boost::bind( &TcpConnection::HandleWrite, shared_from_this(),
                             boost::asio::placeholders::error )
            );
        }
        else
        {
            boost::asio::async_write(
                socket_,
                boost::asio::buffer( reply ),
                boost::bind( &TcpConnection::HandleWriteAndExit,
                             shared_from_this(),
                             boost::asio::placeholders::error )
            );
        }
    }

private:
    TcpConnection( boost::asio::io_service& io_service ) :
        socket_ { io_service },
        session_ { *this }
    {
    }

    void HandleRead( const boost::system::error_code& error )
    {
        if( error && boost::asio::error::eof == error )
        {
            std::cout << "client disconnected" << "\n";
            return;
        }
        else if( error && boost::asio::error::broken_pipe == error )
        {
            std::cout << "connection was interrupted" << "\n";
            return;
        }
        else if( error )
        {
            PRINT_ERR_STR( error.message() );
            PRINT_ERR_STR( "read failed on client connection, closing.." );

            return;
        }

        try
        {
            auto data = buff_.data();
            std::string cmdstring(
                    boost::asio::buffers_begin(data),
                    boost::asio::buffers_end(data)
            );
            buff_.consume( buff_.size() );

            PrintCommand( "new cmd: '" + cmdstring + "'" );

            ftp::ProcessCommand( Command { cmdstring }, session_ );

            return;
        }
        catch( std::exception& ex )
        {
            PRINT_EX( ex );
        }

        try
        {
            SendReply( "500 error processing last command\r\n" );
        }
        catch ( std::exception& ex )
        {
            PRINT_EX( ex );
            PRINT_ERR_STR( "write failed on client connection, closing.." );
        }
    }

    void HandleWrite( const boost::system::error_code& error )
    {
        if( error )
        {
            PRINT_ERR_STR(
                ( boost::format( "%s" ) % error.message() ).str()
            );
            return;
        }

        boost::asio::async_read_until(
            socket_, buff_, std::string { "\r\n" },
            boost::bind( &TcpConnection::HandleRead, shared_from_this(),
                         boost::asio::placeholders::error )
        );
    }

    void HandleWriteAndExit( const boost::system::error_code& error )
    {
        if( error )
        {
            PRINT_ERR_STR(
                ( boost::format( "%s" ) % error.message() ).str()
            );
            return;
        }
    }

    boost::asio::streambuf buff_;
    tcp::socket     socket_;
    Session         session_;
    std::mutex      mutex_;
};

class TcpServer
{
public:
    TcpServer( boost::asio::io_service& io_service, std::string address,
               uint16_t port ) :
        acceptor_ {
            io_service,
            tcp::endpoint(
                boost::asio::ip::address::from_string( address ), port )
        }
    {
        StartAccept();
    }

private:
    void StartAccept()
    {
        TcpConnection::pointer conn =
                TcpConnection::create( acceptor_.get_io_service() );

        acceptor_.async_accept(
                conn->socket(),
                boost::bind(
                    &TcpServer::HandleAccept,
                    this,
                    conn,
                    boost::asio::placeholders::error
                )
        );
    }

    void HandleAccept( TcpConnection::pointer conn,
                       const boost::system::error_code& error )
    {
        if ( !error )
        {
            conn->start();
        }

        StartAccept();
    }

    tcp::acceptor acceptor_;
};

} // namespace ttf
