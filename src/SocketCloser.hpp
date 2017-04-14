#pragma once
#include "Utils.hpp"

namespace fcpp
{

template< typename T >
struct SocketCloser
{
    SocketCloser() = delete;
    SocketCloser( const SocketCloser& ) = delete;
    SocketCloser operator=( const SocketCloser& ) = delete;
    SocketCloser( SocketCloser&& ) = delete;
    SocketCloser operator=( SocketCloser&& ) = delete;

    SocketCloser( T& obj) :
        obj_ { obj }
    {
    }

    ~SocketCloser()
    {
        try
        {
            obj_.close();
        }
        catch ( std::exception& ex )
        {
            PRINT_EX( ex );
        }
    }

private:
    T& obj_;
};

} // namespace ttf
