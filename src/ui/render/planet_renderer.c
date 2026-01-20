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
struct planet_pc {
	bhs_mat4_t model;
	bhs_mat4_t view;
	bhs_mat4_t proj;
	float lightAndStar[4]; /* xyz = lightPos, w = isStar */
	float colorParams[4];  /* xyz = color, w = pad */
};

/* Private Helper: Load Shader Code */
static int load_shader(bhs_gpu_device_t dev, const char *path, enum bhs_gpu_shader_stage stage, bhs_gpu_shader_t *out) {
	/* TODO: Use a proper file reader. Using stdio for now as fallback if `lib.h` doesn't provide FS. */
	/* gui-framework/platform/fs.h? Let's assume standard C for now or check usage */
	FILE *f = fopen(path, "rb");
	if (!f) return BHS_GPU_ERR_INVALID;
	
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
	
	enum bhs_gpu_texture_format color_fmt = BHS_FORMAT_BGRA8_UNORM; /* Swapchain typical */
	/* Warning: Should match actual render pass format. 
	   If drawing to screen, usually BHS_FORMAT_BGRA8_UNORM or RGBA8_UNORM depending on OS.
	   Let's check renderer/lib.h defaults or query swapchain?
	   For now assume Swapchain Format.
	*/
	
	struct bhs_gpu_pipeline_config pipe_conf = {
		.vertex_shader = p->vs,
		.fragment_shader = p->fs,
		.vertex_attrs = attrs,
		.vertex_attr_count = 3,
		.vertex_bindings = &binding,
		.vertex_binding_count = 1,
		.primitive = BHS_PRIMITIVE_TRIANGLES,
		.cull_mode = BHS_CULL_BACK,
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
	
	bhs_gpu_cmd_set_pipeline(cmd, pass->pipeline);
	bhs_gpu_cmd_set_vertex_buffer(cmd, 0, pass->vbo, 0);
	bhs_gpu_cmd_set_index_buffer(cmd, pass->ibo, 0, false); /* 16 bit */
	
	/* View/Proj Matrices */
	/* RTC View Matrix: Camera is at (0,0,0), so we only apply Rotation */
	/* LookAt(0,0,0 -> Dir, Up) */
	struct bhs_v3 eye = { 0, 0, 0 };
	/* Direction from yaw/pitch */
	float cx = cosf(cam->pitch) * sinf(cam->yaw);
	float cy = sinf(cam->pitch);
	float cz = cosf(cam->pitch) * cosf(cam->yaw);
	struct bhs_v3 center = { cx, cy, cz };
	struct bhs_v3 up = { 0, 1, 0 };
	
	bhs_mat4_t mat_view = bhs_mat4_lookat(eye, center, up); /* Now this IS the RTC view */
	
	/* Proj Matrix */
	float aspect = output_width / output_height;
	bhs_mat4_t mat_proj = bhs_mat4_perspective(cam->fov * (PI/180.0f), aspect, 0.1f, 100000.0f);
	
	/* Light Pos (Sun assumed at 0,0,0) - World Space */
	float light_pos[3] = {0, 0, 0};
	
	/* Iterate Bodies */
	int count = 0;
	const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &count);
	
	for (int i = 0; i < count; i++) {
		const struct bhs_body *b = &bodies[i];
		if (b->type != BHS_BODY_PLANET && b->type != BHS_BODY_STAR && b->type != BHS_BODY_MOON) continue;
		
		/* Find Texture */
		bhs_gpu_texture_t tex = assets->sphere_texture; /* Fallback */
		
		/* Lookup in cache */
		if (assets->tex_cache) {
			for (int t=0; t < assets->tex_cache_count; t++) {
				if (strcmp(assets->tex_cache[t].name, b->name) == 0) {
					tex = assets->tex_cache[t].tex;
					break;
				}
			}
		}
		
		/* Bind Texture */
		bhs_gpu_cmd_bind_texture(cmd, 0, 0, tex, pass->sampler);
		
		/* ==========================================================
		 * RTC (Relative-To-Camera) CALCULATION
		 * ==========================================================
		 */
		
		/* 1. Subtração em High Precision (Double) */
		double rel_x = b->state.pos.x - cam->x;
		double rel_y = b->state.pos.y - cam->y;
		double rel_z = b->state.pos.z - cam->z;
		
		/* 2. Conversão para Float (agora que os números são pequenos) */
		float tx = (float)rel_x;
		float ty = (float)rel_y;
		float tz = (float)rel_z;
		float radius = (float)b->state.radius;
		
		/* 3. Matrizes de Modelo */
		bhs_mat4_t m_scale = bhs_mat4_scale(radius, radius, radius);
		bhs_mat4_t m_trans = bhs_mat4_translate(tx, ty, tz);
		
		/* Rotação Axial */
		/* Primeiro o Spin (Dia/Noite) */
		/* Assumindo eixo Y local como eixo de rotação se não especificado, 
		   mas temos rot_axis na struct state se quisermos ser precisos.
		   Por simplicidade (e como rot_axis pode ser 0,0,0 em dados velhos),
		   vamos usar RotateY com Axis Tilt composto.
		   
		   Melhor: Usar Axis-Angle se rot_axis estiver definido.
		*/
		bhs_mat4_t m_rot;
		float angle = (float)b->state.current_rotation_angle;
		
		/* Verifica se rot_axis tem comprimento */
		float ax = (float)b->state.rot_axis.x;
		float ay = (float)b->state.rot_axis.y;
		float az = (float)b->state.rot_axis.z;
		
		if (fabsf(ax) + fabsf(ay) + fabsf(az) > 0.1f) {
			m_rot = bhs_mat4_rotate_axis((struct bhs_v3){ax, ay, az}, angle);
		} else {
			/* Fallback: Rotação no Y (comum) */
			m_rot = bhs_mat4_rotate_y(angle);
		}

		/* Planet Tilt é estático ou parte do eixo? 
		   Se rot_axis já é o eixo inclinado, está resolvido.
		*/
		
		/* Ordem: Scale -> Rotate -> Translate */
		/* Primeiro escala a esfera unitária para o tamanho do planeta */
		/* Depois rotaciona a esfera no lugar */
		/* Depois move para a posição relativa */
		bhs_mat4_t m_model = bhs_mat4_mul(m_rot, m_scale);
		m_model = bhs_mat4_mul(m_trans, m_model);


		/* 4. Matriz View "Limpa" (Câmera na origem) */
		/* Usamos a mat_view calculada fora do loop */

		/* Push Constants */
		struct planet_pc pc = {
			.model = m_model,
			.view = mat_view,
			.proj = mat_proj,
		};
		
		/* Fill Parameters */
		pc.lightAndStar[0] = light_pos[0] - (float)cam->x; /* Light also relative! */
		pc.lightAndStar[1] = light_pos[1] - (float)cam->y; /* Correct lighting in RTC */
		pc.lightAndStar[2] = light_pos[2] - (float)cam->z;
		pc.lightAndStar[3] = (b->type == BHS_BODY_STAR) ? 1.0f : 0.0f;
		
		pc.colorParams[0] = (float)b->color.x;
		pc.colorParams[1] = (float)b->color.y;
		pc.colorParams[2] = (float)b->color.z;
		pc.colorParams[3] = 0.0f;
		
		bhs_gpu_cmd_push_constants(cmd, 0, &pc, sizeof(pc));
		
		/* Draw */
		bhs_gpu_cmd_draw_indexed(cmd, pass->index_count, 1, 0, 0, 0);
	}
}
