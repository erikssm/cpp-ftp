#pragma once
#include "Ftp.hpp"
#include "Server.hpp"
#include <dirent.h>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/range/iterator_range.hpp>
#include <cstring>
#include <sys/stat.h>
#include <ctime>

namespace
{
std::string permissions2string( int perm )
{
    std::string permissions;

    for( int i = 6; i >= 0; i -= 3 )
    {
        auto current = ( ( perm & ALLPERMS ) >> i ) & 0x7;

        auto read   = (current >> 2) & 0x1;
        auto write  = (current >> 1) & 0x1;
        auto exec   = (current >> 0) & 0x1;

        permissions += (
            boost::format( "%c%c%c" )
                           % ( read  ? 'r' : '-' )
                           % ( write ? 'w' : '-' )
                           % ( exec  ? 'x' : '-' )
        ).str();
    }

    return permissions;
}
}

namespace fcpp
{

struct Directory
{
    Directory( const std::string& dir )
    {
        ptr = ::opendir( dir.c_str() );
        if( ! ptr )
        {
            throw std::runtime_error {
                (
                    boost::format( "failed to open directory '%s'" )
                                   % dir
                ).str()
            };
        }
    }

    ~Directory()
    {
        ::closedir( ptr );
    }

    void List( Session& session )
    {
        tcp::socket socket = session.AcceptPassiveConn();

        session.connection.SendReply< ReplyType::NoResponse >(
                                        "150 sending directory contents.." );

        char timebuf[80];
        struct stat statbuf;

        // TODO : replace with boost
        struct dirent *entry;
        while( ( entry = ::readdir( ptr ) ) )
        {
            if( -1 == ::stat( entry->d_name, &statbuf ) )
            {
                PRINT_ERR_STR(
                    ( boost::format( "error reading file stats for '%s'" )
                                     % entry->d_name
                    ).str()
                );
            }
            else
            {
                auto rawtime = statbuf.st_mtime;
                auto time = gmtime( &rawtime );
                strftime( timebuf, sizeof( timebuf ), "%b %d %H:%M", time );

                auto perms = permissions2string( (statbuf.st_mode & ALLPERMS) );

                std::string str = (
                        boost::format( "%c%s %5ld %4d %4d %8ld %s %s\r\n")
                                        % ( (entry->d_type == DT_DIR) ?
                                                'd' : '-' )
                                        % perms
                                        % statbuf.st_nlink
                                        % statbuf.st_uid
                                        % statbuf.st_gid
                                        % statbuf.st_size
                                        % timebuf
                                        % entry->d_name
                                   ).str();
                boost::system::error_code ignored_error;
                boost::asio::write( socket,
                                    boost::asio::buffer( str, str.length() ),
                                    ignored_error );
            }
        } // while

        session.connection.SendReply( "226 directory contents sent" );
        session.mode = ConnectionMode::Normal;
    }

private:
    DIR* ptr;
};

}
