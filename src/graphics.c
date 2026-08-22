#include "graphics.h"

#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspkernel.h>

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define BUFFER_WIDTH 512

static unsigned int __attribute__((aligned(16))) gu_list[262144];

static void *fbp0;
static void *fbp1;
static int graphics_ready = 0;

typedef struct
{
    float u, v;
    u32 color;
    float x, y, z;
} TextureVertex;

typedef struct
{
    u32 color;
    float x, y, z;
} ColorVertex;

static int next_power_of_two(int value)
{
    int result = 1;

    while (result < value)
        result <<= 1;

    return result;
}

int graphics_init(void)
{
    pspDebugScreenInit();
    pspDebugScreenEnableBackColor(0);
    pspDebugScreenSetTextColor(0xFFFFFFFF);

    sceGuInit();

    fbp0 = guGetStaticVramBuffer(
        BUFFER_WIDTH,
        SCREEN_HEIGHT,
        GU_PSM_8888
    );

    fbp1 = guGetStaticVramBuffer(
        BUFFER_WIDTH,
        SCREEN_HEIGHT,
        GU_PSM_8888
    );

    sceGuStart(GU_DIRECT, gu_list);

    sceGuDrawBuffer(
        GU_PSM_8888,
        fbp0,
        BUFFER_WIDTH
    );

    sceGuDispBuffer(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        fbp1,
        BUFFER_WIDTH
    );

    sceGuDepthBuffer(fbp0, 0);

    sceGuOffset(
        2048 - (SCREEN_WIDTH / 2),
        2048 - (SCREEN_HEIGHT / 2)
    );

    sceGuViewport(
        2048,
        2048,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    sceGuScissor(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuFrontFace(GU_CW);
    sceGuShadeModel(GU_FLAT);

    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(
        GU_ADD,
        GU_SRC_ALPHA,
        GU_ONE_MINUS_SRC_ALPHA,
        0,
        0
    );

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    graphics_ready = 1;

    return 0;
}

void graphics_shutdown(void)
{
    if (!graphics_ready)
        return;

    sceGuDisplay(GU_FALSE);
    sceGuTerm();

    graphics_ready = 0;
}

void graphics_begin_frame(void)
{
    if (!graphics_ready)
        return;

    sceGuStart(GU_DIRECT, gu_list);

    sceGuClearColor(0xFF000000);
    sceGuClear(GU_COLOR_BUFFER_BIT);

    sceGuDisable(GU_TEXTURE_2D);
}

void graphics_prepare_text(void)
{
    if (!graphics_ready)
        return;

    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

    pspDebugScreenSetOffset((int)fbp0);
}

void graphics_end_frame(void)
{
    if (!graphics_ready)
        return;

    sceDisplayWaitVblankStart();

    fbp0 = sceGuSwapBuffers();
}

Texture *graphics_load_png(const char *path)
{
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *rows;
    Texture *texture;
    int width;
    int height;
    int bit_depth;
    int color_type;
    int channels;
    int y;
    size_t bytes;

    fp = fopen(path, "rb");

    if (!fp)
        return NULL;

    {
        unsigned char signature[8];

        if (fread(signature, 1, 8, fp) != 8 ||
            png_sig_cmp(signature, 0, 8))
        {
            fclose(fp);
            return NULL;
        }
    }

    png_ptr = png_create_read_struct(
        PNG_LIBPNG_VER_STRING,
        NULL,
        NULL,
        NULL
    );

    if (!png_ptr)
    {
        fclose(fp);
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(
            &png_ptr,
            &info_ptr,
            NULL
        );
        fclose(fp);
        return NULL;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    width = (int)png_get_image_width(
        png_ptr,
        info_ptr
    );

    height = (int)png_get_image_height(
        png_ptr,
        info_ptr
    );

    bit_depth = png_get_bit_depth(
        png_ptr,
        info_ptr
    );

    color_type = png_get_color_type(
        png_ptr,
        info_ptr
    );

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY &&
        bit_depth < 8)
    {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }

    if (png_get_valid(
            png_ptr,
            info_ptr,
            PNG_INFO_tRNS))
    {
        png_set_tRNS_to_alpha(png_ptr);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        png_set_gray_to_rgb(png_ptr);
    }

    color_type = png_get_color_type(
        png_ptr,
        info_ptr
    );

    if (!(color_type & PNG_COLOR_MASK_ALPHA))
        png_set_filler(
            png_ptr,
            0xFF,
            PNG_FILLER_AFTER
        );

    png_read_update_info(png_ptr, info_ptr);

    channels = png_get_channels(
        png_ptr,
        info_ptr
    );

    if (channels != 4 ||
        width <= 0 ||
        height <= 0 ||
        width > 480 ||
        height > 512)
    {
        png_destroy_read_struct(
            &png_ptr,
            &info_ptr,
            NULL
        );
        fclose(fp);
        return NULL;
    }

    texture = calloc(1, sizeof(Texture));

    if (!texture)
    {
        png_destroy_read_struct(
            &png_ptr,
            &info_ptr,
            NULL
        );
        fclose(fp);
        return NULL;
    }

    texture->width = width;
    texture->height = height;
    texture->tex_width = next_power_of_two(width);
    texture->tex_height = next_power_of_two(height);

    if (texture->tex_width > 512 ||
        texture->tex_height > 512)
    {
        free(texture);

        png_destroy_read_struct(
            &png_ptr,
            &info_ptr,
            NULL
        );

        fclose(fp);
        return NULL;
    }

    bytes =
        (size_t)texture->tex_width *
        (size_t)texture->tex_height *
        4;

    texture->data = memalign(16, bytes);

    if (!texture->data)
    {
        free(texture);

        png_destroy_read_struct(
            &png_ptr,
            &info_ptr,
            NULL
        );

        fclose(fp);
        return NULL;
    }

    memset(texture->data, 0, bytes);

    rows = malloc(
        sizeof(png_bytep) * height
    );

    if (!rows)
    {
        free(texture->data);
        free(texture);

        png_destroy_read_struct(
            &png_ptr,
            &info_ptr,
            NULL
        );

        fclose(fp);
        return NULL;
    }

    for (y = 0; y < height; y++)
    {
        rows[y] =
            (png_bytep)texture->data +
            ((size_t)y *
             (size_t)texture->tex_width *
             4);
    }

    png_read_image(
        png_ptr,
        rows
    );

    png_read_end(
        png_ptr,
        NULL
    );

    free(rows);

    png_destroy_read_struct(
        &png_ptr,
        &info_ptr,
        NULL
    );

    fclose(fp);

    sceKernelDcacheWritebackInvalidateAll();

    return texture;
}

void graphics_free_texture(Texture *texture)
{
    if (!texture)
        return;

    if (texture->data)
        free(texture->data);

    free(texture);
}

void graphics_draw_texture(Texture *texture,
                           float x, float y,
                           float w, float h)
{
    TextureVertex *vertices;

    if (!texture || !texture->data)
        return;

    vertices =
        (TextureVertex *)sceGuGetMemory(
            2 * sizeof(TextureVertex)
        );

    vertices[0].u = 0.0f;
    vertices[0].v = 0.0f;
    vertices[0].color = 0xFFFFFFFF;
    vertices[0].x = x;
    vertices[0].y = y;
    vertices[0].z = 0.0f;

    vertices[1].u = (float)texture->width;
    vertices[1].v = (float)texture->height;
    vertices[1].color = 0xFFFFFFFF;
    vertices[1].x = x + w;
    vertices[1].y = y + h;
    vertices[1].z = 0.0f;

    sceGuEnable(GU_TEXTURE_2D);

    sceGuTexMode(
        GU_PSM_8888,
        0,
        0,
        GU_FALSE
    );

    sceGuTexFunc(
        GU_TFX_REPLACE,
        GU_TCC_RGBA
    );

    sceGuTexFilter(
        GU_NEAREST,
        GU_NEAREST
    );

    sceGuTexWrap(
        GU_CLAMP,
        GU_CLAMP
    );

    sceGuTexImage(
        0,
        texture->tex_width,
        texture->tex_height,
        texture->tex_width,
        texture->data
    );

    sceGuDrawArray(
        GU_SPRITES,
        GU_TEXTURE_32BITF |
        GU_COLOR_8888 |
        GU_VERTEX_32BITF |
        GU_TRANSFORM_2D,
        2,
        NULL,
        vertices
    );

    sceGuDisable(GU_TEXTURE_2D);
}

void graphics_draw_rect(float x, float y,
                        float w, float h,
                        u32 color)
{
    ColorVertex *vertices;

    vertices =
        (ColorVertex *)sceGuGetMemory(
            2 * sizeof(ColorVertex)
        );

    vertices[0].color = color;
    vertices[0].x = x;
    vertices[0].y = y;
    vertices[0].z = 0.0f;

    vertices[1].color = color;
    vertices[1].x = x + w;
    vertices[1].y = y + h;
    vertices[1].z = 0.0f;

    sceGuDisable(GU_TEXTURE_2D);

    sceGuDrawArray(
        GU_SPRITES,
        GU_COLOR_8888 |
        GU_VERTEX_32BITF |
        GU_TRANSFORM_2D,
        2,
        NULL,
        vertices
    );
}
