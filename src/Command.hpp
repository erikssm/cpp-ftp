#pragma once
#include "Utils.hpp"
#include <stdexcept>
#include <string>

namespace fcpp
{

struct Command
{
    Command( const Command& ) = default;
    Command( Command&& ) = default;

    Command() = default;
    Command( std::string src )
    {
        if( src.find( "\r\n" ) ==  std::string::npos )
        {
            throw std::invalid_argument
            {
                "invalid command format '" + src + "'"
            };
        }

        StringRepl( src, "\r\n", "" );

        auto pos = src.find( " " );
        if( pos != std::string::npos )
        {
            command = src.substr( 0, pos );
            arg     = src.substr( pos + 1 );
        }
        else
        {
            command = src;
        }
    }

    std::string command;
    std::string arg;
};

} // namespace ttf
