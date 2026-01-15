/**
 * @file image_loader.c
 * @brief Carregador de Imagens (Wrapper para stb_image)
 *
 * Implementação "Kernel-style":
 * - Falha rápido
 * - Sem alocações desnecessárias
 * - Força RGBA para performance na GPU (alinhamento)
 */

#include "image_loader.h"
#include <stdio.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_LINEAR /* Não precisamos de HDR/Linear por enquanto */
#define STBI_NO_HDR
#define STBI_ONLY_PNG /* Otimização: Só compila decoder PNG (user provided  \
                         PNG) */
/* Se precisar de JPG depois, remove STBI_ONLY_PNG ou adiciona STBI_ONLY_JPEG */
#include "stb_image.h"
#pragma GCC diagnostic pop

bhs_image_t bhs_image_load(const char *path) {
  bhs_image_t img = {0};

  if (!path)
    return img;

  /* Força 4 canais (RGBA) para evitar desalinhamento na GPU */
  int desired_channels = 4;

  img.data =
      stbi_load(path, &img.width, &img.height, &img.channels, desired_channels);

  if (!img.data) {
    fprintf(stderr, "[LOADER] Erro ao carregar textura: %s\n", path);
    /* Retorna struct zerado em erro */
    return img;
  }

  img.channels = desired_channels;

  /* Log discreto */
  // printf("[LOADER] Loaded %s (%dx%d)\n", path, img.width, img.height);

  return img;
}

void bhs_image_free(bhs_image_t img) {
  if (img.data) {
    stbi_image_free(img.data);
  }
}
