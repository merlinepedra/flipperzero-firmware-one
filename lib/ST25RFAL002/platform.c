#include "platform.h"
#include <assert.h>
#include <furi.h>
#include <furi_hal_spi.h>
#include <st25r3916_irq.h>
#include <rfal_event.h>

typedef struct {
    FuriThread* thread;
    volatile PlatformIrqCallback callback;
    bool need_spi_lock;
} RfalPlatform;

static volatile RfalPlatform rfal_platform = {
    .thread = NULL,
    .callback = NULL,
    .need_spi_lock = true,
};

void nfc_isr(void* _ctx) {
    UNUSED(_ctx);
    if(platformGpioIsHigh(ST25R_INT_PORT, ST25R_INT_PIN)) {
        rfal_event_interrupt_received();
    }
}

void platformEnableIrqCallback() {
    furi_hal_gpio_init(&gpio_nfc_irq_rfid_pull, GpioModeInterruptRise, GpioPullDown, GpioSpeedLow);
    furi_hal_gpio_enable_int_callback(&gpio_nfc_irq_rfid_pull);
}

void platformDisableIrqCallback() {
    furi_hal_gpio_init(&gpio_nfc_irq_rfid_pull, GpioModeOutputOpenDrain, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_disable_int_callback(&gpio_nfc_irq_rfid_pull);
}

void platformSetIrqCallback(PlatformIrqCallback callback) {
    UNUSED(callback);

    furi_hal_gpio_add_int_callback(&gpio_nfc_irq_rfid_pull, nfc_isr, NULL);
    // Disable interrupt callback as the pin is shared between 2 apps
    // It is enabled in rfalLowPowerModeStop()
    furi_hal_gpio_disable_int_callback(&gpio_nfc_irq_rfid_pull);
}

bool platformSpiTxRx(const uint8_t* txBuf, uint8_t* rxBuf, uint16_t len) {
    bool ret = false;
    if(txBuf && rxBuf) {
        ret =
            furi_hal_spi_bus_trx(&furi_hal_spi_bus_handle_nfc, (uint8_t*)txBuf, rxBuf, len, 1000);
    } else if(txBuf) {
        ret = furi_hal_spi_bus_tx(&furi_hal_spi_bus_handle_nfc, (uint8_t*)txBuf, len, 1000);
    } else if(rxBuf) {
        ret = furi_hal_spi_bus_rx(&furi_hal_spi_bus_handle_nfc, (uint8_t*)rxBuf, len, 1000);
    }

    return ret;
}

// Until we completely remove RFAL, NFC works with SPI from rfal_platform_irq_thread and nfc_worker
// threads. Some nfc features already stop using RFAL and work with SPI from nfc_worker only.
// rfal_platform_spi_acquire() and rfal_platform_spi_release() functions are used to lock SPI for a
// long term without locking it for each SPI transaction. This is needed for time critical communications.
void rfal_platform_spi_acquire() {
    platformDisableIrqCallback();
    rfal_platform.need_spi_lock = false;
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_nfc);
}

void rfal_platform_spi_release() {
    furi_hal_spi_release(&furi_hal_spi_bus_handle_nfc);
    rfal_platform.need_spi_lock = true;
    platformEnableIrqCallback();
}

void platformProtectST25RComm() {
    if(rfal_platform.need_spi_lock) {
        furi_hal_spi_acquire(&furi_hal_spi_bus_handle_nfc);
    }
}

void platformUnprotectST25RComm() {
    if(rfal_platform.need_spi_lock) {
        furi_hal_spi_release(&furi_hal_spi_bus_handle_nfc);
    }
}
