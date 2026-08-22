#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <time.h>

PSP_MODULE_INFO("Nikitu v0.0.4", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

#define FIELD_LEFT   1
#define FIELD_RIGHT  38
#define FIELD_TOP    1
#define FIELD_BOTTOM 18

int exit_callback(int arg1, int arg2, void *common)
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

int setup_callbacks(void)
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

    return thid;
}

/*
    Направления Орешника:

    0 = слева -> направо
    1 = справа -> налево
    2 = сверху -> вниз
    3 = снизу -> вверх
*/

void spawn_missile(
    int *missileX,
    int *missileY,
    int *direction,
    int *missileSpeed
)
{
    *direction = rand() % 4;

    *missileSpeed = 1 + rand() % 3;

    if (*direction == 0)
    {
        *missileX = FIELD_LEFT;
        *missileY = FIELD_TOP +
                     rand() % (FIELD_BOTTOM - FIELD_TOP + 1);
    }
    else if (*direction == 1)
    {
        *missileX = FIELD_RIGHT;
        *missileY = FIELD_TOP +
                     rand() % (FIELD_BOTTOM - FIELD_TOP + 1);
    }
    else if (*direction == 2)
    {
        *missileX = FIELD_LEFT +
                    rand() % (FIELD_RIGHT - FIELD_LEFT + 1);
        *missileY = FIELD_TOP;
    }
    else
    {
        *missileX = FIELD_LEFT +
                    rand() % (FIELD_RIGHT - FIELD_LEFT + 1);
        *missileY = FIELD_BOTTOM;
    }
}

int missile_inside_field(int x, int y)
{
    if (x < FIELD_LEFT || x > FIELD_RIGHT)
        return 0;

    if (y < FIELD_TOP || y > FIELD_BOTTOM)
        return 0;

    return 1;
}

void game_over(SceCtrlData *pad)
{
    pspDebugScreenClear();

    pspDebugScreenPrintf("\n\n");
    pspDebugScreenPrintf("========================================\n");
    pspDebugScreenPrintf("              GAME OVER\n");
    pspDebugScreenPrintf("========================================\n\n");

    pspDebugScreenPrintf("THE ORESHNIK GOT YOU.\n\n");
    pspDebugScreenPrintf("PRESS START TO RETURN\n");

    while (1)
    {
        sceCtrlReadBufferPositive(pad, 1);

        if (pad.Buttons & PSP_CTRL_START)
            return;

        sceDisplayWaitVblankStart();
    }
}

void game(void)
{
    SceCtrlData pad;

    int playerX = 20;
    int playerY = 10;

    int missileX;
    int missileY;
    int missileDirection;
    int missileSpeed;

    int frame = 0;

    int obstacleX = 0;
    int obstacleY = 0;
    int obstacleActive = 0;

    int obstacleTimer = 0;
    int obstacleDuration = 0;

    int nextObstacleTimer = 100;

    srand((unsigned int)time(NULL));

    spawn_missile(
        &missileX,
        &missileY,
        &missileDirection,
        &missileSpeed
    );

    while (1)
    {
        pspDebugScreenClear();

        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("             NIKITU v0.0.4\n");
        pspDebugScreenPrintf("========================================\n\n");

        for (int y = 0; y < 20; y++)
        {
            for (int x = 0; x < 40; x++)
            {
                if (x == playerX && y == playerY)
                {
                    pspDebugScreenPrintf("@");
                }
                else if (
                    x == missileX &&
                    y == missileY &&
                    missile_inside_field(missileX, missileY)
                )
                {
                    pspDebugScreenPrintf("O");
                }
                else if (
                    obstacleActive &&
                    x == obstacleX &&
                    y == obstacleY
                )
                {
                    pspDebugScreenPrintf("#");
                }
                else if (
                    x == 0 ||
                    x == 39 ||
                    y == 0 ||
                    y == 19
                )
                {
                    pspDebugScreenPrintf("#");
                }
                else
                {
                    pspDebugScreenPrintf(" ");
                }
            }

            pspDebugScreenPrintf("\n");
        }

        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("D-PAD - MOVE\n");
        pspDebugScreenPrintf("START - MENU\n");

        sceCtrlReadBufferPositive(&pad, 1);

        /* Управление игроком */

        if (pad.Buttons & PSP_CTRL_LEFT)
        {
            if (playerX > FIELD_LEFT)
                playerX--;

            sceKernelDelayThread(50000);
        }

        if (pad.Buttons & PSP_CTRL_RIGHT)
        {
            if (playerX < FIELD_RIGHT)
                playerX++;

            sceKernelDelayThread(50000);
        }

        if (pad.Buttons & PSP_CTRL_UP)
        {
            if (playerY > FIELD_TOP)
                playerY--;

            sceKernelDelayThread(50000);
        }

        if (pad.Buttons & PSP_CTRL_DOWN)
        {
            if (playerY < FIELD_BOTTOM)
                playerY++;

            sceKernelDelayThread(50000);
        }

        if (pad.Buttons & PSP_CTRL_START)
            return;

        /* Движение Орешника */

        frame++;

        if (frame >= missileSpeed)
        {
            frame = 0;

            if (missileDirection == 0)
                missileX++;

            else if (missileDirection == 1)
                missileX--;

            else if (missileDirection == 2)
                missileY++;

            else if (missileDirection == 3)
                missileY--;
        }

        /*
            Если Орешник вылетел за пределы поля,
            создаём новый с другой случайной траекторией.
        */

        if (
            missileX < FIELD_LEFT ||
            missileX > FIELD_RIGHT ||
            missileY < FIELD_TOP ||
            missileY > FIELD_BOTTOM
        )
        {
            spawn_missile(
                &missileX,
                &missileY,
                &missileDirection,
                &missileSpeed
            );

            frame = 0;
        }

        /* Столкновение с Орешником */

        if (
            playerX == missileX &&
            playerY == missileY
        )
        {
            game_over(&pad);
            return;
        }

        /* Таймер появления препятствия */

        if (!obstacleActive)
        {
            nextObstacleTimer--;

            if (nextObstacleTimer <= 0)
            {
                obstacleActive = 1;

                obstacleX =
                    FIELD_LEFT +
                    rand() % (FIELD_RIGHT - FIELD_LEFT + 1);

                obstacleY =
                    FIELD_TOP +
                    rand() % (FIELD_BOTTOM - FIELD_TOP + 1);

                /*
                    Не создаём препятствие
                    прямо на игроке.
                */

                while (
                    obstacleX == playerX &&
                    obstacleY == playerY
                )
                {
                    obstacleX =
                        FIELD_LEFT +
                        rand() % (FIELD_RIGHT - FIELD_LEFT + 1);

                    obstacleY =
                        FIELD_TOP +
                        rand() % (FIELD_BOTTOM - FIELD_TOP + 1);
                }

                obstacleDuration = 60 + rand() % 120;
                obstacleTimer = obstacleDuration;
            }
        }
        else
        {
            obstacleTimer--;

            if (obstacleTimer <= 0)
            {
                obstacleActive = 0;

                nextObstacleTimer =
                    80 + rand() % 180;
            }
        }

        /* Столкновение с препятствием */

        if (
            obstacleActive &&
            playerX == obstacleX &&
            playerY == obstacleY
        )
        {
            game_over(&pad);
            return;
        }

        sceDisplayWaitVblankStart();
    }
}

int main(void)
{
    SceCtrlData pad;
    int selected = 0;

    setup_callbacks();

    pspDebugScreenInit();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);

    while (1)
    {
        pspDebugScreenClear();

        pspDebugScreenPrintf("\n\n");
        pspDebugScreenPrintf("================================\n");
        pspDebugScreenPrintf("            NIKITU v0.0.4\n");
        pspDebugScreenPrintf("================================\n\n");

        if (selected == 0)
            pspDebugScreenPrintf("  > START GAME\n");
        else
            pspDebugScreenPrintf("    START GAME\n");

        if (selected == 1)
            pspDebugScreenPrintf("  > OPTIONS\n");
        else
            pspDebugScreenPrintf("    OPTIONS\n");

        if (selected == 2)
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
                selected = 2;

            sceKernelDelayThread(150000);
        }

        if (pad.Buttons & PSP_CTRL_DOWN)
        {
            selected++;

            if (selected > 2)
                selected = 0;

            sceKernelDelayThread(150000);
        }

        if (pad.Buttons & PSP_CTRL_CROSS)
        {
            if (selected == 0)
            {
                game();
            }
            else if (selected == 1)
            {
                pspDebugScreenClear();

                pspDebugScreenPrintf("\n\n");
                pspDebugScreenPrintf("OPTIONS\n\n");
                pspDebugScreenPrintf("Nothing here yet.\n");

                sceKernelDelayThread(2000000);
            }
            else if (selected == 2)
            {
                sceKernelExitGame();
            }
        }

        sceDisplayWaitVblankStart();
    }

    return 0;
}