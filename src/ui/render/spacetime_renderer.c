/**
 * @file spacetime_renderer.c
 * @brief Renderização Pura da Malha (Projeção e Desenho de Linhas)
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


#include "include/bhs_fabric.h" /* [NEW] */

/* Helper: Calculate gravity depth for object placement */
static float calculate_gravity_depth(float x, float z, const struct bhs_body *bodies, int n_bodies)
{
	if (!bodies || n_bodies == 0) return 0.0f;
	
	float potential = 0.0f;
	for (int i = 0; i < n_bodies; i++) {
		float dx = x - bodies[i].state.pos.x;
		float dz = z - bodies[i].state.pos.z; /* Z is physics Y? Wait. Physics uses X, Y, Z. */
		/* In this renderer:
		   visual_x = pos.x
		   visual_y = pos.y (Physics Y is Up/Down? Or Z?) 
		   Looking at project_point: x, y, z. 
		   Looking at body loop: visual_x = pos.x, visual_y = pos.y, visual_z = pos.z.
		   Physics usually is 2D on X,Y plane? Or X,Z?
		   The fabric uses X,Y as plane, Z as depth.
		   So for calculating depth at (x,y), we sum.
		   
		   Wait, `bhs_fabric.c`:
		     dx = v->pos.x - b->position.x;
             dy = v->pos.y - b->position.y;
             v->cur.z = deform;
             
           So Fabric UP is Z.
           In this renderer, project_point takes x, y, z.
           It rotates Yaw around Y axis. Pitch around X axis.
           So Y is UP.
           
           Fabric logic: "cur.z = deform". This implies Z is the "depth" dimension in Fabric local space?
           Usually Embedding diagrams have Z as depth.
           
           But `project_point` assumes standard 3D axes. Y usually is UP in OpenGL/Games.
           If Fabric uses Z as depth, we probably need to map:
           Fabric X -> World X
           Fabric Y -> World Z (Ground plane)
           Fabric Z -> World Y (Depth/Elevation)
           
           Let's verify `bhs_fabric.c` implementation:
             v->pos.x = (x * spacing) ...
             v->pos.y = (y * spacing) ...
             v->pos.z = 0.0;
             
             r2 = dx*dx + dy*dy; (Distance in XY plane)
             v->cur.z = deform;
             
           It seems Fabric assumes X,Y is the plane.
           If the camera coordinate system (Y-up) expects ground on X-Z, then we need to swap Y and Z when rendering fabric vertices.
           
           Legacy Grid code:
             x00 = vertices[0], y00 = vertices[1] * depth_scale, z00 = vertices[2];
           It scaled Y. So legacy grid had Y as depth?
           
           Let's assume we map:
           Fabric.x -> World.x
           Fabric.y -> World.z
           Fabric.z -> World.y (Depth)
           
           And when calculating depth for planets:
           Planet.x -> World.x
           Planet.y -> World.z (Physics pos.y is actually z in 3D view?)
           
           Let's look at `app_state.c` / `spacetime_renderer.c`:
             visual_x = b->state.pos.x;
             visual_y = b->state.pos.y;
             visual_z = b->state.pos.z;
             
           If simulation is 2D (X, Y), then Z is 0.
           If we want planets to sink, we modify visual_y?
           Legacy code: `visual_y = get_depth(...)`. So Y is Up/Down.
           
           So physics (x,y) maps to visual (x, z). And visual y is depth.
           Let's stick to this mapping.
		*/
		
		float dist_sq = dx*dx + dz*dz;
		float dist = sqrtf(dist_sq + 0.1f);
		
		double eff_mass = bodies[i].state.mass;
		if (bodies[i].type == BHS_BODY_PLANET) {
			eff_mass *= 5000.0; /* Match bhs_fabric visual boost */
		}
		
		potential -= (eff_mass) / dist; /* G=1 visual */
	}
	float depth = potential * 5.0f; /* Scale matches BHS_FABRIC_POTENTIAL_SCALE */
	if (depth < -50.0f) depth = -50.0f;
	return depth;
}

void bhs_spacetime_renderer_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
				 const bhs_camera_t *cam, int width, int height,
				 const void *assets_void)
{
	const bhs_view_assets_t *assets = (const bhs_view_assets_t *)assets_void;
	void *tex_bg = assets ? assets->bg_texture : NULL;
	void *tex_sphere = assets ? assets->sphere_texture : NULL;
	void *tex_bh = assets ? assets->bh_texture : NULL;
	const struct bhs_fabric *fabric = assets ? assets->fabric : NULL; /* [NEW] */

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

	/* 
	 * [PHASE 1] Doppler Fabric (High Fidelity Rendering)
	 * Strategy:
	 * 1. Calculate Lighting per vertex (CPU-side Gouraud Shading style)
	 * 2. Draw Filled Quads (Surface)
	 * 3. Draw Wireframe Overlay (Definition)
	 */
	if (assets && assets->show_grid && fabric) {
		/* Palette Definition (Sci-Fi Deep Space) */
		struct bhs_ui_color col_base = { 0.02f, 0.05f, 0.15f, 1.0f };   /* Deep Void Blue */
		struct bhs_ui_color col_high = { 0.1f, 0.2f, 0.5f, 1.0f };      /* Illuminated Blue */
		struct bhs_ui_color col_grid = { 0.3f, 0.6f, 0.9f, 0.3f };      /* Subtle Wireframe */
		struct bhs_ui_color col_rim  = { 0.0f, 0.8f, 1.0f, 1.0f };      /* Electric Rim Light */

		uint32_t w = fabric->width;
		uint32_t h = fabric->height;
		struct bhs_vec3 light_dir = { 0.5f, 0.8f, 0.3f }; /* Fixed Directional Light */
		
		/* Normalize light dir manually */
		double l_len = sqrt(light_dir.x*light_dir.x + light_dir.y*light_dir.y + light_dir.z*light_dir.z);
		light_dir.x /= l_len; light_dir.y /= l_len; light_dir.z /= l_len;

		/* Camera View Vector (Approximate based on Yaw/Pitch for Rim Light) */
		struct bhs_vec3 view_dir = {
			sinf(cam->yaw) * cosf(cam->pitch),
			sinf(cam->pitch),
			cosf(cam->yaw) * cosf(cam->pitch)
		}; /* This is camera forward. View dir is usually -Forward. */
        (void)view_dir; /* Unused but kept for reference logic */

		/* Iterate Quads (Cells) */
		for (uint32_t y = 0; y < h - 1; y++) {
			for (uint32_t x = 0; x < w - 1; x++) {
				/* Vertex Indices */
				uint32_t i00 = y * w + x;
				uint32_t i10 = y * w + (x + 1);
				uint32_t i11 = (y + 1) * w + (x + 1);
				uint32_t i01 = (y + 1) * w + x;

				struct bhs_fabric_vertex *v00 = &fabric->vertices[i00];
				struct bhs_fabric_vertex *v10 = &fabric->vertices[i10];
				struct bhs_fabric_vertex *v11 = &fabric->vertices[i11];
				struct bhs_fabric_vertex *v01 = &fabric->vertices[i01];

				/* Project Points */
				float sx00, sy00, sx10, sy10, sx11, sy11, sx01, sy01;
				
				/* Mapping: Fabric(x,y,z) -> Visual(x,z,y) due to embedding orientation */
				/* Fabric Z is deformation (depth) -> Visual Y (Up/Down) */
				project_point(cam, v00->cur.x, v00->cur.z, v00->cur.y, width, height, &sx00, &sy00);
				project_point(cam, v10->cur.x, v10->cur.z, v10->cur.y, width, height, &sx10, &sy10);
				project_point(cam, v11->cur.x, v11->cur.z, v11->cur.y, width, height, &sx11, &sy11);
				project_point(cam, v01->cur.x, v01->cur.z, v01->cur.y, width, height, &sx01, &sy01);

				/* 
				 * Lighting Calculation (Lambert + Fresnel) 
				 * Calculated for the quad center (flat shading) or average? 
				 * Flat shading is faster and gives that "low poly" retro-futuristic look.
				 */
				double nx = (v00->normal.x + v10->normal.x + v11->normal.x + v01->normal.x) * 0.25;
				double ny = (v00->normal.y + v10->normal.y + v11->normal.y + v01->normal.y) * 0.25;
				double nz = (v00->normal.z + v10->normal.z + v11->normal.z + v01->normal.z) * 0.25;
				
				/* Remap normal to visual space: Fabric (x,y,z) -> Visual (x,z,y) ? 
				   Wait, if Fabric Z is depth, and Visual Y is depth.
				   Normal was computed in Fabric space. 
				   tx_z was computed from Z difference (depth).
				   So Normal.z is the component along depth axis.
				   In Visual space, Y is depth. So we must swap Y and Z in normal too?
				   Normal(x,y,z) -> Visual(x, z, y).
				*/
				double vis_nx = nx;
				double vis_ny = nz; /* Depth component is Y in visual */
				double vis_nz = ny; /* Plane component */

				/* Diffuse: N dot L */
				/* Light is coming from top-right-ish */
				double diff = vis_nx*light_dir.x + vis_ny*light_dir.y + vis_nz*light_dir.z;
				if (diff < 0) diff = 0;

				/* Fresnel: 1 - (N dot V) */
				/* View dir approx. Need simple approximation. N.y (Up) usually correlates. */
				/* Let's use a simpler heuristic: Slope. The steeper the slope, the brighter the rim. */
				/* In fabric space, steep slope means normal.z (depth) is small (horizontal). 
				   Flat space means normal.z is 1.0.
				   So 1.0 - normal.z gives slope intensity.
				*/
				double slope = 1.0 - fabs(v00->normal.z); /* Fabric Z is depth/up in its local frame? No, Z is depth. */
				/* Wait, bhs_fabric.c fallback normal is (0,0,1). That means Z is UP/Normal to plane in Fabric Space logic. 
				   Checking bhs_fabric.c:
				   Fallback: v->normal.z = 1;
				   So Z is the "height" of the fabric membrane.
				   Steep well = Z component of normal drops, X/Y increase.
				*/
				double fresnel = slope * slope; /* Sharpen curve */

				/* Compose Color */
				struct bhs_ui_color quad_col = col_base;
				
				/* Add Diffuse Highlight */
				quad_col.r += col_high.r * diff * 0.5f;
				quad_col.g += col_high.g * diff * 0.5f;
				quad_col.b += col_high.b * diff * 0.5f;

				/* Add Fresnel/Rim (Electric Blue on edges) */
				quad_col.r += col_rim.r * fresnel;
				quad_col.g += col_rim.g * fresnel;
				quad_col.b += col_rim.b * fresnel;

				/* Clamp Alpha */
				quad_col.a = 0.9f; /* Slightly transparent */

				/* Draw Filled Quad */
				bhs_ui_draw_quad_uv(ctx, NULL, 
					sx00, sy00, 0,0, 
					sx10, sy10, 1,0, 
					sx11, sy11, 1,1, 
					sx01, sy01, 0,1, 
					quad_col);

				/* Draw Wireframe (Overlay) */
				/* Only draw grid lines if simple or near camera?
				   Let's draw all for now, optimized by simple line draw.
				*/
				bhs_ui_draw_line(ctx, sx00, sy00, sx10, sy10, col_grid, 1.0f); /* Top */
				bhs_ui_draw_line(ctx, sx00, sy00, sx01, sy01, col_grid, 1.0f); /* Left */
				
				/* Bottom and Right will be drawn by neighbor cells, avoiding duplication 
				   Logic: Draw Top and Left for every cell.
				   Last row/col needs specific handling or just ignore (open edge is fine visually).
				*/
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
		
		float visual_radius = b->state.radius * 30.0f; /* Keep the 30x scale */
		float s_radius = (visual_radius / dist) * cam->fov;
		if (s_radius < 2.0f) s_radius = 2.0f;

		struct bhs_ui_color color = { (float)b->color.x, (float)b->color.y, (float)b->color.z, 1.0f };
		
		if (b->type == BHS_BODY_PLANET) {
			const char *label = (b->name[0]) ? b->name : "Planet";
            bhs_ui_draw_circle_fill(ctx, sx, sy, s_radius, color);
            
            /* Impostor Logic */
            void *use_tex = tex_sphere;
            if (assets && assets->tex_cache) {
                for(int t=0; t<assets->tex_cache_count; t++) {
                    if (strncmp(assets->tex_cache[t].name, b->name, 32) == 0) {
                        use_tex = assets->tex_cache[t].tex;
                        break;
                    }
                }
            }
            if (use_tex) {
                struct bhs_ui_color tint = (use_tex == tex_sphere) ? color : (struct bhs_ui_color){1,1,1,1};
                bhs_ui_draw_texture(ctx, use_tex, sx-s_radius, sy-s_radius, s_radius*2, s_radius*2, tint);
            }
            bhs_ui_draw_text(ctx, label, sx + s_radius + 5, sy, 12.0f, BHS_UI_COLOR_WHITE);
		} else {
             bhs_ui_draw_circle_fill(ctx, sx, sy, s_radius, color);
		}
	}
}

