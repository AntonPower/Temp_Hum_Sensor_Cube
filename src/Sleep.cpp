#include "Sleep.h"

extern "C" {
#include "gpio.h"
#include "user_interface.h"
}

namespace Sleep {
namespace {

volatile bool wokeUp = false;

void onWake() {
    wokeUp = true;
}

}  // namespace

bool light(uint32_t durationMs, uint8_t wakePin) {
    durationMs = min(durationMs, MAX_LIGHT_SLEEP_MS);
    Serial.flush();
    wokeUp = false;

    // WiFi.forceSleepBegin() hat Modem-Sleep geöffnet, das muss erst zu
    wifi_fpm_do_wakeup();
    wifi_fpm_close();
    wifi_station_disconnect();
    wifi_set_opmode_current(NULL_MODE);
    wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
    wifi_fpm_open();
    wifi_fpm_set_wakeup_cb(onWake);
    gpio_pin_wakeup_enable(GPIO_ID_PIN(wakePin), GPIO_PIN_INTR_LOLEVEL);

    wifi_fpm_do_sleep(durationMs * 1000UL);
    // Der Chip schläft erst während delay() ein. In kurzen Stücken warten,
    // damit ein Tastendruck sofort zählt, auch falls der Schlaf nicht klappt.
    const uint32_t start = millis();
    while (!wokeUp && digitalRead(wakePin) == HIGH && millis() - start <= durationMs) {
        delay(10);
    }

    gpio_pin_wakeup_disable();
    wifi_fpm_close();

    // Taster liegt nach dem Aufwachen noch auf LOW, wenn er geweckt hat
    return digitalRead(wakePin) == LOW;
}

}  // namespace Sleep
