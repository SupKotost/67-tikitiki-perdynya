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
                pspDebugScreenClear();
                pspDebugScreenPrintf("\n\n");
                pspDebugScreenPrintf("================================\n");
                pspDebugScreenPrintf("          GAME START!\n");
                pspDebugScreenPrintf("================================\n\n");
                pspDebugScreenPrintf("THE METEOR IS COMING...\n");

                sceKernelDelayThread(3000000);
            }
            else if (selected == 1)
            {
                pspDebugScreenClear();
                pspDebugScreenPrintf("\n\nOPTIONS\n\n");
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
