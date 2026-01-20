/**
 * @file planet_renderer.c
 * @brief Renderizador de Planetas 3D
 */

#include "planet_renderer.h"
#include "src/engine/geometry/mesh_gen.h"
#include "src/math/mat4.h"
#include "engine/assets/image_loader.h" /* Utils if needed */
#include "gui-framework/log.h"
#include <stdlib.h>
#include <string.h>

/* Shaders (embed via build system or file load? Using file load for now based on pattern) */
/* Actually, codebase uses `bhs_read_file` or embedded? 
   Let's check `blackhole_pass.c`... It reads `.spv` from `assets/shaders/`.
   I need to make sure my shaders are compiled to SPV by CMake.
   The CMakeLists.txt handles `blackhole_sim_shaders` target?
   Yes, `[ 6%] Built target blackhole_sim_shaders` seen in logs.
   I need to check if it automatically picks up new shaders in `src/assets/shaders`.
*/

#ifndef PI
#define PI 3.14159265359f
#endif

struct bhs_planet_pass {
	bhs_ui_ctx_t ctx;
	bhs_gpu_pipeline_t pipeline;
	bhs_gpu_shader_t vs, fs;
	bhs_gpu_buffer_t vbo, ibo;
	bhs_gpu_sampler_t sampler;
	uint32_t index_count;
};

/* Push Constants matching shader */
/* Push Constants matching shader (PACKED to 128 bytes) */
struct planet_pc {
	bhs_mat4_t viewProj;    /* 64 bytes */
	float modelParams[4];   /* xyz=pos, w=radius */
	float rotParams[4];     /* xyz=axis, w=angle */
	float lightAndStar[4];  /* xyz=lightPos, w=isStar */
	float colorParams[4];   /* xyz=color, w=pad */
}; /* Total 128 bytes */

/* Helper: Gravity Depth (Copied from spacetime_renderer.c to match grid) */
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
			eff_mass *= 5000.0; /* Match bhs_fabric visual boost */
		}
		
		potential -= (eff_mass) / dist;
	}
	float depth = potential * 5.0f;
	if (depth < -50.0f) depth = -50.0f;
	return depth;
}

/* Private Helper: Load Shader Code */
static int load_shader(bhs_gpu_device_t dev, const char *rel_path, enum bhs_gpu_shader_stage stage, bhs_gpu_shader_t *out) {
	/* Try multiple prefixes to find the shader */
	const char *prefixes[] = {
		"",                      /* If running from bin/ or if assets are in CWD */
		"build/bin/",            /* If running from root and shaders are in build */
		"../",                   /* If running from some subdir */
		"bin/",                  /* Generic */
		"assets/"                /* Redundant check */
	};
	
	FILE *f = NULL;
	char full_path[512];
	
	for (int i = 0; i < 5; i++) {
		snprintf(full_path, sizeof(full_path), "%s%s", prefixes[i], rel_path);
		f = fopen(full_path, "rb");
		if (f) {
			BHS_LOG_INFO("Shader found at: %s", full_path);
			break;
		}
	}

	if (!f) {
		BHS_LOG_ERROR("Shader NOT found: %s (checked common paths)", rel_path);
		return BHS_GPU_ERR_INVALID;
	}
	
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	void *code = malloc(size);
	if (!code) { fclose(f); return BHS_GPU_ERR_NOMEM; }
	fread(code, 1, size, f);
	fclose(f);
	
	struct bhs_gpu_shader_config conf = {
		.stage = stage,
		.code = code,
		.code_size = size,
		.entry_point = "main"
	};
	
	int res = bhs_gpu_shader_create(dev, &conf, out);
	free(code);
	
	if (res != 0) {
		BHS_LOG_ERROR("Shader compilation/create failed for: %s", full_path);
	}
	
	return res;
}

int bhs_planet_pass_create(bhs_ui_ctx_t ctx, bhs_planet_pass_t *out_pass)
{
	bhs_planet_pass_t p = calloc(1, sizeof(*p));
	if (!p) return -1;
	
	p->ctx = ctx;
	bhs_gpu_device_t dev = bhs_ui_get_gpu_device(ctx);
	
	/* 1. Load Shaders (Hope CMake compiled them to SPV) */
	/* Paths relative to binary CWD? */
	if (load_shader(dev, "assets/shaders/planet.vert.spv", BHS_SHADER_VERTEX, &p->vs) != 0) {
		BHS_LOG_ERROR("Failed to load planet.vert.spv");
		free(p); return -1;
	}
	if (load_shader(dev, "assets/shaders/planet.frag.spv", BHS_SHADER_FRAGMENT, &p->fs) != 0) {
		BHS_LOG_ERROR("Failed to load planet.frag.spv");
		free(p); return -1;
	}
	
	/* 2. Create Sphere Mesh */
	bhs_mesh_t mesh = bhs_mesh_gen_sphere(32, 32);
	p->index_count = mesh.index_count;
	
	/* VBO */
	struct bhs_gpu_buffer_config vbo_conf = {
		.size = mesh.vertex_count * sizeof(bhs_vertex_3d_t),
		.usage = BHS_BUFFER_VERTEX,
		.memory = BHS_MEMORY_CPU_TO_GPU, /* Dynamic upload for simplicity or Staged? Use CPU_TO_GPU for immutable if possible? */
		/* Use CPU_TO_GPU for simplicity on UMA, or separate staging. The API hides details usually. */
	};
	bhs_gpu_buffer_create(dev, &vbo_conf, &p->vbo);
	bhs_gpu_buffer_upload(p->vbo, 0, mesh.vertices, vbo_conf.size);
	
	/* IBO */
	struct bhs_gpu_buffer_config ibo_conf = {
		.size = mesh.index_count * sizeof(uint16_t),
		.usage = BHS_BUFFER_INDEX,
		.memory = BHS_MEMORY_CPU_TO_GPU,
	};
	bhs_gpu_buffer_create(dev, &ibo_conf, &p->ibo);
	bhs_gpu_buffer_upload(p->ibo, 0, mesh.indices, ibo_conf.size);
	
	bhs_mesh_free(mesh);
	
	/* 3. Sampler */
	struct bhs_gpu_sampler_config samp_conf = {
		.min_filter = BHS_FILTER_LINEAR,
		.mag_filter = BHS_FILTER_LINEAR,
		.address_u = BHS_ADDRESS_REPEAT,
		.address_v = BHS_ADDRESS_CLAMP_TO_EDGE, /* Texture Lat/Long map */
	};
	bhs_gpu_sampler_create(dev, &samp_conf, &p->sampler);
	
	/* 4. Pipeline Config */
	struct bhs_gpu_vertex_attr attrs[] = {
		{ .location = 0, .binding = 0, .format = BHS_FORMAT_RGB32_FLOAT, .offset = offsetof(bhs_vertex_3d_t, pos) },
		{ .location = 1, .binding = 0, .format = BHS_FORMAT_RGB32_FLOAT, .offset = offsetof(bhs_vertex_3d_t, normal) },
		{ .location = 2, .binding = 0, .format = BHS_FORMAT_RG32_FLOAT, .offset = offsetof(bhs_vertex_3d_t, uv) }
	};
	struct bhs_gpu_vertex_binding binding = {
		.binding = 0,
		.stride = sizeof(bhs_vertex_3d_t),
		.per_instance = false
	};
	
	enum bhs_gpu_texture_format color_fmt = BHS_FORMAT_BGRA8_SRGB; /* Match Swapchain/UI Pass */
	
	struct bhs_gpu_pipeline_config pipe_conf = {
		.vertex_shader = p->vs,
		.fragment_shader = p->fs,
		.vertex_attrs = attrs,
		.vertex_attr_count = 3,
		.vertex_bindings = &binding,
		.vertex_binding_count = 1,
		.primitive = BHS_PRIMITIVE_TRIANGLES,
		.cull_mode = BHS_CULL_BACK, /* Enable Backface Culling for solid look */
		.front_ccw = true, /* Sphere gen uses CCW */
		.depth_test = true, /* ENABLE 3D! */
		.depth_write = true,
		.depth_compare = BHS_COMPARE_LESS_EQUAL,
		.color_formats = &color_fmt,
		.color_format_count = 1,
		.depth_format = BHS_FORMAT_DEPTH32_FLOAT, /* Typical Depth */
		.label = "Planet Pipeline"
	};
	
	if (bhs_gpu_pipeline_create(dev, &pipe_conf, &p->pipeline) != 0) {
		BHS_LOG_ERROR("Failed to create Planet Pipeline");
		/* cleanup */
		return -1;
	}
	
	*out_pass = p;
	return 0;
}

void bhs_planet_pass_destroy(bhs_planet_pass_t pass)
{
	if (!pass) return;
	bhs_gpu_pipeline_destroy(pass->pipeline);
	bhs_gpu_shader_destroy(pass->vs);
	bhs_gpu_shader_destroy(pass->fs);
	bhs_gpu_buffer_destroy(pass->vbo);
	bhs_gpu_buffer_destroy(pass->ibo);
	bhs_gpu_sampler_destroy(pass->sampler);
	free(pass);
}

void bhs_planet_pass_draw(bhs_planet_pass_t pass,
			  bhs_gpu_cmd_buffer_t cmd,
			  bhs_scene_t scene,
			  const bhs_camera_t *cam,
			  const bhs_view_assets_t *assets,
			  float output_width, float output_height)
{
	if (!pass || !cmd || !scene) return;
	
	/* Explicitly set Viewport/Scissor (Dynamic State) */
	bhs_gpu_cmd_set_viewport(cmd, 0, 0, output_width, output_height, 0.0f, 1.0f);
	bhs_gpu_cmd_set_scissor(cmd, 0, 0, (uint32_t)output_width, (uint32_t)output_height);

	bhs_gpu_cmd_set_pipeline(cmd, pass->pipeline);
	bhs_gpu_cmd_set_vertex_buffer(cmd, 0, pass->vbo, 0);
	bhs_gpu_cmd_set_index_buffer(cmd, pass->ibo, 0, false); /* 16 bit */
	
	/* View/Proj Matrices */
	/* 
	 * MANUAL VIEW MATRIX CONSTRUCTION
	 * Goal: Match 'spacetime_renderer.c' logic exactly.
	 * Logic: 1. Translate (RTC handled via Model) -> 2. Yaw (Y) -> 3. Pitch (X)
	 * Standard Matrix Multiplication: M = P * V * M
	 * V = R_pitch * R_yaw
	 */
	
	float cy = cosf(cam->yaw);
	float sy = sinf(cam->yaw);
	float cp = cosf(cam->pitch);
	float sp = sinf(cam->pitch);
	
	/* Rotation Yaw (Y-Axis) */
	/* Matches spacetime: x' = x*cy - z*sy; z' = x*sy + z*cy */
	/* Column 0 (Input X): contributes to x' (cos) and z' (sin) */
	/* Column 2 (Input Z): contributes to x' (-sin) and z' (cos) */
	bhs_mat4_t m_yaw = bhs_mat4_identity();
	m_yaw.m[0] = cy;   /* Row 0, Col 0: x->x */
	m_yaw.m[2] = sy;   /* Row 2, Col 0: x->z (z' = x*sy...) */
	m_yaw.m[8] = -sy;  /* Row 0, Col 2: z->x (x' = ... - z*sy) */
	m_yaw.m[10] = cy;  /* Row 2, Col 2: z->z */
	
	/* Rotation Pitch (X-Axis) */
	/* Matches spacetime: y' = y*cp - z*sp; z' = y*sp + z*cp */
	/* Column 1 (Input Y): contributes to y' (cos) and z' (sin) */
	/* Column 2 (Input Z): contributes to y' (-sin) and z' (cos) */
	bhs_mat4_t m_pitch = bhs_mat4_identity();
	m_pitch.m[5] = cp;   /* Row 1, Col 1: y->y */
	m_pitch.m[6] = sp;   /* Row 2, Col 1: y->z (z' = y*sp...) */
	m_pitch.m[9] = -sp;  /* Row 1, Col 2: z->y (y' = ... - z*sp) */
	m_pitch.m[10] = cp;  /* Row 2, Col 2: z->z */
	
	/* Combined View Matrix: V = Pitch * Yaw */
	/* Since we are using Column-Major (OpenGL/Vulkan standard), 
	   and we want to apply Yaw THEN Pitch on the vector v:
	   v' = P * Y * v
	   So Matrix = P * Y
	*/
	bhs_mat4_t mat_view = bhs_mat4_mul(m_pitch, m_yaw);
	
	/* NO Z-FLIP NEEDED.
	   spacetime_renderer projects z' directly. 
	   It treats +Z as forward depth.
	   Vulkan expects +Z as depth.
	   So this matrix is natively compatible.
	*/
	
	/* Proj Matrix */
	float focal_length = cam->fov;
	if (focal_length < 1.0f) focal_length = 1.0f;
	
	float fov_y = 2.0f * atanf((output_height * 0.5f) / focal_length);
	float aspect = output_width / output_height;
	
	bhs_mat4_t mat_proj = bhs_mat4_perspective(fov_y, aspect, 0.1f, 2000.0f);
	
	/* Pre-multiply ViewProj for Packed PC */
	bhs_mat4_t mat_vp = bhs_mat4_mul(mat_proj, mat_view);
	
	/* Light Pos (Sun at 0,0,0) */
	float light_pos[3] = { 0.0f, 0.0f, 0.0f };
	
	/* Iterate Bodies */
	int count = 0;
	const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &count);
	
	for (int i = 0; i < count; i++) {
		const struct bhs_body *b = &bodies[i];
		if (b->type != BHS_BODY_PLANET && b->type != BHS_BODY_STAR && b->type != BHS_BODY_MOON) continue;
		
		/* Textura ... */
		bhs_gpu_texture_t tex = assets->sphere_texture;
		if (assets->tex_cache) {
			for (int t=0; t < assets->tex_cache_count; t++) {
				if (strcmp(assets->tex_cache[t].name, b->name) == 0) {
					tex = assets->tex_cache[t].tex;
					break;
				}
			}
		}
		bhs_gpu_cmd_bind_texture(cmd, 0, 0, tex, pass->sampler);
		
		/* RTC Position */
		float visual_y = calculate_gravity_depth((float)b->state.pos.x, (float)b->state.pos.z, bodies, count);
		
		double rel_x = b->state.pos.x - cam->x;
		double rel_y = visual_y - cam->y;
		double rel_z = b->state.pos.z - cam->z;
		
		float tx = (float)rel_x;
		float ty = (float)rel_y;
		float tz = (float)rel_z;
		float radius = (float)b->state.radius * 30.0f;
		
		/* Rotation Params */
		float angle = (float)b->state.current_rotation_angle;
		float ax = (float)b->state.rot_axis.x;
		float ay = (float)b->state.rot_axis.y;
		float az = (float)b->state.rot_axis.z;
		
		/* Ensure valid axis for shader normalize() */
		if (fabsf(ax) + fabsf(ay) + fabsf(az) < 0.01f) {
			ax = 0.0f; ay = 1.0f; az = 0.0f;
		}

		/* Fill Packed Push Constants */
		struct planet_pc pc;
		pc.viewProj = mat_vp;
		
		pc.modelParams[0] = tx;
		pc.modelParams[1] = ty;
		pc.modelParams[2] = tz;
		pc.modelParams[3] = radius;
		
		pc.rotParams[0] = ax;
		pc.rotParams[1] = ay;
		pc.rotParams[2] = az;
		pc.rotParams[3] = angle;
		
		pc.lightAndStar[0] = light_pos[0] - (float)cam->x;
		pc.lightAndStar[1] = light_pos[1] - (float)cam->y;
		pc.lightAndStar[2] = light_pos[2] - (float)cam->z;
		pc.lightAndStar[3] = (b->type == BHS_BODY_STAR) ? 1.0f : 0.0f;
		
		pc.colorParams[0] = (float)b->color.x;
		pc.colorParams[1] = (float)b->color.y;
		pc.colorParams[2] = (float)b->color.z;
		pc.colorParams[3] = 0.0f;
		
		bhs_gpu_cmd_push_constants(cmd, 0, &pc, sizeof(pc));
		bhs_gpu_cmd_draw_indexed(cmd, pass->index_count, 1, 0, 0, 0);
	}
}
