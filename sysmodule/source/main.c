#include "fancontrol.h"
#include <sys/stat.h>

// Size of the inner heap (adjust as necessary).
#define INNER_HEAP_SIZE   0x80000
#define NUM_TEMP_POINTS   6

#ifdef __cplusplus
extern "C" {
#endif

// Sysmodules should not use applet*.
u32 __nx_applet_type = AppletType_None;

// Sysmodules will normally only want to use one FS session.
u32 __nx_fs_num_sessions = 1;

// Newlib heap configuration function (makes malloc/free work).
void __libnx_initheap(void)
{
    static u8 inner_heap[INNER_HEAP_SIZE];
    extern void* fake_heap_start;
    extern void* fake_heap_end;

    // Configure the newlib heap.
    fake_heap_start = inner_heap;
    fake_heap_end   = inner_heap + sizeof(inner_heap);
}

// Service initialization.
void __appInit(void)
{
    Result rc;

    rc = smInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }

    rc = fsInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    rc = fsdevMountSdmc();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    rc = fanInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));

    rc = i2cInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));

    rc = apmInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));

    rc = psmInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));

    smExit();
}

// Service deinitialization.
void __appExit(void)
{
    psmExit();
    apmExit();
    fanExit();
    i2cExit();
    fsExit();
    fsdevUnmountAll();
}

#ifdef __cplusplus
}
#endif

void RunFanLoop(TemperaturePoint* handheld, TemperaturePoint* docked);

static void writeFanConfig(TemperaturePoint* handheld, TemperaturePoint* docked)
{
    FILE* f = fopen(CONFIG_FILE, "wb");
    if (f) {
        fwrite(handheld, sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f);
        fwrite(docked,   sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f);
        fclose(f);
    }
}

static void readFanConfig(TemperaturePoint** handheld, TemperaturePoint** docked)
{
    static const TemperaturePoint defaultHandheld[NUM_TEMP_POINTS] = {
        { 37, 0.00f }, { 40, 0.20f }, { 48, 0.20f },
        { 55, 0.40f }, { 60, 0.60f }, { 60, 0.60f },
    };
    static const TemperaturePoint defaultDocked[NUM_TEMP_POINTS] = {
        { 37, 0.00f }, { 40, 0.20f }, { 53, 0.30f },
        { 57, 0.40f }, { 65, 0.60f }, { 76, 1.00f },
    };

    *handheld = malloc(sizeof(TemperaturePoint) * NUM_TEMP_POINTS);
    *docked   = malloc(sizeof(TemperaturePoint) * NUM_TEMP_POINTS);
    memcpy(*handheld, defaultHandheld, sizeof(defaultHandheld));
    memcpy(*docked,   defaultDocked,   sizeof(defaultDocked));

    FILE* f = fopen(CONFIG_FILE, "rb");
    if (f) {
        if (fread(*handheld, sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f) != 1 ||
            fread(*docked,   sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f) != 1) {
            memcpy(*handheld, defaultHandheld, sizeof(defaultHandheld));
            memcpy(*docked,   defaultDocked,   sizeof(defaultDocked));
        }
        fclose(f);
    }
}

// Main program entrypoint
int main(int argc, char* argv[])
{
    mkdir("./config", 0777);
    mkdir("./config/NX-FanControl", 0777);

    TemperaturePoint *handheld, *docked;
    readFanConfig(&handheld, &docked);

    // Seed config file on first install using the defaults readFanConfig loaded.
    if (access(CONFIG_FILE, F_OK) != 0)
        writeFanConfig(handheld, docked);

    RunFanLoop(handheld, docked);
    return 0;
}
