#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>

#include <stdlib.h>
#include <time.h>

#include "graphics.h"

PSP_MODULE_INFO("Nikitu v0.0.7", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

#define FIELD_WIDTH 40
#define FIELD_HEIGHT 20

#define LEFT 0
#define RIGHT 39
#define TOP 0
#define BOTTOM 19

#define CELL_W 12
#define CELL_H 12
#define FIELD_Y 28

#define MAX_MISSILES 5

#define METEOR_W 120
#define METEOR_H 68

typedef struct
{
    int x;
    int y;
    int direction;
    int speed;
    int frame;
    int active;
} Missile;

typedef struct
{
    int x;
    int y;
    int direction;
    int speed;
    int frame;
    int active;
} Meteor;

static Texture *background = NULL;
static Texture *player_texture = NULL;

static int exit_callback(int a, int b, void *c)
{
    sceKernelExitGame();
    return 0;
}

static int callback_thread(SceSize args, void *argp)
{
    int cbid = sceKernelCreateCallback(
        "Exit Callback",
        exit_callback,
        NULL
    );

    if (cbid >= 0)
        sceKernelRegisterExitCallback(cbid);

    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void)
{
    int thid = sceKernelCreateThread(
        "CallbackThread",
        callback_thread,
        0x11,
        0xFA0,
        0,
        NULL
    );

    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);
}

static int clamp_x(int x)
{
    if (x < LEFT)
        return LEFT;

    if (x > RIGHT)
        return RIGHT;

    return x;
}

static int clamp_y(int y)
{
    if (y < TOP)
        return TOP;

    if (y > BOTTOM)
        return BOTTOM;

    return y;
}

static void spawn_missile(Missile *m, int difficulty)
{
    int side = rand() % 4;

    m->active = 1;
    m->frame = 0;
    m->direction = side;

    if (side == 0 || side == 1)
        m->y = 4 + rand() % 12;
    else
        m->x = 10 + rand() % 20;

    if (side == 0)
        m->x = LEFT;
    else if (side == 1)
        m->x = RIGHT;
    else if (side == 2)
        m->y = TOP;
    else
        m->y = BOTTOM;

    m->speed = 4 - difficulty;

    if (m->speed < 1)
        m->speed = 1;
}

static void move_missile(Missile *m)
{
    if (!m->active)
        return;

    m->frame++;

    if (m->frame < m->speed)
        return;

    m->frame = 0;

    if (m->direction == 0)
        m->x++;
    else if (m->direction == 1)
        m->x--;
    else if (m->direction == 2)
        m->y++;
    else
        m->y--;

    if (m->x < LEFT ||
        m->x > RIGHT ||
        m->y < TOP ||
        m->y > BOTTOM)
    {
        m->active = 0;
    }
}

static int hit_player(
    Missile *m,
    int px,
    int py
)
{
    return m->active &&
           m->x == px &&
           m->y == py;
}

static void spawn_meteor(
    Meteor *m,
    int difficulty
)
{
    int side = rand() % 4;

    m->active = 1;
    m->frame = 0;
    m->direction = side;

    m->speed = 2 - difficulty / 2;

    if (m->speed < 1)
        m->speed = 1;

    if (side == 0)
    {
        m->x = -METEOR_W;
        m->y = 50 + rand() % 100;
    }
    else if (side == 1)
    {
        m->x = SCREEN_WIDTH;
        m->y = 50 + rand() % 100;
    }
    else if (side == 2)
    {
        m->x = 80 + rand() % 300;
        m->y = -METEOR_H;
    }
    else
    {
        m->x = 80 + rand() % 300;
        m->y = SCREEN_HEIGHT;
    }
}

static void move_meteor(Meteor *m)
{
    if (!m->active)
        return;

    m->frame++;

    if (m->frame < m->speed)
        return;

    m->frame = 0;

    if (m->direction == 0)
        m->x += 3;
    else if (m->direction == 1)
        m->x -= 3;
    else if (m->direction == 2)
        m->y += 3;
    else
        m->y -= 3;

    if (m->x > SCREEN_WIDTH ||
        m->x + METEOR_W < 0 ||
        m->y > SCREEN_HEIGHT ||
        m->y + METEOR_H < 0)
    {
        m->active = 0;
    }
}

static int meteor_hits_player(
    Meteor *m,
    int px,
    int py
)
{
    int sx;
    int sy;

    if (!m->active)
        return 0;

    sx = px * CELL_W + 6;
    sy = FIELD_Y + py * CELL_H + 6;

    return sx >= m->x &&
           sx <= m->x + METEOR_W &&
           sy >= m->y &&
           sy <= m->y + METEOR_H;
}

static void draw_world(
    int player_x,
    int player_y,
    Missile *missiles,
    int missile_count,
    int obstacle_active,
    int obstacle_x,
    int obstacle_y,
    Meteor *meteor,
    int show_meteor
)
{
    int i;

    graphics_begin_frame();

    if (background)
    {
        graphics_draw_texture(
            background,
            0.0f,
            0.0f,
            480.0f,
            272.0f
        );
    }

    /*
       Орешники: визуально крупнее старого "O",
       но всё ещё читаются поверх фона.
    */
    for (i = 0; i < missile_count; i++)
    {
        if (!missiles[i].active)
            continue;

        graphics_draw_rect(
            missiles[i].x * CELL_W + 2,
            FIELD_Y + missiles[i].y * CELL_H + 2,
            8,
            8,
            0xFF202020
        );

        graphics_draw_rect(
            missiles[i].x * CELL_W + 4,
            FIELD_Y + missiles[i].y * CELL_H + 4,
            4,
            4,
            0xFFFF5050
        );
    }

    if (obstacle_active)
    {
        graphics_draw_rect(
            obstacle_x * CELL_W + 2,
            FIELD_Y + obstacle_y * CELL_H + 2,
            8,
            8,
            0xFF00D080
        );
    }

    if (show_meteor && meteor && meteor->active)
    {
        graphics_draw_rect(
            (float)meteor->x,
            (float)meteor->y,
            (float)METEOR_W,
            (float)METEOR_H,
            0xCC4A2A20
        );

        graphics_draw_rect(
            (float)meteor->x + 12.0f,
            (float)meteor->y + 10.0f,
            (float)METEOR_W - 24.0f,
            (float)METEOR_H - 20.0f,
            0xFF8A4630
        );
    }

    if (player_texture)
    {
        graphics_draw_texture(
            player_texture,
            player_x * CELL_W - 18,
            FIELD_Y + player_y * CELL_H - 26,
            48.0f,
            64.0f
        );
    }
}

static void print_hud(
    int score,
    int best,
    const char *title
)
{
    graphics_prepare_text();

    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf(
        "%s  SCORE:%d  BEST:%d",
        title,
        score,
        best
    );
}

static void end_text_frame(void)
{
    graphics_end_frame();
}

static void wait_start(SceCtrlData *pad)
{
    while (1)
    {
        graphics_begin_frame();

        if (background)
        {
            graphics_draw_texture(
                background,
                0,
                0,
                480,
                272
            );
        }

        graphics_prepare_text();

    pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf(
            "PRESS START TO RETURN"
        );

        graphics_end_frame();

        sceCtrlReadBufferPositive(pad, 1);

        if (pad->Buttons & PSP_CTRL_START)
            return;
    }
}

static void game_over(
    SceCtrlData *pad,
    int score,
    int best
)
{
    while (1)
    {
        graphics_begin_frame();

        if (background)
        {
            graphics_draw_texture(
                background,
                0,
                0,
                480,
                272
            );
        }

        graphics_prepare_text();

    pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf("GAME OVER\n\n");
        pspDebugScreenPrintf(
            "THE ORESHNIK GOT YOU!\n\n"
        );
        pspDebugScreenPrintf(
            "SCORE: %d\nBEST: %d\n\n",
            score,
            best
        );
        pspDebugScreenPrintf(
            "PRESS START TO RETURN"
        );

        graphics_end_frame();

        sceCtrlReadBufferPositive(pad, 1);

        if (pad->Buttons & PSP_CTRL_START)
            return;
    }
}

static void victory(SceCtrlData *pad)
{
    while (1)
    {
        graphics_begin_frame();

        if (background)
        {
            graphics_draw_texture(
                background,
                0,
                0,
                480,
                272
            );
        }

        graphics_prepare_text();

    pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf(
            "YOU WIN!\n\n"
            "67 ORESHNIKS SURVIVED!\n\n"
            "TWO METEORITES SURVIVED!\n\n"
            "BUTCHER\n"
            "(THE CRAZY FART SOUND IS NOT\n"
            "ADDED YET)\n\n"
            "PRESS START TO RETURN"
        );

        graphics_end_frame();

        sceCtrlReadBufferPositive(pad, 1);

        if (pad->Buttons & PSP_CTRL_START)
            return;
    }
}

static void final_wave(
    SceCtrlData *pad,
    int px,
    int py,
    int difficulty
)
{
    Meteor meteor;
    int meteor_number = 1;

    spawn_meteor(&meteor, difficulty);

    while (1)
    {
        graphics_begin_frame();

        if (background)
        {
            graphics_draw_texture(
                background,
                0,
                0,
                480,
                272
            );
        }

        if (meteor.active)
        {
            graphics_draw_rect(
                (float)meteor.x,
                (float)meteor.y,
                (float)METEOR_W,
                (float)METEOR_H,
                0xCC4A2A20
            );

            graphics_draw_rect(
                (float)meteor.x + 14.0f,
                (float)meteor.y + 12.0f,
                (float)METEOR_W - 28.0f,
                (float)METEOR_H - 24.0f,
                0xFF9C5035
            );
        }

        if (player_texture)
        {
            graphics_draw_texture(
                player_texture,
                px * CELL_W - 18,
                FIELD_Y + py * CELL_H - 26,
                48,
                64
            );
        }

        graphics_prepare_text();

    pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf(
            "FINAL METEOR %d/2",
            meteor_number
        );

        graphics_end_frame();

        sceCtrlReadBufferPositive(pad, 1);

        if (pad->Buttons & PSP_CTRL_LEFT)
            px = clamp_x(px - 1);

        if (pad->Buttons & PSP_CTRL_RIGHT)
            px = clamp_x(px + 1);

        if (pad->Buttons & PSP_CTRL_UP)
            py = clamp_y(py - 1);

        if (pad->Buttons & PSP_CTRL_DOWN)
            py = clamp_y(py + 1);

        if (pad->Buttons & PSP_CTRL_START)
            return;

        move_meteor(&meteor);

        if (meteor_hits_player(&meteor, px, py))
        {
            game_over(pad, 67, 67);
            return;
        }

        if (!meteor.active)
        {
            if (meteor_number == 1)
            {
                meteor_number = 2;
                spawn_meteor(
                    &meteor,
                    difficulty
                );
            }
            else
            {
                victory(pad);
                return;
            }
        }
    }
}

static void game(
    SceCtrlData *pad,
    int difficulty,
    int *best
)
{
    Missile missiles[MAX_MISSILES];

    int player_x = 20;
    int player_y = 10;
    int missile_count = 2 + rand() % 2;

    int score = 0;

    int obstacle_active = 0;
    int obstacle_x = 0;
    int obstacle_y = 0;
    int obstacle_timer = 0;
    int next_obstacle = 80;

    int i;

    for (i = 0; i < MAX_MISSILES; i++)
        missiles[i].active = 0;

    for (i = 0; i < missile_count; i++)
        spawn_missile(
            &missiles[i],
            difficulty
        );

    while (1)
    {
        draw_world(
            player_x,
            player_y,
            missiles,
            missile_count,
            obstacle_active,
            obstacle_x,
            obstacle_y,
            NULL,
            0
        );

        print_hud(
            score,
            *best,
            "NIKITU v0.0.7"
        );

        pspDebugScreenSetXY(0, 1);
        pspDebugScreenPrintf(
            "D-PAD MOVE   START MENU"
        );

        end_text_frame();

        sceCtrlReadBufferPositive(pad, 1);

        if (pad->Buttons & PSP_CTRL_LEFT)
            player_x = clamp_x(player_x - 1);

        if (pad->Buttons & PSP_CTRL_RIGHT)
            player_x = clamp_x(player_x + 1);

        if (pad->Buttons & PSP_CTRL_UP)
            player_y = clamp_y(player_y - 1);

        if (pad->Buttons & PSP_CTRL_DOWN)
            player_y = clamp_y(player_y + 1);

        if (pad->Buttons & PSP_CTRL_START)
            return;

        for (i = 0; i < missile_count; i++)
            move_missile(&missiles[i]);

        for (i = 0; i < missile_count; i++)
        {
            if (!missiles[i].active)
            {
                score++;

                if (score > *best)
                    *best = score;

                if (score >= 67)
                {
                    final_wave(
                        pad,
                        player_x,
                        player_y,
                        difficulty
                    );
                    return;
                }

                spawn_missile(
                    &missiles[i],
                    difficulty
                );
            }
        }

        for (i = 0; i < missile_count; i++)
        {
            if (hit_player(
                    &missiles[i],
                    player_x,
                    player_y))
            {
                game_over(
                    pad,
                    score,
                    *best
                );
                return;
            }
        }

        if (!obstacle_active)
        {
            next_obstacle--;

            if (next_obstacle <= 0)
            {
                obstacle_active = 1;

                obstacle_x =
                    rand() % FIELD_WIDTH;

                obstacle_y =
                    2 + rand() % 16;

                obstacle_timer =
                    50 + rand() % (70 + difficulty * 20);
            }
        }
        else
        {
            obstacle_timer--;

            if (obstacle_timer <= 0)
            {
                obstacle_active = 0;

                next_obstacle =
                    70 + rand() % 60;
            }
        }

        if (obstacle_active &&
            player_x == obstacle_x &&
            player_y == obstacle_y)
        {
            game_over(
                pad,
                score,
                *best
            );
            return;
        }

        if (score > 0 &&
            score % 10 == 0 &&
            missile_count < 2 + difficulty &&
            missile_count < MAX_MISSILES)
        {
            int active_count = 0;

            for (i = 0; i < missile_count; i++)
            {
                if (missiles[i].active)
                    active_count++;
            }

            if (active_count == missile_count)
            {
                spawn_missile(
                    &missiles[missile_count],
                    difficulty
                );

                missile_count++;
            }
        }
    }
}

static void difficulty_menu(
    SceCtrlData *pad,
    int *difficulty
)
{
    int selected = *difficulty;

    while (1)
    {
        graphics_begin_frame();

        if (background)
        {
            graphics_draw_texture(
                background,
                0, 0, 480, 272
            );
        }

        graphics_prepare_text();

    pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf(
            "DIFFICULTY\n\n"
        );

        pspDebugScreenPrintf(
            "%s EASY\n",
            selected == 0 ? ">" : " "
        );

        pspDebugScreenPrintf(
            "%s NORMAL\n",
            selected == 1 ? ">" : " "
        );

        pspDebugScreenPrintf(
            "%s HARD\n",
            selected == 2 ? ">" : " "
        );

        pspDebugScreenPrintf(
            "%s INSANE\n\n",
            selected == 3 ? ">" : " "
        );

        pspDebugScreenPrintf(
            "X SELECT   START BACK"
        );

        graphics_end_frame();

        sceCtrlReadBufferPositive(pad, 1);

        if (pad->Buttons & PSP_CTRL_UP)
        {
            selected--;

            if (selected < 0)
                selected = 3;
        }

        if (pad->Buttons & PSP_CTRL_DOWN)
        {
            selected++;

            if (selected > 3)
                selected = 0;
        }

        if (pad->Buttons & PSP_CTRL_CROSS)
        {
            *difficulty = selected;
            return;
        }

        if (pad->Buttons & PSP_CTRL_START)
            return;
    }
}

int main(void)
{
    SceCtrlData pad;

    int selected = 0;
    int difficulty = 1;
    int best = 0;

    setup_callbacks();

    if (graphics_init() < 0)
        sceKernelExitGame();

    srand((unsigned int)time(NULL));

    background = graphics_load_png(
        "assets/verhniy_novgorod_480x272.png"
    );

    player_texture = graphics_load_png(
        "assets/player_sprite_48x64.png"
    );

    while (1)
    {
        graphics_begin_frame();

        if (background)
        {
            graphics_draw_texture(
                background,
                0, 0, 480, 272
            );
        }

        graphics_prepare_text();

    pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf(
            "NIKITU v0.0.7\n\n"
        );

        pspDebugScreenPrintf(
            "%s START GAME\n",
            selected == 0 ? ">" : " "
        );

        pspDebugScreenPrintf(
            "%s DIFFICULTY\n",
            selected == 1 ? ">" : " "
        );

        pspDebugScreenPrintf(
            "%s OPTIONS\n",
            selected == 2 ? ">" : " "
        );

        pspDebugScreenPrintf(
            "%s EXIT\n\n",
            selected == 3 ? ">" : " "
        );

        pspDebugScreenPrintf(
            "UP/DOWN SELECT   X CONFIRM"
        );

        graphics_end_frame();

        sceCtrlReadBufferPositive(&pad, 1);

        if (pad.Buttons & PSP_CTRL_UP)
        {
            selected--;

            if (selected < 0)
                selected = 3;
        }

        if (pad.Buttons & PSP_CTRL_DOWN)
        {
            selected++;

            if (selected > 3)
                selected = 0;
        }

        if (pad.Buttons & PSP_CTRL_CROSS)
        {
            if (selected == 0)
            {
                game(
                    &pad,
                    difficulty,
                    &best
                );
            }
            else if (selected == 1)
            {
                difficulty_menu(
                    &pad,
                    &difficulty
                );
            }
            else if (selected == 2)
            {
                wait_start(&pad);
            }
            else
            {
                graphics_shutdown();
                sceKernelExitGame();
            }
        }
    }

    return 0;
}
