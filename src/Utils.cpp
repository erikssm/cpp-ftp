#include <string>
#include "Utils.hpp"

namespace fcpp
{

void StringRepl( std::string& src, const std::string& find, const std::string& replaceWith )
{
    using namespace std;

    for( string::size_type i = 0;
            ( i = src.find( find, i ) ) != string::npos; )
    {
        src.replace( i, find.length(), replaceWith );
        i += replaceWith.length();
    }
}

} // namespace ttf
