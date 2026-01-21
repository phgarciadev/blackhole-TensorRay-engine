/**
 * @file blackhole.c
 * @brief Implementação de Buraco Negro (Singularidade)
 */

#include "src/simulation/data/planet.h"
#include <math.h>

static struct bhs_vec3 blackhole_surface_color(struct bhs_vec3 p)
{
	(void)p;
	/* Horizonte de Eventos: Preto absoluto */
	return (struct bhs_vec3){ 0.0, 0.0, 0.0 };
}

struct bhs_planet_desc bhs_blackhole_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Gargantua"; /* Homenagem a Interstellar */
	d.type = BHS_BLACK_HOLE;
	
	/* Essencial - Um buraco negro supermassivo para o centro da simulação?
	 * Ou estelar? Vamos usar um valor default respeitável.
	 * 10 Massas Solares.
	 */
	d.mass = 10.0 * 1.989e30; 
	
	/* Raio de Schwarzschild Rs = 2GM/c^2 
	 * G ~ 6.674e-11, c ~ 3e8
	 * Rs ~ 3000m por massa solar. -> 30km
	 * Visual radius: Event Horizon.
	 */
	d.radius = 29532.0; /* ~30km */
	
	d.density = 1e18; /* Infinito na prática, mas valor alto numérico */
	d.rotation_period = 0.001; /* Rotação extrema (Kerr) */
	d.axis_tilt = 0.0;
	d.gravity = 1e12; /* "Infinito" */
	
	d.semimajor_axis = 0.0;
	d.eccentricity = 0.0;
	d.orbital_period = 0.0;
	
	d.has_atmosphere = false;
	d.surface_pressure = 0.0;
	d.mean_temperature = 0.0;
	d.albedo = 0.0; /* Black body perfeito */
	
	d.base_color = (struct bhs_vec3){ 0.0, 0.0, 0.0 };
	d.get_surface_color = blackhole_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Gargantua (BN)", bhs_blackhole_get_desc)
