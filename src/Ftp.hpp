#pragma once

#include "Command.hpp"
#include "Session.hpp"

namespace fcpp
{
namespace ftp
{
void ProcessCommand( const Command& cmd, Session& session);
void OutputResponse( Session& );

void user( const Command&, Session& );
void pass( const Command&, Session& );
void pwd( const Command&, Session& );
void cwd( const Command&, Session& );
void mkd( const Command&, Session& );
void rmd( const Command&, Session& );
void pasv( const Command&, Session& );
void list( const Command&, Session& );
void retr( const Command&, Session& );
void stor( const Command&, Session& );
void dele( const Command&, Session& );
void size( const Command&, Session& );
void quit( const Command&, Session& );
void type( const Command&, Session& );
void abor( const Command&, Session& );
void noop( const Command&, Session& );

} // namespace ftp
} // namespace ttf
