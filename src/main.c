#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>

PSP_MODULE_INFO("Nikitu", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

int exit_callback(int arg1, int arg2, void *common)
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

void game(void)
{
    SceCtrlData pad;

    int playerX = 15;
    int playerY = 10;

    while (1)
    {
        pspDebugScreenClear();

        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("             NIKITU\n");
        pspDebugScreenPrintf("================================\n\n");

        /* Игровое поле */
        for (int y = 0; y < 20; y++)
        {
            for (int x = 0; x < 40; x++)
            {
                if (x == playerX && y == playerY)
                    pspDebugScreenPrintf("@");
                else if (x == 0 || x == 39 || y == 0 || y == 19)
                    pspDebugScreenPrintf("#");
                else
                    pspDebugScreenPrintf(" ");
            }

            pspDebugScreenPrintf("\n");
        }

        pspDebugScreenPrintf("\nD-PAD - MOVE");
        pspDebugScreenPrintf("\nSTART - MENU");

        sceCtrlReadBufferPositive(&pad, 1);

        if (pad.Buttons & PSP_CTRL_LEFT)
        {
            if (playerX > 1)
                playerX--;

            sceKernelDelayThread(80000);
        }

        if (pad.Buttons & PSP_CTRL_RIGHT)
        {
            if (playerX < 38)
                playerX++;

            sceKernelDelayThread(80000);
        }

        if (pad.Buttons & PSP_CTRL_UP)
        {
            if (playerY > 1)
                playerY--;

            sceKernelDelayThread(80000);
        }

        if (pad.Buttons & PSP_CTRL_DOWN)
        {
            if (playerY < 18)
                playerY++;

            sceKernelDelayThread(80000);
        }

        if (pad.Buttons & PSP_CTRL_START)
            return;

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
        pspDebugScreenPrintf("            NIKITU\n");
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