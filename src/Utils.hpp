#pragma once

#include <iostream>
#include <string>

#define PRINT_EX(ex) ( std::cerr << "error in " << __FILE__  \
                                  << ":"  << __LINE__ \
                                  << " " << __func__ \
                                  << " :: " << ex.what() << "\n" )

#define PRINT_ERR_STR(s) ( std::cerr << "error in " << __FILE__  \
                                    << ":"  << __LINE__ \
                                    << " " << __func__ \
                                    << " :: " << ( s ) << "\n" )

namespace fcpp
{

void StringRepl( std::string& src, const std::string& find, const std::string& replaceWith );

} // namespace ttf
