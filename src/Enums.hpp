#pragma once

namespace fcpp
{

enum class ConnectionMode
{
    Normal, // normal connection mode
    Passive, // passive conn mode (PASV command, server listens)
    Port, // server connects to client (PORT command)
};

enum class ReplyType
{
    WaitResponse,
    NoResponse
};

} // namespace ttf
