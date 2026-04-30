#include <fancontrol.h>
#include <switch.h>

#define FAN_DEVICE              0x3D000001
#define FAN_MIN_LEVEL           0.20f
#define FAN_OFF_LEVEL           0.0f
#define NUM_TEMP_POINTS         6

#define TMP451_SOC_INT_REG      0x01
#define TMP451_SOC_DEC_REG      0x10

// Blast the set command at 5ms intervals to continuously override the stock
// thermal service, which reasserts its own fan speed via IPC callbacks.
// TMP451 conversion cycle is ~125ms, so temp is only re-read once per 20 blasts.
#define POLL_INTERVAL_NS          1000000LL
#define POLL_BLASTS_PER_CYCLE          100

// Self-contained SoC temp read — avoids linker dependency on libfancontrol
// internal symbols (tmp451.h/i2c.h define functions inline, causing duplicate
// symbol errors if included in a second translation unit).
static Result readSocTemp(float* out)
{
    struct {
        u8 send;
        u8 sendLength;
        u8 sendData;
        u8 receive;
        u8 receiveLength;
    } cmd = {
        .send          = I2cTransactionOption_Start << 6,
        .sendLength    = 1,
        .sendData      = 0,
        .receive       = (I2cTransactionOption_All << 6) | 1,
        .receiveLength = 1,
    };

    I2cSession session;
    Result rc = i2cOpenSession(&session, I2cDevice_Tmp451);
    if (R_FAILED(rc)) return rc;

    u8 integer = 0, dec = 0;

    cmd.sendData = TMP451_SOC_INT_REG;
    rc = i2csessionExecuteCommandList(&session, &integer, 1, &cmd, sizeof(cmd));
    if (R_SUCCEEDED(rc)) {
        cmd.sendData = TMP451_SOC_DEC_REG;
        i2csessionExecuteCommandList(&session, &dec, 1, &cmd, sizeof(cmd));
    }

    i2csessionClose(&session);
    if (R_FAILED(rc)) return rc;

    u8 decimals = ((u16)(dec >> 4) * 625) / 100;
    *out = (float)integer + (float)decimals / 100.0f;
    return 0;
}

static float fanInterpolate(TemperaturePoint* table, float temp)
{
    if (temp <= (float)table[0].temperature_c) {
        if (table[0].fanLevel_f <= 0.0f) return 0.0f;
        float m = table[0].fanLevel_f / (float)table[0].temperature_c;
        return m * temp - m;
    }

    if (temp >= (float)table[NUM_TEMP_POINTS - 1].temperature_c)
        return table[NUM_TEMP_POINTS - 1].fanLevel_f;

    for (int i = 0; i < NUM_TEMP_POINTS - 1; i++) {
        if (temp >= (float)table[i].temperature_c &&
            temp <= (float)table[i+1].temperature_c)
        {
            float m = (table[i+1].fanLevel_f - table[i].fanLevel_f) /
                      (float)(table[i+1].temperature_c - table[i].temperature_c);
            float q = table[i].fanLevel_f - m * (float)table[i].temperature_c;
            return m * temp + q;
        }
    }
    return 0.0f;
}

// Runs the fan control loop indefinitely. Never returns.
//
// Selects handheld or docked curve each iteration via APM service.
// Hysteresis: fan turns ON when curve reaches FAN_MIN_LEVEL, holds at
// FAN_MIN_LEVEL while cooling through the transition zone, turns OFF only when
// curve returns to 0.0 (at/below P0.temp). Presets set P0.fanLevel = 0.0 and
// P1.fanLevel = 0.20 a few degrees apart to define the hysteresis band.
void RunFanLoop(TemperaturePoint* handheld, TemperaturePoint* docked)
{
    FanController fc;
    Result rs;
    for (int retry = 0; retry < 10; retry++) {
        rs = fanOpenController(&fc, FAN_DEVICE);
        if (R_SUCCEEDED(rs)) break;
        svcSleepThread(100000000LL); // 100ms — wait for previous session to close
    }
    if (R_FAILED(rs)) {
        WriteLog("Error opening fanController");
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));
    }

    bool fanActive = false;

    while (1) {
        ApmPerformanceMode mode = ApmPerformanceMode_Normal;
        apmGetPerformanceMode(&mode);
        TemperaturePoint* table;
        if (mode == ApmPerformanceMode_Boost) {
            table = docked;
        } else {
            PsmChargerType chargerType = PsmChargerType_Unconnected;
            psmGetChargerType(&chargerType);
            table = (chargerType == PsmChargerType_EnoughPower) ? docked : handheld;
        }

        float temp = 0.0f;
        rs = readSocTemp(&temp);
        if (R_FAILED(rs)) {
            WriteLog("readSocTemp error");
            diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));
        }

        float desired = fanInterpolate(table, temp);
        if (desired < 0.0f) desired = 0.0f;

        float setLevel;
        if (desired <= 0.0f) {
            setLevel  = FAN_OFF_LEVEL;
            fanActive = false;
        } else if (desired < FAN_MIN_LEVEL) {
            setLevel = fanActive ? FAN_MIN_LEVEL : FAN_OFF_LEVEL;
        } else {
            setLevel  = desired;
            fanActive = true;
        }

        for (int i = 0; i < POLL_BLASTS_PER_CYCLE; i++) {
            rs = fanControllerSetRotationSpeedLevel(&fc, setLevel);
            if (R_FAILED(rs)) {
                WriteLog("fanControllerSetRotationSpeedLevel error");
                diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));
            }
            svcSleepThread(POLL_INTERVAL_NS);
        }
    }

    fanControllerClose(&fc);
}
