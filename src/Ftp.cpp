#include "Command.hpp"
#include "Directory.hpp"
#include "Ftp.hpp"
#include "Port.hpp"
#include "Session.hpp"
#include "Server.hpp"
#include "SocketCloser.hpp"
#include "Utils.hpp"
#include "WorkingDirChanger.hpp"
#include <algorithm>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <memory>
#include <thread>


using boost::asio::detail::thread;
using boost::asio::ip::tcp;
using std::ofstream;

namespace fcpp
{
namespace ftp
{

namespace
{
const std::string unauthorized      = "530 please log in using USER and PASS";
const std::string notImplemented    = "502 command is not implemented";

const std::string usernames[] { "ftp", "anonymous", "anon" };

constexpr size_t buffSize = 256; // for file transfers

enum class Auth
{
    None,
    MustLogIn
};

struct FtpCommand
{
    typedef void (*function_t)(const fcpp::Command&, fcpp::Session&);

    FtpCommand( const char* cmd, function_t fn, Auth auth ) :
        name     { cmd },
        authType { auth },
        func     { fn }
    {
    }
    ~FtpCommand() {}

    void invoke( const fcpp::Command& cmd, fcpp::Session& session )
    {
        func( cmd, session );
    }

    FtpCommand() = delete;
    FtpCommand& operator=( const FtpCommand& ) = delete;

public:
    std::string name;
    Auth        authType;

private:
    function_t  func;
};

FtpCommand CommanList[] {
    FtpCommand { "USER",    &fcpp::ftp::user,    Auth::None       },
    FtpCommand { "PASS",    &fcpp::ftp::pass,    Auth::None       },
    FtpCommand { "ABOR",    &fcpp::ftp::abor,    Auth::MustLogIn  },
    FtpCommand { "CWD",     &fcpp::ftp::cwd,     Auth::MustLogIn  },
    FtpCommand { "DELE",    &fcpp::ftp::dele,    Auth::MustLogIn  },
    FtpCommand { "LIST",    &fcpp::ftp::list,    Auth::MustLogIn  },
    FtpCommand { "MKD",     &fcpp::ftp::mkd,     Auth::MustLogIn  },
    FtpCommand { "NOOP",    &fcpp::ftp::noop,    Auth::MustLogIn  },
    FtpCommand { "PASV",    &fcpp::ftp::pasv,    Auth::MustLogIn  },
    FtpCommand { "PWD",     &fcpp::ftp::pwd,     Auth::MustLogIn  },
    FtpCommand { "QUIT",    &fcpp::ftp::quit,    Auth::MustLogIn  },
    FtpCommand { "RETR",    &fcpp::ftp::retr,    Auth::MustLogIn  },
    FtpCommand { "RMD",     &fcpp::ftp::rmd,     Auth::MustLogIn  },
    FtpCommand { "SIZE",    &fcpp::ftp::size,    Auth::MustLogIn  },
    FtpCommand { "STOR",    &fcpp::ftp::stor,    Auth::MustLogIn  },
    FtpCommand { "TYPE",    &fcpp::ftp::type,    Auth::MustLogIn  },
};

} // namespace

void ProcessCommand( const Command& cmd, Session& session)
{
    for( auto handler : CommanList )
    {
        if( cmd.command != handler.name )
        {
            continue;
        }

        if( handler.authType == Auth::MustLogIn && ! session.authenticated )
        {
            session.connection.SendReply( "530  not logged in" );
        }

        handler.invoke( cmd, session );
        return;
    }

    session.connection.SendReply( "500 unknown command" );
}

void user( const Command& cmd, Session& session )
{
    auto end = std::end( usernames );
    auto it = std::find( std::begin( usernames ), end , cmd.arg );
    if( it != end )
    {
        session.user = cmd.arg;
        session.connection.SendReply( "331 user name " + cmd.arg );
    }
    else
    {
        session.connection.SendReply( "530 invalid username" );
    }
}

void pass( const Command&, Session& session )
{
    if( ! session.user.empty() )
    {
        session.authenticated = true;
        session.connection.SendReply( "230 logged in successfully" );
    }
    else
    {
        session.connection.SendReply(
                "500 Invalid username or password" );
    }
}

void pasv( const Command&, Session& session )
{
    Port port;

    session.pasvPtr.reset(
        new tcp::acceptor (
            session.connection.socket().get_io_service(),
            tcp::endpoint( tcp::v4(), port.value() )
        )
    );

    auto ip = session.connection.socket().remote_endpoint()
                                            .address().to_v4().to_bytes();
    session.connection.SendReply(
        ( boost::format( "227 entering passive mode (%d,%d,%d,%d,%d,%d)" )
                        % (int)ip[0] % (int)ip[1] % (int)ip[2] % (int)ip[3]
                        % port.p1 % port.p2
        ).str()
    );

    session.mode = ConnectionMode::Passive;
}

void list( const Command& cmd, Session& session)
{
    try
    {
        if( session.mode == ConnectionMode::Passive )
        {
            WorkingDirChanger dirChanger{ cmd.arg };
            Directory dir { dirChanger.GetCWD() };
            dir.List( session );
        }
        else if( session.mode == ConnectionMode::Port )
        {
            session.connection.SendReply( notImplemented );
        }
        else
        {
            session.connection.SendReply( "425 use PASV or PORT first" );
        }
    }
    catch ( std::exception& ex )
    {
        PRINT_EX( ex );
        session.connection.SendReply( "550 failed to list directory" );
    }

    session.mode = ConnectionMode::Normal;
}

void quit( const Command&, Session& session )
{
    session.connection.SendReply< ReplyType::NoResponse >( "221 bye" );
}

void pwd( const Command&, Session& session )
{
    session.connection.SendReply(
            ( boost::format( "257 \"%s\"" )
                             % WorkingDirChanger::GetCWD()
            ).str()
    );
}

void cwd( const Command& cmd, Session& session )
{
    try
    {
        WorkingDirChanger::CWD( cmd.arg );
        session.connection.SendReply( "250 directory changed" );
        return;
    }
    catch ( std::exception& ex )
    {
        PRINT_EX( ex );
    }
    session.connection.SendReply( "550 failed to change directory" );
}

void mkd( const Command& cmd, Session& session)
{
    if( ::mkdir( cmd.arg.c_str(), S_IRWXU ) == 0 )
    {
        if( cmd.arg[0] == '/' ) // absolute path
        {
            session.connection.SendReply(
                    ( boost::format( "257 \"%s\" directory created" )
                                   % cmd.arg ).str()
            );
        }
        else
        {
            session.connection.SendReply(
                    ( boost::format( "257 \"%s/%s\" directory created" )
                                   % WorkingDirChanger::GetCWD()
                                   % cmd.arg ).str()
            );
        }
    }
    else
    {
        session.connection.SendReply(
                "550 failed to create directory, please check permissions" );
    }
}

void retr( const Command& cmd, Session& session )
{
    if( session.mode != ConnectionMode::Passive )
    {
        session.connection.SendReply( "550 please use PASV mode" );
        return;
    }

    auto worker = [ cmd ]( TcpConnection::pointer connection,
                      tcp::socket pasvSocket )
    {
        try
        {
            SocketCloser< tcp::socket > sockCloser{ pasvSocket };

            std::vector< char > buf ( buffSize );
            boost::system::error_code error;

            std::ifstream file( cmd.arg.c_str() );
            if( !file )
            {
                throw std::runtime_error { "could not open file" };
            }

            connection->SendReply(
                    "150 opening BINARY mode data connection" );

            auto bytesLeft = boost::filesystem::file_size( cmd.arg.c_str() );
            while( bytesLeft > 0 && ! error  )
            {
                auto n = bytesLeft > buffSize ? buffSize : bytesLeft;
                file.read( buf.data(), n );
                bytesLeft -= n;

                boost::asio::write( pasvSocket,
                                    boost::asio::buffer( buf, n ),
                                    error );
            }

            if( error )
            {
                throw std::runtime_error {
                    ( boost::format( "error transfering file: %s" )
                                     % error.message()

                    ).str()
                };
            }

            connection->SendReply(
                    "226 file downloaded successfully" );
        }
        catch( std::exception& ex )
        {
            PRINT_EX( ex );
            connection->SendReply( "550 failed to download file" );
        }

    };

    session.workers.emplace_back( worker, session.connection.get(),
            session.AcceptPassiveConn() );
}

void stor( const Command& cmd, Session& session)
{
    if( ! ( session.mode == ConnectionMode::Passive) )
    {
        session.connection.SendReply(
                "550 please use PASV instead of PORT" );
        return;
    }

    auto worker = [ cmd ]( TcpConnection::pointer connection,
                           tcp::socket pasvSocket )
    {
        //TODO: implement ABOR
        try
        {
            SocketCloser< tcp::socket > sockCloser{ pasvSocket };

            std::ofstream file( cmd.arg.c_str() );
            if( !file )
            {
                throw std::runtime_error {
                    ( boost::format( "error creating file: %s" )
                                     % cmd.arg.c_str()

                    ).str()
                };
            }

            connection->SendReply< ReplyType::NoResponse >(
                            "125 data connection open, staring transfer" );

            boost::asio::streambuf buffer;
            boost::system::error_code error;
            while( ! error )
            {
                auto transfered = boost::asio::read(
                                    pasvSocket,
                                    buffer,
                                    boost::asio::transfer_exactly( buffSize ),
                                    error );
                if( ! transfered )
                {
                    break;
                }

                file.write( boost::asio::buffer_cast< const char * >(
                                                buffer.data() ), transfered );
                buffer.consume( transfered );
            }
            if( error && error != boost::asio::error::eof )
            {
                throw std::runtime_error {
                    ( boost::format( "error transfering file: %s" )
                                     % error.message()

                    ).str()
                };
            }
            connection->SendReply( "226 file sent successfully" );

            // TODO: for linux systems use copy_file_range for in-kernel
            // file copy = better performance
            // http://man7.org/linux/man-pages/man2/copy_file_range.2.html
        }
        catch ( std::exception& ex )
        {
            PRINT_EX( ex );
            connection->SendReply( "550 failed to transfer target file" );
        }
    };

    session.workers.emplace_back( worker, session.connection.get(),
            session.AcceptPassiveConn() );
}

void abor( const Command&, Session& session )
{
    session.connection.SendReply( "226 closing data connection" );
    // TODO: implement ABOR
}

void type( const Command& cmd, Session& session )
{
    if( cmd.arg[0] == 'I' )
    {
        session.connection.SendReply( "200 switched to binary mode" );
    }
    else if( cmd.arg[0] == 'A' )
    {
         session.connection.SendReply( "200 switching to ASCII mode" );
    }
    else
    {
        session.connection.SendReply( "504 unknown TYPE parameter" );
    }

}

void dele( const Command& cmd, Session& session )
{
    try
    {
        boost::system::error_code ec;
        if( boost::filesystem::remove( cmd.arg, ec ) )
        {
            session.connection.SendReply( "250 file was removed" );
            return;
        }

        PRINT_ERR_STR( ec.message() );
    }
    catch ( std::exception& ex )
    {
        PRINT_EX( ex );
    }

    session.connection.SendReply( "550 error file directory" );
}

void rmd( const Command& cmd, Session& session )
{
    try
    {
        boost::system::error_code ec;
        if( boost::filesystem::remove( cmd.arg, ec ) )
        {
            session.connection.SendReply( "250 directory was removed" );
            return;
        }

        PRINT_ERR_STR( ec.message() );
    }
    catch ( std::exception& ex )
    {
        PRINT_EX( ex );
    }

    session.connection.SendReply( "550 error deleting directory" );
}

void size( const Command& cmd, Session& session )
{
    try
    {
        session.connection.SendReply( "213 " +
            std::to_string( boost::filesystem::file_size( cmd.arg.c_str() ) )
        );
    }
    catch ( std::exception& ex )
    {
        session.connection.SendReply( "550 failed to get file size" );
    }
}

void noop( const Command&, Session& session )
{
    session.connection.SendReply( "200 noop" );
}

} // namespace ftp
} // namespace ttf
