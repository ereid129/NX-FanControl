#define TESLA_INIT_IMPL
#include <exception_wrap.hpp>
#include <tesla.hpp>
#include "pwm.h"
#include "main_menu.hpp"

PwmChannelSession g_fanSession;
I2cSession        g_tmpSession;
Result g_pwmCheck  = -1;
Result g_i2cCheck  = -1;
Result g_tmpCheck  = -1;
Result g_tcCheck   = -1;
Service g_apmRawSrv = {};
Result  g_apmRawCheck = -1;

class NxFanControlOverlay : public tsl::Overlay {
public:
    virtual void initServices() override
    {
        fsdevMountSdmc();
        pmshellInitialize();
        pmdmntInitialize();
        pminfoInitialize();
        apmInitialize();
        g_apmRawCheck = smGetService(&g_apmRawSrv, "apm");
        psmInitialize();

        if (hosversionAtLeast(6, 0, 0) && R_SUCCEEDED(pwmInitialize()))
            g_pwmCheck = pwmOpenSession2(&g_fanSession, 0x3D000001);

        g_i2cCheck = i2cInitialize();
        if (R_SUCCEEDED(g_i2cCheck))
            g_tmpCheck = i2cOpenSession(&g_tmpSession, I2cDevice_Tmp451);

        if (hosversionAtLeast(5, 0, 0))
            g_tcCheck = tcInitialize();
    }

    virtual void exitServices() override
    {
        if (R_SUCCEEDED(g_tcCheck))   tcExit();
        if (R_SUCCEEDED(g_pwmCheck)) { pwmChannelSessionClose(&g_fanSession); pwmExit(); }
        if (R_SUCCEEDED(g_tmpCheck)) i2csessionClose(&g_tmpSession);
        if (R_SUCCEEDED(g_i2cCheck)) i2cExit();
        psmExit();
        if (R_SUCCEEDED(g_apmRawCheck)) serviceClose(&g_apmRawSrv);
        apmExit();
        pminfoExit();
        pmdmntExit();
        pmshellExit();
        fsdevUnmountAll();
    }

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<MainMenu>();
    }
};

int main(int argc, char **argv) {
    return tsl::loop<NxFanControlOverlay>(argc, argv);
}
