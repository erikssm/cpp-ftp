#pragma once

#include "Session.hpp"
#include <boost/format.hpp>
#include <cerrno>
#include <climits>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include "Utils.hpp"

namespace fcpp
{

struct WorkingDirChanger
{
    WorkingDirChanger( const std::string& dir )
    {
        origdir_[ 0 ] = 0;

        if( dir.empty() || dir[0] == '-' )
        {
            return;
        }

        if( ! ::getcwd( origdir_, PATH_MAX ) )
        {
            throw std::runtime_error {
                ( boost::format( "getcwd failed (%s)" )
                                 % ::strerror( errno ) ).str()
            };
        }

        CWD( dir );
    }

    ~WorkingDirChanger()
    {
        if( 0 == origdir_[ 0 ] )
        {
            return;
        }

        CWD( origdir_ );
    }

    static void CWD( const std::string& dir )
    {
        if( -1 == ::chdir( dir.c_str() ) )
        {
            throw std::runtime_error {
                ( boost::format( "chdir failed (%s)" )
                                 % ::strerror( errno ) ).str()
            };
        }
    }

    static std::string GetCWD()
    {
        char dir[ PATH_MAX + 1];
        if( ! ::getcwd( dir, PATH_MAX ) )
        {
            throw std::runtime_error {
                ( boost::format( "getcwd failed (%s)" )
                                 % ::strerror( errno ) ).str()
            };
        }

        return dir;
    }

    WorkingDirChanger() = delete;
    WorkingDirChanger( const WorkingDirChanger& ) = delete;
    WorkingDirChanger& operator=( const WorkingDirChanger& ) = delete;
    WorkingDirChanger( WorkingDirChanger&& ) = delete;
    WorkingDirChanger& operator=( WorkingDirChanger&& ) = delete;

private:
    char origdir_[ PATH_MAX + 1 ];
};

} // namespace ttf
