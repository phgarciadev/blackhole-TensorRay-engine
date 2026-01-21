/**
 * @file moon.c
 * @brief Implementação da Lua (Placeholder)
 */

#include "src/simulation/data/planet.h"

struct bhs_planet_desc bhs_moon_get_desc(void)
{
    /* Placeholder - A Lua ainda não está no sistema de registro */
    struct bhs_planet_desc d = {0};
    d.name = "Lua";
    d.mass = 7.34e22; 
    d.radius = 1.737e6;
    return d;
}
