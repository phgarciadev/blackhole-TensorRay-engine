/**
 * @file blackhole.c
 * @brief Implementação de Buraco Negro (Singularidade)
 */

#include "src/simulation/data/planet.h"
#include "src/simulation/data/blackhole.h"
#include <math.h>

static struct bhs_vec3 blackhole_surface_color(struct bhs_vec3 p)
{
	(void)p;
	return (struct bhs_vec3){ 0.0, 0.0, 0.0 };
}

/* 
 * Lógica dedicada para Gargantua 
 */
static const struct bhs_blackhole_desc GARGANTUA = {
    .name = "Gargantua",
    .mass = 1.989e31, /* 10 Suns */
    .spin = 0.9,
    .charge = 0.0,
    /* Rs = 2GM/c^2 = 2 * 6.67e-11 * 1.989e31 / 9e16 ~= 29500 */
    .event_horizon_r = 29532.0, 
    .accretion_disk_mass = 1.989e29, /* 0.1 Sun */
    .base_color = { 0.0, 0.0, 0.0 },
    .get_surface_color = blackhole_surface_color
};

/* Adapter para UI/Registry Legacy */
struct bhs_planet_desc bhs_blackhole_get_desc(void)
{
	struct bhs_planet_desc d = {0};
    const struct bhs_blackhole_desc *bh = &GARGANTUA;
	
	d.name = bh->name;
	d.type = BHS_BLACK_HOLE;
	
	d.mass = bh->mass;
	d.radius = bh->event_horizon_r;
	
	d.density = 1e18; 
	d.rotation_period = 0.001; 
	d.axis_tilt = 0.0;
	d.gravity = 1e12; 
	
	d.semimajor_axis = 0.0;
	d.eccentricity = 0.0;
	d.orbital_period = 0.0;
	
	d.has_atmosphere = false;
	d.surface_pressure = 0.0;
	d.mean_temperature = 0.0;
	d.albedo = 0.0; 
	
	d.base_color = bh->base_color;
	d.get_surface_color = bh->get_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Gargantua (BN)", bhs_blackhole_get_desc)
