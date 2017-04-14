#pragma once
#include <cstdint>

namespace fcpp
{

struct Port
{
    Port()
    {
        p1 = (uint16_t)( 128 + (rand() % 64) );
        p2 = (uint16_t)( rand() % 0xff );
    }

    uint16_t value() const
    {
        return (uint16_t)(p1 * 256 + p2);
    }

    Port( const Port& ) = delete;
    Port& operator=( const Port& ) = delete;

    Port( Port&& ) = default;
    Port& operator=( Port&& ) = default;

    uint16_t p1;
    uint16_t p2;
};

} // namespace ttf
