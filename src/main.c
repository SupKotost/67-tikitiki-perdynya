#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <time.h>

PSP_MODULE_INFO("Nikitu v0.0.5", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

#define LEFT 1
#define RIGHT 38
#define TOP 1
#define BOTTOM 18
#define MAX_MISSILES 5

typedef struct
{
    int x;
    int y;
    int direction;
    int speed;
    int frame;
    int active;
} Missile;

int exit_callback(int a, int b, void *c)
{
    sceKernelExitGame();
    return 0;
}

int callback_thread(SceSize args, void *argp)
{
    int cbid = sceKernelCreateCallback(
        "Exit Callback", exit_callback, NULL
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

void spawn_missile(Missile *m, int difficulty)
{
    int central;

    m->active = 1;
    m->frame = 0;

    m->direction = rand() % 4;

    /*
       Чаще выбираем траектории ближе к центру.
    */
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

    /*
       Чем выше сложность, тем быстрее.
    */
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
    pspDebugScreenPrintf("FINAL METEOR WAVE\n");
    pspDebugScreenPrintf("2 METEORITES SURVIVED!\n\n");

    pspDebugScreenPrintf("BUTCHER VICTORY!\n");
    pspDebugScreenPrintf("SMACHNOOOOOOOO!\n\n");

    pspDebugScreenPrintf("PRESS START TO RETURN");

    wait_start(pad);
}

void final_wave(SceCtrlData *pad, int px, int py)
{
    Missile m[2];

    int i;
    int frame = 0;

    m[0].active = 0;
    m[1].active = 0;

    spawn_missile(&m[0], 3);
    spawn_missile(&m[1], 3);

    while (1)
    {
        pspDebugScreenClear();

        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("          FINAL METEOR WAVE\n");
        pspDebugScreenPrintf("========================================\n\n");

        for (int y = 0; y < 20; y++)
        {
            for (int x = 0; x < 40; x++)
            {
                int printed = 0;

                if (x == px && y == py)
                {
                    pspDebugScreenPrintf("@");
                    printed = 1;
                }

                for (i = 0; i < 2; i++)
                {
                    if (
                        !printed &&
                        m[i].active &&
                        m[i].x == x &&
                        m[i].y == y
                    )
                    {
                        pspDebugScreenPrintf("M");
                        printed = 1;
                    }
                }

                if (!printed)
                {
                    if (
                        x == 0 ||
                        x == 39 ||
                        y == 0 ||
                        y == 19
                    )
                        pspDebugScreenPrintf("#");
                    else
                        pspDebugScreenPrintf(" ");
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

        frame++;

        if (frame >= 1)
        {
            frame = 0;

            move_missile(&m[0]);
            move_missile(&m[1]);
        }

        if (hit_player(&m[0], px, py) ||
            hit_player(&m[1], px, py))
        {
            game_over(pad, 67, 67);
            return;
        }

        if (!m[0].active && !m[1].active)
        {
            victory(pad);
            return;
        }

        sceDisplayWaitVblankStart();
    }
}

void game(SceCtrlData *pad, int difficulty, int *best)
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
        spawn_missile(&missiles[i], difficulty);

    while (1)
    {
        pspDebugScreenClear();

        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("             NIKITU v0.0.5\n");
        pspDebugScreenPrintf("========================================\n");
        pspDebugScreenPrintf("SCORE: %d     BEST: %d\n\n", score, *best);

        for (int y = 0; y < 20; y++)
        {
            for (int x = 0; x < 40; x++)
            {
                int printed = 0;

                if (x == playerX && y == playerY)
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
                        x == 39 ||
                        y == 0 ||
                        y == 19
                    )
                        pspDebugScreenPrintf("#");
                    else
                        pspDebugScreenPrintf(" ");
                }
            }

            pspDebugScreenPrintf("\n");
        }

        pspDebugScreenPrintf("\nD-PAD - MOVE\n");
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
           Двигаем все Орешники.
        */

        for (i = 0; i < missileCount; i++)
            move_missile(&missiles[i]);

        /*
           Если Орешник пережит — новый появляется сразу.
        */

        for (i = 0; i < missileCount; i++)
        {
            if (!missiles[i].active)
            {
                score++;

                if (score > *best)
                    *best = score;

                /*
                   После 67 обычных Орешников
                   запускаем финальную волну.
                */
                if (score >= 67)
                {
                    final_wave(pad, playerX, playerY);
                    return;
                }

                spawn_missile(&missiles[i], difficulty);
            }
        }

        /*
           Проверка столкновения с Орешником.
        */

        for (i = 0; i < missileCount; i++)
        {
            if (hit_player(&missiles[i], playerX, playerY))
            {
                game_over(pad, score, *best);
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
                    LEFT + rand() % (RIGHT - LEFT + 1);

                obstacleY =
                    TOP + rand() % (BOTTOM - TOP + 1);

                /*
                   Не спавним препятствие прямо на игроке.
                */

                while (
                    obstacleX == playerX &&
                    obstacleY == playerY
                )
                {
                    obstacleX =
                        LEFT + rand() % (RIGHT - LEFT + 1);

                    obstacleY =
                        TOP + rand() % (BOTTOM - TOP + 1);
                }

                obstacleTimer =
                    60 + rand() % (80 + difficulty * 40);
            }
        }
        else
        {
            obstacleTimer--;

            if (obstacleTimer <= 0)
            {
                obstacleActive = 0;

                nextObstacle =
                    70 + rand() % (100 - difficulty * 10);
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
            game_over(pad, score, *best);
            return;
        }

        /*
           После каждых 10 очков добавляем
           ещё один Орешник, если есть место.
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
        pspDebugScreenPrintf("================================\n");
        pspDebugScreenPrintf("           DIFFICULTY\n");
        pspDebugScreenPrintf("================================\n\n");

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

        pspDebugScreenPrintf("\nX - SELECT\nSTART - BACK\n");

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

int main(void)
{
    SceCtrlData pad;

    int selected = 0;
    int difficulty = 1;
    int best = 0;

    setup_callbacks();

    pspDebugScreenInit();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);

    srand((unsigned int)time(NULL));

    while (1)
    {
        pspDebugScreenClear();

        pspDebugScreenPrintf("\n\n");
        pspDebugScreenPrintf("================================\n");
        pspDebugScreenPrintf("          NIKITU v0.0.5\n");
        pspDebugScreenPrintf("================================\n\n");

        if (selected == 0)
            pspDebugScreenPrintf("  > START GAME\n");
        else
            pspDebugScreenPrintf("    START GAME\n");

        if (selected == 1)
            pspDebugScreenPrintf("  > DIFFICULTY\n");
        else
            pspDebugScreenPrintf("    DIFFICULTY\n");

        if (selected == 2)
            pspDebugScreenPrintf("  > OPTIONS\n");
        else
            pspDebugScreenPrintf("    OPTIONS\n");

        if (selected == 3)
            pspDebugScreenPrintf("  > EXIT\n");
        else
            pspDebugScreenPrintf("    EXIT\n");

        pspDebugScreenPrintf("\n\n");
        pspDebugScreenPrintf("UP/DOWN - SELECT\n");
        pspDebugScreenPrintf("X - CONFIRM\n");

        sceCtrlReadBufferPositive(&pad, 1);

        if (pad.Buttons & PSP_CTRL_UP)
        {
            selected--;

            if (selected < 0)
                selected = 3;

            sceKernelDelayThread(150000);
        }

        if (pad.Buttons & PSP_CTRL_DOWN)
        {
            selected++;

            if (selected > 3)
                selected = 0;

            sceKernelDelayThread(150000);
        }

        if (pad.Buttons & PSP_CTRL_CROSS)
        {
            if (selected == 0)
            {
                game(&pad, difficulty, &best);
            }
            else if (selected == 1)
            {
                difficulty_menu(&pad, &difficulty);
            }
            else if (selected == 2)
            {
                pspDebugScreenClear();

                pspDebugScreenPrintf("\n\n");
                pspDebugScreenPrintf("OPTIONS\n\n");
                pspDebugScreenPrintf("Nothing here yet.\n\n");
                pspDebugScreenPrintf("PRESS START");

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