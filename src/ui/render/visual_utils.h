#ifndef BHS_UI_RENDER_VISUAL_UTILS_H
#define BHS_UI_RENDER_VISUAL_UTILS_H

#include "src/ui/screens/view_spacetime.h"
#include "engine/scene/scene.h"
#include <math.h>
#include <stdbool.h>

/**
 * @brief Calculates the visual position and radius of a body based on the active Visual Mode.
 *        Implements "Wall-to-Wall" logic for Didactic/Cinematic modes.
 * 
 * @param target The body to calculate for.
 * @param bodies Array of all bodies (needed to find attractor).
 * @param n_bodies Number of bodies.
 * @param mode Current visual mode.
 * @param out_x Output Visual X
 * @param out_y Output Visual Y
 * @param out_z Output Visual Z
 * @param out_radius Output Visual Radius
 */
static inline void bhs_visual_transform_point(
    double px, double py, double pz, /* Input Real Pos */
    double body_radius,              /* Object Radius (for gap calc) */
    enum bhs_body_type type,         /* Object Type (for coloring/scaling logic if needed) */
    const struct bhs_body *bodies,
    int n_bodies,
    bhs_visual_mode_t mode,
    float *out_x,
    float *out_y,
    float *out_z,
    float *out_radius)              /* Output Radius (scaled) */
{
    /* 1. Define Visual Parameters */
    float rad_mult_planet = 1.0f;
    float rad_mult_star = 1.0f;
    float gap_scale = 1.0f;
    bool use_gravity_well = false;
    float well_depth_mult = 1.0f;

    switch (mode) {
    case BHS_VISUAL_MODE_SCIENTIFIC:
        rad_mult_planet = 1.0f;
        rad_mult_star = 1.0f;
        gap_scale = 1.0f;
        use_gravity_well = false;
        break;
    case BHS_VISUAL_MODE_DIDACTIC:
        rad_mult_planet = 1200.0f;
        rad_mult_star = 100.0f;
        gap_scale = 1.0f;
        use_gravity_well = false;
        break;
    case BHS_VISUAL_MODE_CINEMATIC:
        rad_mult_planet = 1200.0f;
        rad_mult_star = 100.0f;
        gap_scale = 1.0f;
        use_gravity_well = true;
        well_depth_mult = 2.0f; 
        break;
    }

    /* 2. Identify Main Attractor */
    int attractor_idx = -1;
    double attractor_mass = -1.0;

    for (int i = 0; i < n_bodies; i++) {
        if (bodies[i].type == BHS_BODY_STAR || bodies[i].type == BHS_BODY_BLACKHOLE) {
            if (bodies[i].state.mass > attractor_mass) {
                attractor_mass = bodies[i].state.mass;
                attractor_idx = i;
            }
        }
    }
    if (attractor_idx == -1 && n_bodies > 0) attractor_idx = 0;

    /* 3. Determine Radius Multiplier */
    float mult = (type == BHS_BODY_STAR) ? rad_mult_star : rad_mult_planet;
    if (out_radius) *out_radius = (float)body_radius * mult;

    /* 4. Determine Position */
    if (attractor_idx == -1 || bodies == NULL) {
        *out_x = (float)px; *out_y = (float)py; *out_z = (float)pz;
        return;
    }

    const struct bhs_body *att = &bodies[attractor_idx];
    
    /* Check if this point IS the attractor (approx check) */
    double d_att_sq = (px - att->state.pos.x)*(px - att->state.pos.x) + 
                      (pz - att->state.pos.z)*(pz - att->state.pos.z);
    
    if (d_att_sq < 1.0) { /* It's the attractor center */
        *out_x = (float)px;
        *out_y = (float)py;
        *out_z = (float)pz;
    } else {
        double dx = px - att->state.pos.x;
        double dz = pz - att->state.pos.z;
        double dist_real_center = sqrt(dx*dx + dz*dz);

        if (dist_real_center < 0.0001) {
             *out_x = (float)px; *out_y = (float)py; *out_z = (float)pz;
        } else {
            double dir_x = dx / dist_real_center;
            double dir_z = dz / dist_real_center;

            /* Surface-to-Surface Logic */
            double gap_real = dist_real_center - att->state.radius - body_radius;
            if (gap_real < 0) gap_real = 0;

            double gap_vis = gap_real * gap_scale;

            float att_mult = (att->type == BHS_BODY_STAR) ? rad_mult_star : rad_mult_planet;
            double r_att_vis = att->state.radius * att_mult;
            double r_body_vis = body_radius * mult;

            double dist_visual_center = r_att_vis + gap_vis + r_body_vis;

            /* Visual Pos */
            double att_vx = att->state.pos.x;
            double att_vz = att->state.pos.z;

            *out_x = (float)(att_vx + dir_x * dist_visual_center);
            *out_z = (float)(att_vz + dir_z * dist_visual_center);
            *out_y = (float)py;
        }
    }

    /* 5. Gravity Well Depth Application */
    if (use_gravity_well) {
        float potential = 0.0f;
        for (int i = 0; i < n_bodies; i++) {
            float dx = *out_x - bodies[i].state.pos.x;
            float dz = *out_z - bodies[i].state.pos.z;
            float dist_sq = dx*dx + dz*dz;
            float dist = sqrtf(dist_sq + 0.1f);
            
            double eff_mass = bodies[i].state.mass;
            if (bodies[i].type == BHS_BODY_PLANET) eff_mass *= 5000.0;
            potential -= (float)(eff_mass / dist);
        }
        float depth = potential * 5.0f;
        if (depth < -50.0f) depth = -50.0f;
        
        *out_y = depth * well_depth_mult;
    }
}

static inline void bhs_visual_calculate_transform(
    const struct bhs_body *target,
    const struct bhs_body *bodies,
    int n_bodies,
    bhs_visual_mode_t mode,
    float *out_x,
    float *out_y,
    float *out_z,
    float *out_radius)
{
    bhs_visual_transform_point(
        target->state.pos.x, 
        target->state.pos.y, 
        target->state.pos.z,
        target->state.radius,
        target->type,
        bodies, n_bodies, mode,
        out_x, out_y, out_z, out_radius
    );
}

#endif /* BHS_UI_RENDER_VISUAL_UTILS_H */
