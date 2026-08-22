#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <time.h>

PSP_MODULE_INFO("Nikitu v0.0.6", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

#define FIELD_WIDTH 40
#define FIELD_HEIGHT 20

#define LEFT 1
#define RIGHT 38
#define TOP 1
#define BOTTOM 18

#define MAX_MISSILES 5

#define METEOR_WIDTH 10
#define METEOR_HEIGHT 5

#define FIELD_X 0

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

/* -------------------------------------------------- */

int exit_callback(int a, int b, void *c)
{
    sceKernelExitGame();
    return 0;
}

int callback_thread(SceSize args, void *argp)
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

void setup_callbacks(void)
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

/* -------------------------------------------------- */
/* ОРЕШНИКИ                                           */
/* -------------------------------------------------- */

void spawn_missile(Missile *m, int difficulty)
{
    int central;

    m->active = 1;
    m->frame = 0;

    m->direction = rand() % 4;

    central = rand() % 100;

    if (m->direction == 0 || m->direction == 1)
    {
        if (central < 70)
            m->y = 6 + rand() % 9;
        else
            m->y = TOP + rand() % (BOTTOM - TOP + 1);
    }
    else
    {
        if (central < 70)
            m->x = 15 + rand() % 11;
        else
            m->x = LEFT + rand() % (RIGHT - LEFT + 1);
    }

    if (m->direction == 0)
        m->x = LEFT;
    else if (m->direction == 1)
        m->x = RIGHT;
    else if (m->direction == 2)
        m->y = TOP;
    else
        m->y = BOTTOM;

    m->speed = 3 - difficulty;

    if (m->speed < 1)
        m->speed = 1;
}

void move_missile(Missile *m)
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

    if (
        m->x < LEFT ||
        m->x > RIGHT ||
        m->y < TOP ||
        m->y > BOTTOM
    )
    {
        m->active = 0;
    }
}

int hit_player(Missile *m, int px, int py)
{
    if (!m->active)
        return 0;

    return m->x == px && m->y == py;
}

/* -------------------------------------------------- */
/* ЭКРАНЫ                                              */
/* -------------------------------------------------- */

void wait_start(SceCtrlData *pad)
{
    while (1)
    {
        sceCtrlReadBufferPositive(pad, 1);

        if (pad->Buttons & PSP_CTRL_START)
            return;

        sceDisplayWaitVblankStart();
    }
}

void game_over(SceCtrlData *pad, int score, int best)
{
    pspDebugScreenClear();

    pspDebugScreenPrintf("\n\n");
    pspDebugScreenPrintf("========================================\n");
    pspDebugScreenPrintf("              GAME OVER\n");
    pspDebugScreenPrintf("========================================\n\n");

    pspDebugScreenPrintf("THE ORESHNIK GOT YOU!\n\n");

    pspDebugScreenPrintf("SCORE: %d\n", score);
    pspDebugScreenPrintf("BEST : %d\n\n", best);

    pspDebugScreenPrintf("PRESS START TO RETURN");

    wait_start(pad);
}

void victory(SceCtrlData *pad)
{
    pspDebugScreenClear();

    pspDebugScreenPrintf("\n\n");
    pspDebugScreenPrintf("========================================\n");
    pspDebugScreenPrintf("              YOU WIN!\n");
    pspDebugScreenPrintf("========================================\n\n");

    pspDebugScreenPrintf("67 ORESHNIKS SURVIVED!\n\n");
    pspDebugScreenPrintf("TWO METEORITES SURVIVED!\n\n");

    pspDebugScreenPrintf("BUTCHER\n");
    pspDebugScreenPrintf("(Тут должен быть звук дичайшего пердежа\n");
    pspDebugScreenPrintf("но пока я его не добавил)\n\n");

    pspDebugScreenPrintf("PRESS START TO RETURN");

    wait_start(pad);
}

/* -------------------------------------------------- */
/* МЕТЕОРИТЫ                                           */
/* -------------------------------------------------- */

void spawn_meteor(Meteor *m, int difficulty)
{
    int side = rand() % 4;

    m->active = 1;
    m->frame = 0;

    /*
       0 = слева направо
       1 = справа налево
       2 = сверху вниз
       3 = снизу вверх
    */

    m->direction = side;

    /*
       Скорость метеорита.
       Чем выше сложность — тем быстрее.
    */

    m->speed = 2 - (difficulty / 2);

    if (m->speed < 1)
        m->speed = 1;

    if (side == 0)
    {
        m->x = LEFT - METEOR_WIDTH;
        m->y = TOP + rand() % (BOTTOM - TOP - METEOR_HEIGHT + 2);
    }
    else if (side == 1)
    {
        m->x = RIGHT + 1;
        m->y = TOP + rand() % (BOTTOM - TOP - METEOR_HEIGHT + 2);
    }
    else if (side == 2)
    {
        m->x = LEFT + rand() % (RIGHT - LEFT - METEOR_WIDTH + 2);
        m->y = TOP - METEOR_HEIGHT;
    }
    else
    {
        m->x = LEFT + rand() % (RIGHT - LEFT - METEOR_WIDTH + 2);
        m->y = BOTTOM + 1;
    }
}

void move_meteor(Meteor *m)
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

    /*
       Проверяем, полностью ли метеорит
       ушёл за пределы поля.
    */

    if (m->direction == 0)
    {
        if (m->x > RIGHT)
            m->active = 0;
    }
    else if (m->direction == 1)
    {
        if (m->x + METEOR_WIDTH < LEFT)
            m->active = 0;
    }
    else if (m->direction == 2)
    {
        if (m->y > BOTTOM)
            m->active = 0;
    }
    else
    {
        if (m->y + METEOR_HEIGHT < TOP)
            m->active = 0;
    }
}

int meteor_hits_player(Meteor *m, int px, int py)
{
    int meteorLeft;
    int meteorRight;
    int meteorTop;
    int meteorBottom;

    if (!m->active)
        return 0;

    meteorLeft = m->x;
    meteorRight = m->x + METEOR_WIDTH - 1;

    meteorTop = m->y;
    meteorBottom = m->y + METEOR_HEIGHT - 1;

    if (px >= meteorLeft &&
        px <= meteorRight &&
        py >= meteorTop &&
        py <= meteorBottom)
    {
        return 1;
    }

    return 0;
}

/* -------------------------------------------------- */
/* ФИНАЛЬНАЯ ВОЛНА                                    */
/* -------------------------------------------------- */

void draw_meteor(
    Meteor *m,
    int x,
    int y
)
{
    int mx;
    int my;

    if (!m->active)
        return;

    mx = x - m->x;
    my = y - m->y;

    if (
        mx >= 0 &&
        mx < METEOR_WIDTH &&
        my >= 0 &&
        my < METEOR_HEIGHT
    )
    {
        /*
           Разные символы создают простую
           форму огромного метеорита.
        */

        if (mx == 0 ||
            mx == METEOR_WIDTH - 1 ||
            my == 0 ||
            my == METEOR_HEIGHT - 1)
        {
            pspDebugScreenPrintf("#");
        }
        else if (
            mx == 1 ||
            mx == METEOR_WIDTH - 2 ||
            my == 1 ||
            my == METEOR_HEIGHT - 2)
        {
            pspDebugScreenPrintf("O");
        }
        else
        {
            pspDebugScreenPrintf("@");
        }
    }
    else
    {
        pspDebugScreenPrintf(" ");
    }
}

void final_wave(
    SceCtrlData *pad,
    int px,
    int py,
    int difficulty
)
{
    Meteor meteor;

    int meteorNumber = 1;

    meteor.active = 0;

    spawn_meteor(&meteor, difficulty);

    while (1)
    {
        pspDebugScreenClear();

        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf(
            "          FINAL METEOR %d/2\n",
            meteorNumber
        );

        pspDebugScreenPrintf(
            "========================================\n\n"
        );

        for (int y = 0; y < FIELD_HEIGHT; y++)
        {
            pspDebugScreenPrintf(" ");

            for (int x = 0; x < FIELD_WIDTH; x++)
            {
                int printed = 0;

                /*
                   Игрок.
                */

                if (x == px && y == py)
                {
                    pspDebugScreenPrintf("@");
                    printed = 1;
                }

                /*
                   Метеорит.
                */

                if (!printed &&
                    meteor.active &&
                    x >= meteor.x &&
                    x < meteor.x + METEOR_WIDTH &&
                    y >= meteor.y &&
                    y < meteor.y + METEOR_HEIGHT)
                {
                    draw_meteor(&meteor, x, y);
                    printed = 1;
                }

                /*
                   Рамка.
                */

                if (!printed)
                {
                    if (
                        x == 0 ||
                        x == FIELD_WIDTH - 1 ||
                        y == 0 ||
                        y == FIELD_HEIGHT - 1
                    )
                    {
                        pspDebugScreenPrintf("#");
                    }
                    else
                    {
                        pspDebugScreenPrintf(" ");
                    }
                }
            }

            pspDebugScreenPrintf("\n");
        }

        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("D-PAD - DODGE\n");

        sceCtrlReadBufferPositive(pad, 1);

        if (pad->Buttons & PSP_CTRL_LEFT)
        {
            if (px > LEFT)
                px--;
        }

        if (pad->Buttons & PSP_CTRL_RIGHT)
        {
            if (px < RIGHT)
                px++;
        }

        if (pad->Buttons & PSP_CTRL_UP)
        {
            if (py > TOP)
                py--;
        }

        if (pad->Buttons & PSP_CTRL_DOWN)
        {
            if (py < BOTTOM)
                py++;
        }

        if (pad->Buttons & PSP_CTRL_START)
            return;

        move_meteor(&meteor);

        if (meteor_hits_player(&meteor, px, py))
        {
            game_over(pad, 67, 67);
            return;
        }

        /*
           Первый метеорит ушёл.
           Запускаем второй.
        */

        if (!meteor.active)
        {
            if (meteorNumber == 1)
            {
                meteorNumber = 2;

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

        sceDisplayWaitVblankStart();
    }
}

/* -------------------------------------------------- */
/* ОСНОВНАЯ ИГРА                                      */
/* -------------------------------------------------- */

void game(
    SceCtrlData *pad,
    int difficulty,
    int *best
)
{
    Missile missiles[MAX_MISSILES];

    int playerX = 20;
    int playerY = 10;

    int missileCount;
    int score = 0;

    int obstacleActive = 0;
    int obstacleX = 0;
    int obstacleY = 0;
    int obstacleTimer = 0;
    int nextObstacle = 80;

    int i;

    for (i = 0; i < MAX_MISSILES; i++)
        missiles[i].active = 0;

    missileCount = 2 + rand() % 2;

    for (i = 0; i < missileCount; i++)
        spawn_missile(
            &missiles[i],
            difficulty
        );

    while (1)
    {
        pspDebugScreenClear();

        pspDebugScreenPrintf("\n");

        pspDebugScreenPrintf(
            "             NIKITU v0.0.6\n"
        );

        pspDebugScreenPrintf(
            "========================================\n"
        );

        pspDebugScreenPrintf(
            "SCORE: %d     BEST: %d\n\n",
            score,
            *best
        );

        for (int y = 0; y < FIELD_HEIGHT; y++)
        {
            /*
               Центрирование поля.
            */

            pspDebugScreenPrintf(" ");

            for (int x = 0; x < FIELD_WIDTH; x++)
            {
                int printed = 0;

                if (
                    x == playerX &&
                    y == playerY
                )
                {
                    pspDebugScreenPrintf("@");
                    printed = 1;
                }

                for (i = 0; i < missileCount; i++)
                {
                    if (
                        !printed &&
                        missiles[i].active &&
                        missiles[i].x == x &&
                        missiles[i].y == y
                    )
                    {
                        pspDebugScreenPrintf("O");
                        printed = 1;
                    }
                }

                if (
                    !printed &&
                    obstacleActive &&
                    x == obstacleX &&
                    y == obstacleY
                )
                {
                    pspDebugScreenPrintf("X");
                    printed = 1;
                }

                if (!printed)
                {
                    if (
                        x == 0 ||
                        x == FIELD_WIDTH - 1 ||
                        y == 0 ||
                        y == FIELD_HEIGHT - 1
                    )
                    {
                        pspDebugScreenPrintf("#");
                    }
                    else
                    {
                        pspDebugScreenPrintf(" ");
                    }
                }
            }

            pspDebugScreenPrintf("\n");
        }

        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("D-PAD - MOVE\n");
        pspDebugScreenPrintf("START - MENU\n");

        sceCtrlReadBufferPositive(pad, 1);

        if (pad->Buttons & PSP_CTRL_LEFT)
        {
            if (playerX > LEFT)
                playerX--;
        }

        if (pad->Buttons & PSP_CTRL_RIGHT)
        {
            if (playerX < RIGHT)
                playerX++;
        }

        if (pad->Buttons & PSP_CTRL_UP)
        {
            if (playerY > TOP)
                playerY--;
        }

        if (pad->Buttons & PSP_CTRL_DOWN)
        {
            if (playerY < BOTTOM)
                playerY++;
        }

        if (pad->Buttons & PSP_CTRL_START)
            return;

        /*
           Двигаем Орешники.
        */

        for (i = 0; i < missileCount; i++)
            move_missile(&missiles[i]);

        /*
           Пережитые Орешники.
        */

        for (i = 0; i < missileCount; i++)
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
                        playerX,
                        playerY,
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

        /*
           Столкновение с Орешником.
        */

        for (i = 0; i < missileCount; i++)
        {
            if (
                hit_player(
                    &missiles[i],
                    playerX,
                    playerY
                )
            )
            {
                game_over(
                    pad,
                    score,
                    *best
                );

                return;
            }
        }

        /*
           Появление препятствия.
        */

        if (!obstacleActive)
        {
            nextObstacle--;

            if (nextObstacle <= 0)
            {
                obstacleActive = 1;

                obstacleX =
                    LEFT +
                    rand() %
                    (RIGHT - LEFT + 1);

                obstacleY =
                    TOP +
                    rand() %
                    (BOTTOM - TOP + 1);

                while (
                    obstacleX == playerX &&
                    obstacleY == playerY
                )
                {
                    obstacleX =
                        LEFT +
                        rand() %
                        (RIGHT - LEFT + 1);

                    obstacleY =
                        TOP +
                        rand() %
                        (BOTTOM - TOP + 1);
                }

                obstacleTimer =
                    60 +
                    rand() %
                    (80 + difficulty * 40);
            }
        }
        else
        {
            obstacleTimer--;

            if (obstacleTimer <= 0)
            {
                obstacleActive = 0;

                nextObstacle =
                    70 +
                    rand() %
                    (100 - difficulty * 10);
            }
        }

        /*
           Столкновение с препятствием.
        */

        if (
            obstacleActive &&
            playerX == obstacleX &&
            playerY == obstacleY
        )
        {
            game_over(
                pad,
                score,
                *best
            );

            return;
        }

        /*
           Добавляем Орешники каждые 10 очков.
        */

        if (
            score > 0 &&
            score % 10 == 0 &&
            missileCount < 2 + difficulty
        )
        {
            int activeCount = 0;

            for (i = 0; i < missileCount; i++)
            {
                if (missiles[i].active)
                    activeCount++;
            }

            if (activeCount == missileCount)
            {
                if (missileCount < MAX_MISSILES)
                {
                    spawn_missile(
                        &missiles[missileCount],
                        difficulty
                    );

                    missileCount++;
                }
            }
        }

        sceDisplayWaitVblankStart();
    }
}

/* -------------------------------------------------- */
/* СЛОЖНОСТЬ                                         */
/* -------------------------------------------------- */

void difficulty_menu(
    SceCtrlData *pad,
    int *difficulty
)
{
    int selected = *difficulty;

    while (1)
    {
        pspDebugScreenClear();

        pspDebugScreenPrintf("\n\n");
        pspDebugScreenPrintf(
            "================================\n"
        );

        pspDebugScreenPrintf(
            "           DIFFICULTY\n"
        );

        pspDebugScreenPrintf(
            "================================\n\n"
        );

        if (selected == 0)
            pspDebugScreenPrintf("  > EASY\n");
        else
            pspDebugScreenPrintf("    EASY\n");

        if (selected == 1)
            pspDebugScreenPrintf("  > NORMAL\n");
        else
            pspDebugScreenPrintf("    NORMAL\n");

        if (selected == 2)
            pspDebugScreenPrintf("  > HARD\n");
        else
            pspDebugScreenPrintf("    HARD\n");

        if (selected == 3)
            pspDebugScreenPrintf("  > INSANE\n");
        else
            pspDebugScreenPrintf("    INSANE\n");

        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("X - SELECT\n");
        pspDebugScreenPrintf("START - BACK\n");

        sceCtrlReadBufferPositive(pad, 1);

        if (pad->Buttons & PSP_CTRL_UP)
        {
            selected--;

            if (selected < 0)
                selected = 3;

            sceKernelDelayThread(150000);
        }

        if (pad->Buttons & PSP_CTRL_DOWN)
        {
            selected++;

            if (selected > 3)
                selected = 0;

            sceKernelDelayThread(150000);
        }

        if (pad->Buttons & PSP_CTRL_CROSS)
        {
            *difficulty = selected;
            return;
        }

        if (pad->Buttons & PSP_CTRL_START)
            return;

        sceDisplayWaitVblankStart();
    }
}

/* -------------------------------------------------- */
/* MAIN                                               */
/* -------------------------------------------------- */

int main(void)
{
    SceCtrlData pad;

    int selected = 0;
    int difficulty = 1;
    int best = 0;

    setup_callbacks();

    pspDebugScreenInit();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(
        PSP_CTRL_MODE_DIGITAL
    );

    srand(
        (unsigned int)time(NULL)
    );

    while (1)
    {
        pspDebugScreenClear();

        pspDebugScreenPrintf("\n\n");

        pspDebugScreenPrintf(
            "================================\n"
        );

        pspDebugScreenPrintf(
            "          NIKITU v0.0.6\n"
        );

        pspDebugScreenPrintf(
            "================================\n\n"
        );

        if (selected == 0)
            pspDebugScreenPrintf(
                "  > START GAME\n"
            );
        else
            pspDebugScreenPrintf(
                "    START GAME\n"
            );

        if (selected == 1)
            pspDebugScreenPrintf(
                "  > DIFFICULTY\n"
            );
        else
            pspDebugScreenPrintf(
                "    DIFFICULTY\n"
            );

        if (selected == 2)
            pspDebugScreenPrintf(
                "  > OPTIONS\n"
            );
        else
            pspDebugScreenPrintf(
                "    OPTIONS\n"
            );

        if (selected == 3)
            pspDebugScreenPrintf(
                "  > EXIT\n"
            );
        else
            pspDebugScreenPrintf(
                "    EXIT\n"
            );

        pspDebugScreenPrintf("\n\n");

        pspDebugScreenPrintf(
            "UP/DOWN - SELECT\n"
        );

        pspDebugScreenPrintf(
            "X - CONFIRM\n"
        );

        sceCtrlReadBufferPositive(
            &pad,
            1
        );

        if (pad.Buttons & PSP_CTRL_UP)
        {
            selected--;

            if (selected < 0)
                selected = 3;

            sceKernelDelayThread(
                150000
            );
        }

        if (pad.Buttons & PSP_CTRL_DOWN)
        {
            selected++;

            if (selected > 3)
                selected = 0;

            sceKernelDelayThread(
                150000
            );
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
                pspDebugScreenClear();

                pspDebugScreenPrintf(
                    "\n\n"
                );

                pspDebugScreenPrintf(
                    "OPTIONS\n\n"
                );

                pspDebugScreenPrintf(
                    "Nothing here yet.\n\n"
                );

                pspDebugScreenPrintf(
                    "PRESS START"
                );

                wait_start(&pad);
            }
            else if (selected == 3)
            {
                sceKernelExitGame();
            }
        }

        sceDisplayWaitVblankStart();
    }

    return 0;
}