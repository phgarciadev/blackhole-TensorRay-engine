/**
 * @file moon.c
 * @brief Implementação da Lua (Placeholder)
 */

#include "src/simulation/data/planet.h"

struct bhs_planet_desc bhs_moon_get_desc(void)
{
	/* Lua (The Moon) - J2000 Orbital Elements (Approx) */
    struct bhs_planet_desc d = {0};
    d.name = "Lua";
    d.type = BHS_PLANET_TERRESTRIAL;
    
    /* Physical Properties */
    d.mass = 7.342e22;      /* kg */
    d.radius = 1.7371e6;    /* m */
    d.density = 3344.0;     /* kg/m^3 */
    d.rotation_period = 27.321661 * 24 * 3600; /* Locked */
    d.axis_tilt = 1.5424 * (3.14159/180.0); /* rad */
    d.gravity = 1.62;       /* m/s^2 */
    
    /* Orbital Elements (Relative to Earth Mean Equator/Ecliptic J2000 approx) */
    /* Note: Ideally Moon orbit is complex, this is simplified Keplerian */
    d.semimajor_axis = 0.3844e9; /* m (384,400 km) */
    d.eccentricity = 0.0549;
    d.inclination = 5.145;       /* deg */
    
    /* These vary rapidly, using J2000 mean values */
    d.long_asc_node = 125.08;    /* deg */
    d.long_perihelion = 318.15;  /* deg */
    d.mean_longitude = 135.27;   /* deg */
    
    /* Visual */
    d.base_color = (struct bhs_vec3){0.6f, 0.6f, 0.6f}; /* Grey */
    d.has_atmosphere = false;
    
    return d;
}
