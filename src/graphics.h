#ifndef NIKITU_GRAPHICS_H
#define NIKITU_GRAPHICS_H

#include <psptypes.h>

typedef struct
{
    u32 *data;
    int width;
    int height;
    int tex_width;
    int tex_height;
} Texture;

int graphics_init(void);
void graphics_shutdown(void);
void graphics_begin_frame(void);
void graphics_prepare_text(void);
void graphics_end_frame(void);

Texture *graphics_load_png(const char *path);
void graphics_free_texture(Texture *texture);

void graphics_draw_texture(Texture *texture,
                           float x, float y,
                           float w, float h);

void graphics_draw_rect(float x, float y,
                        float w, float h,
                        u32 color);

#endif
