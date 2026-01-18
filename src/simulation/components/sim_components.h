#ifndef SIMULATION_COMPONENTS_H
#define SIMULATION_COMPONENTS_H

#include "engine/components/components.h"

/* IDs for Simulation Components (starting after Engine components) */
enum {
    BHS_COMP_CELESTIAL = BHS_COMP_COUNT,
    BHS_COMP_ORBIT_DESC,
    SIM_COMP_COUNT
};

/* Celestial Types (Star, Planet, BlackHole) */
typedef enum {
    BHS_CELESTIAL_PLANET,
    BHS_CELESTIAL_MOON,
    BHS_CELESTIAL_STAR,
    BHS_CELESTIAL_BLACKHOLE,
    BHS_CELESTIAL_ASTEROID
} bhs_celestial_type;

/* Game-logic properties */
typedef struct {
    double density;
    double radius;
    bool has_atmosphere;
    char composition[64];
    struct bhs_vec3 color; // Visual color
} bhs_planet_component;

typedef struct {
    double luminosity;
    double temp_effective;
    char spectral_type[8];
    struct bhs_vec3 color; // Visual color for star
} bhs_star_component;

/**
 * struct bhs_celestial_component
 * Game-logic properties for celestial bodies.
 */
typedef struct {
    bhs_celestial_type type;
    char name[64];
    
    // Inline factories or references could be used, 
    // but for now keeping it flat might be easier or use union if exclusive
    union {
        bhs_planet_component planet;
        bhs_star_component star;
    } data;
} bhs_celestial_component;

#endif
