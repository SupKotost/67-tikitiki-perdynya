#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>

PSP_MODULE_INFO("67 TIKI TIKI", 0, 1, 0);
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

    setup_callbacks();

    pspDebugScreenInit();

    pspDebugScreenPrintf("================================\n");
    pspDebugScreenPrintf("        67 TIKI TIKI\n");
    pspDebugScreenPrintf("================================\n\n");

    pspDebugScreenPrintf("HOMELANDER EDITION\n\n");
    pspDebugScreenPrintf("PRESS START TO BEGIN\n");

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);

    while (1)
    {
        sceCtrlReadBufferPositive(&pad, 1);

        if (pad.Buttons & PSP_CTRL_START)
        {
            pspDebugScreenClear();

            pspDebugScreenPrintf("67 TIKI TIKI\n\n");
            pspDebugScreenPrintf("GAME START!\n\n");
            pspDebugScreenPrintf("THE METEOR IS COMING...\n");
        }

        sceDisplayWaitVblankStart();
    }

    return 0;
}
