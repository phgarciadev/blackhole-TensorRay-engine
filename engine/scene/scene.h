#ifndef BHS_ENGINE_SCENE_NEW_H
#define BHS_ENGINE_SCENE_NEW_H

/* Typedef must be visible */
typedef struct bhs_scene_impl *bhs_scene_t;

#include "math/bhs_math.h"
#include "engine/ecs/ecs.h"
#include "engine/engine.h"
#include "engine/physics/spacetime/spacetime.h"

/* ============================================================================
 * VIEW DTOs (Legacy Adapter for UI/Renderer)
 * ============================================================================
 */

enum bhs_body_type {
	BHS_BODY_PLANET,
	BHS_BODY_MOON,
	BHS_BODY_STAR,
	BHS_BODY_BLACKHOLE,
	BHS_BODY_ASTEROID,
};

enum bhs_matter_state {
	BHS_STATE_SOLID,
	BHS_STATE_LIQUID,
	BHS_STATE_GAS,
	BHS_STATE_PLASMA
};

enum bhs_shape_type {
	BHS_SHAPE_SPHERE,
	BHS_SHAPE_ELLIPSOID,
	BHS_SHAPE_IRREGULAR
};

enum bhs_star_stage {
	BHS_STAR_MAIN_SEQUENCE,
	BHS_STAR_GIANT,
	BHS_STAR_WHITE_DWARF,
	BHS_STAR_NEUTRON
};

struct bhs_planet_data {
	double density;
	double axis_tilt; 
	double albedo;
	bool has_atmosphere;
	double surface_pressure;
	double atmosphere_mass;
	char composition[64];
	double temperature;
	double heat_capacity;
	double energy_flux;
	int physical_state; // Simplified enum
	bool has_magnetic_field;
};

struct bhs_star_data {
	double luminosity;
	double temp_effective;
	double age;
	double density;
	double hydrogen_frac;
	double helium_frac;
	double metals_frac;
	int stage; // Simplified
	double metallicity;
	char spectral_type[8];
};

struct bhs_blackhole_data {
	double spin_factor;
	double event_horizon_r;
	double ergososphere_r; // Typo fix? Keep legacy name.
	double accretion_disk_mass;
	double accretion_rate;
};

struct bhs_body_state {
	struct bhs_vec3 pos;
	struct bhs_vec3 vel;
	struct bhs_vec3 acc;
	struct bhs_vec3 rot_axis;
	double rot_speed;
	double moment_inertia;
	double mass;
	double radius;
	int shape;
};

struct bhs_body {
	struct bhs_body_state state;
	enum bhs_body_type type;
	union {
		struct bhs_planet_data planet;
		struct bhs_star_data star;
		struct bhs_blackhole_data bh;
	} prop;
	struct bhs_vec3 color;
	bool is_fixed;
	bool is_alive;
	char name[32];
};

/* API */
bhs_scene_t bhs_scene_create(void);
void bhs_scene_destroy(bhs_scene_t scene);
void bhs_scene_init_default(bhs_scene_t scene);
void bhs_scene_update(bhs_scene_t scene, double dt);
/* Accessors */
bhs_spacetime_t bhs_scene_get_spacetime(bhs_scene_t scene);
bhs_world_handle bhs_scene_get_world(bhs_scene_t scene);
const struct bhs_body *bhs_scene_get_bodies(bhs_scene_t scene, int *count);
bool bhs_scene_add_body_struct(bhs_scene_t scene, struct bhs_body b);
bool bhs_scene_add_body(bhs_scene_t scene, enum bhs_body_type type,
			struct bhs_vec3 pos, struct bhs_vec3 vel, double mass,
			double radius, struct bhs_vec3 color);
void bhs_scene_remove_body(bhs_scene_t scene, int index);

/* Factories (Legacy/Shim) */
struct bhs_planet_desc; 
struct bhs_body bhs_body_create_planet_simple(struct bhs_vec3 pos, double mass,
					      double radius,
					      struct bhs_vec3 color);
struct bhs_body bhs_body_create_star_simple(struct bhs_vec3 pos, double mass,
					    double radius,
					    struct bhs_vec3 color);
struct bhs_body bhs_body_create_blackhole_simple(struct bhs_vec3 pos,
						 double mass, double radius);

struct bhs_body bhs_body_create_from_desc(const struct bhs_planet_desc *desc, 
					  struct bhs_vec3 pos);

#endif
