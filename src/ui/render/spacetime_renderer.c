/**
 * @file spacetime_renderer.c
 * @brief Renderização de Fundo e Corpos Celestes
 */

#include "spacetime_renderer.h"
#include <math.h>
#include <string.h>

/* Helper: Projeta world -> screen */
/* Helper: Projeta world -> screen */
static void project_point(const bhs_camera_t *cam, float x, float y, float z,
			  float sw, float sh, float *ox, float *oy)
{
	/* 1. Translação (World -> Camera Space) */
	float dx = x - cam->x;
	float dy = y - cam->y;
	float dz = z - cam->z;

	/* 2. Rotação (Yaw - Y axis) */
	float cos_yaw = cosf(cam->yaw);
	float sin_yaw = sinf(cam->yaw);

	float x1 = dx * cos_yaw - dz * sin_yaw;
	float z1 = dx * sin_yaw + dz * cos_yaw;
	float y1 = dy;

	/* 3. Rotação (Pitch - X axis) */
	float cos_pitch = cosf(cam->pitch);
	float sin_pitch = sinf(cam->pitch);

	float y2 = y1 * cos_pitch - z1 * sin_pitch;
	float z2 = y1 * sin_pitch + z1 * cos_pitch;
	float x2 = x1;

	/* 4. Projeção Perspectiva */
	/* Prevent division by zero behind camera */
	if (z2 < 0.1f)
		z2 = 0.1f;

	float factor = cam->fov / z2;

	float proj_x = x2 * factor;
	float proj_y = y2 * factor;

	*ox = proj_x + sw * 0.5f;
	*oy = (sh * 0.5f) - proj_y; /* Screen Y flip */
}

/* Helper: Screen -> UV (Spherical) */
static void calculate_sphere_uv(const bhs_camera_t *cam, float width,
				float height, float sx, float sy, float *u,
				float *v)
{
	float rx = (sx - width * 0.5f) / cam->fov;
	float ry = (height * 0.5f - sy) / cam->fov;
	float rz = 1.0f;

	float len = sqrtf(rx * rx + ry * ry + rz * rz);
	rx /= len;
	ry /= len;
	rz /= len;

	/* Rotation matrices */
	float cy = cosf(cam->yaw);
	/* Invert Yaw to match user expectation (Turn Left -> World moves Right
     correctly) Wait, if previous was 'Inverted', maybe now we need direct? User
     said: "Turn left, skybox turns right. Invert THIS." Meaning: Turn Left ->
     Skybox should turn LEFT? (Attached?) OR: Turn Left -> Skybox moves Right is
     CORRECT for 3D. Maybe my previous code produced Turn Left -> Skybox moves
     LEFT (Wrong). Let's flip the sign. It's binary.
   */
	float sy_aw = -sinf(cam->yaw); /* Inverted sign to fix direction */
	float cp = cosf(cam->pitch);
	float sp = sinf(cam->pitch);

	/* Rotate Pitch (X) */
	float ry2 = ry * cp + rz * sp;
	float rz2 = -ry * sp + rz * cp;
	float rx2 = rx;

	/* Rotate Yaw (Y) */
	float rx3 = rx2 * cy - rz2 * sy_aw;
	float rz3 = rx2 * sy_aw + rz2 * cy;
	float ry3 = ry2;

	/* Clamp for asin */
	if (ry3 > 1.0f)
		ry3 = 1.0f;
	if (ry3 < -1.0f)
		ry3 = -1.0f;

	*u = atan2f(rx3, rz3) * (1.0f / (2.0f * 3.14159265f)) + 0.5f;
	*v = 0.5f - (asinf(ry3) * (1.0f / 3.14159265f));
}

#include "src/ui/screens/view_spacetime.h" /* For struct definition */

/* Helper: Calculate gravity depth for object placement */
static float calculate_gravity_depth(float x, float z, const struct bhs_body *bodies, int n_bodies)
{
	if (!bodies || n_bodies == 0) return 0.0f;
	
	float potential = 0.0f;
	for (int i = 0; i < n_bodies; i++) {
		float dx = x - bodies[i].state.pos.x;
		float dz = z - bodies[i].state.pos.z;
		
		float dist_sq = dx*dx + dz*dz;
		float dist = sqrtf(dist_sq + 0.1f);
		
		double eff_mass = bodies[i].state.mass;
		if (bodies[i].type == BHS_BODY_PLANET) {
			eff_mass *= 5000.0; /* Visual boost for planet gravity wells */
		}
		
		potential -= (eff_mass) / dist; /* G=1 visual */
	}
	float depth = potential * 5.0f; /* Potential scale */
	if (depth < -50.0f) depth = -50.0f;
	return depth;
}

void bhs_spacetime_renderer_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
				 const bhs_camera_t *cam, int width, int height,
				 const void *assets_void)
{
	const bhs_view_assets_t *assets = (const bhs_view_assets_t *)assets_void;
	void *tex_bg = assets ? assets->bg_texture : NULL;

	void *tex_bh = assets ? assets->bh_texture : NULL;

	/* -1. Black Hole Compute Output */
	if (tex_bh) {
		struct bhs_ui_color white = { 1.0f, 1.0f, 1.0f, 1.0f };
		bhs_ui_draw_quad_uv(ctx, tex_bh,
				    0, 0, 0, 0,
				    (float)width, 0, 1, 0,
				    (float)width, (float)height, 1, 1,
				    0, (float)height, 0, 1,
				    white);
		tex_bg = NULL; 
	}

	/* 0. Background */
	if (tex_bg) {
		/* ... (Existing Background Logic preserved implies standard skybox) ... */
		/* For brevity, I'll allow the skybox to be drawn if not BH. 
		   Assuming previous logic was correct. 
		   Re-implementing simplified quad for skybox to save tokens/lines if needed,
                   but safer to keep existing loop if I can.
                   Actually, I am replacing the whole file content from line 99, so I must re-include logic.
		*/
        int segs_x = 32; int segs_y = 16;
        float tile_w = (float)width / segs_x;
        float tile_h = (float)height / segs_y;
        struct bhs_ui_color space_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        for (int y = 0; y < segs_y; y++) {
            for (int x = 0; x < segs_x; x++) {
                float x0 = x * tile_w; float y0 = y * tile_h;
                float x1 = (x+1) * tile_w; float y1 = (y+1) * tile_h;
                
                float u0,v0, u1,v1, u2,v2, u3,v3;
				/* Re-use calculate_sphere_uv helper which is static above this function (not replaced) */
                calculate_sphere_uv(cam, width, height, x0, y0, &u0, &v0);
                calculate_sphere_uv(cam, width, height, x1, y0, &u1, &v1);
                calculate_sphere_uv(cam, width, height, x1, y1, &u2, &v2);
                calculate_sphere_uv(cam, width, height, x0, y1, &u3, &v3);
                
                /* Wrap fix */
                if(fabsf(u1-u0)>0.5f) { if(u0<0.5f) u0+=1; else u1+=1; }
                if(fabsf(u2-u1)>0.5f) { if(u1<0.5f) u1+=1; else u2+=1; }
                if(fabsf(u3-u2)>0.5f) { if(u2<0.5f) u2+=1; else u3+=1; }
                if(fabsf(u3-u0)>0.5f) { if(u0<0.5f) u0+=1; else u3+=1; }

                bhs_ui_draw_quad_uv(ctx, tex_bg, x0, y0, u0, v0, x1, y0, u1, v1, x1, y1, u2, v2, x0, y1, u3, v3, space_color);
            }
        }
	}

	/* 2. Bodies */
	int n_bodies = 0;
	const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &n_bodies);

	for (int i = 0; i < n_bodies; i++) {
		const struct bhs_body *b = &bodies[i];
		float sx, sy;

        /* Visual Mapping:
           Physics Pos (x, y) -> Visual (x, z) 
           Y is 0 in 2D physics.
        */
		float visual_x = b->state.pos.x;
		float visual_z = b->state.pos.z; /* Physics Z -> Visual Z (Matches presets.c X-Z orbit) */
		
		/* Calculate depth based on gravity well logic */
		float visual_y = calculate_gravity_depth(visual_x, visual_z, bodies, n_bodies);

		project_point(cam, visual_x, visual_y, visual_z, width, height, &sx, &sy);
		
		/* ... [Using same projection/scaling logic as before] ... */
		float dx = visual_x - cam->x;
		float dy = visual_y - cam->y; /* Including depth in distance */
		float dz = visual_z - cam->z;
		float dist = sqrtf(dx*dx + dy*dy + dz*dz);
		if (dist < 0.1f) dist = 0.1f;
		
		/* VISUAL SCALE: Uniform 80x */
		float visual_radius = b->state.radius * 80.0f;
		float s_radius = (visual_radius / dist) * cam->fov;
		if (s_radius < 2.0f) s_radius = 2.0f;

		struct bhs_ui_color color = { (float)b->color.x, (float)b->color.y, (float)b->color.z, 1.0f };
		
		/* [FIX] Aggressive Refactor: NO 2D DRAWING for Celestial Bodies */
		/* We only draw labels for planets/stars to help identification in 3D view */
		if (b->type == BHS_BODY_PLANET || b->type == BHS_BODY_STAR || b->type == BHS_BODY_MOON) {
            /* Only draw text if visible on screen */
            if (sx > 0 && sx < width && sy > 0 && sy < height) {
    			const char *label = (b->name[0]) ? b->name : "Planet";
	    		bhs_ui_draw_text(ctx, label, sx + 5, sy, 12.0f, BHS_UI_COLOR_WHITE);
            }
			/* NO 2D SPRITES - "Garantir ao supremo que o planeta tá só 3d" */
		} else {
             /* Other bodies (Asteroids, Ships?) can stay simple circles for now */
             bhs_ui_draw_circle_fill(ctx, sx, sy, s_radius, color);
		}
	}
}

