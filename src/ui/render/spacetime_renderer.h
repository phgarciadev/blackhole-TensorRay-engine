#ifndef BHS_CMD_UI_RENDER_SPACETIME_RENDERER_H
#define BHS_CMD_UI_RENDER_SPACETIME_RENDERER_H

#include "src/ui/camera/camera.h"
#include "engine/scene/scene.h"
#include "framework/ui/lib.h"

/**
 * @brief Renderiza a malha do espaço-tempo
 * @param ctx Contexto de desenho
 * @param scene Cena contendo a simulação
 * @param cam Câmera para projeção
 * @param width Largura da tela
 * @param height Altura da tela
 */
void bhs_spacetime_renderer_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
				 const bhs_camera_t *cam, int width, int height,
				 const void *assets);

#endif /* BHS_CMD_UI_RENDER_SPACETIME_RENDERER_H */
