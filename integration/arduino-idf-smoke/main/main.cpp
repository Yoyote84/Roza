#include <Arduino.h>

#include "launcher_platform.h"

#include <cinttypes>
#include <cstdint>

extern "C" void app_main(void) {
    // NucleoOS owns app_main(). Arduino must therefore be initialized explicitly;
    // the Arduino component's automatic setup()/loop() task stays disabled.
    initArduino();

    launcherConsoleBegin(115200);
    launcherConsolePrintln("ROZA: ESP-IDF + Arduino bridge smoke test");

    const uint32_t start_ms = launcherMillis();
    const uint32_t sample = launcherRandom(100U);
    launcherConsolePrintf(
        "Launcher platform bridge linked: millis=%" PRIu32 " random=%" PRIu32 "\n",
        start_ms,
        sample
    );

    // Exercise the IDF-backed, bounds-safe GPIO wrapper without touching a board pin.
    launcherGpioInput(-1);
    launcherGpioOutput(-1);
    launcherGpioWrite(-1, 0);
    (void)launcherGpioRead(-1);

    for (;;) {
        launcherDelayMs(1000U);
        launcherConsolePrintf("ROZA bridge alive: %" PRIu32 " ms\n", launcherMillis() - start_ms);
    }
}
