#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_pm.h"

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/sens_reg.h"
#include "soc/soc.h"
#include "driver/rtc_io.h"
#include "driver/rtc_cntl.h"
#include "esp32/ulp.h"
#include "sdkconfig.h"
#include "time.h"
#include <sys/time.h>

#include "deep_sleep.h"

#include "ulp_main.h"
#include "ulp_wake.h"
#include "network_common.h"
#include "sntpController.h"
#include "display.h"

static int counting(gpio_evt_msg *message);
static void gpio_task();
static void send_task();
static void ulp_isr(void *arg);
static void realtime_now(int64_t *timestamp_ms);

struct CountValue
{
    int64_t timestamp_ms;
    int count;
};
enum
{
    COUNT_DATA_N_MAX = 20
};
static RTC_DATA_ATTR int count_data_n = 0;
static RTC_DATA_ATTR struct CountValue count_data[COUNT_DATA_N_MAX];
static RTC_NOINIT_ATTR int count;

static const char *TAG = "main";
static SemaphoreHandle_t sema = NULL;

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static void gpio_task()
{
    gpio_evt_msg msg;
    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &msg, portMAX_DELAY))
        {

            printf("event time: %lld pin: %d level %d\n", msg.timestamp_10us, msg.rtc_gpio_num, msg.level);
            counting(&msg);
        }
    }
}

static int counting(gpio_evt_msg *message)
{
    //TODO: intergrate your cornercase handler
    return 0;
}

static void realtime_now(int64_t *timestamp_ms)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    *timestamp_ms = (now.tv_sec * (int64_t)1000) + (now.tv_usec / 1000);
}

static void ulp_isr(void *arg)
{
    portBASE_TYPE pxHigherPriorityTaskWoken = 0;
    xSemaphoreGiveFromISR(sema, &pxHigherPriorityTaskWoken);
}

static void send_task()
{
    while (1)
    {
        if (xSemaphoreTake(sema, portMAX_DELAY))
        {
            restart_sleep_timer();
            send_gpio_evt();
        }
    }
}

void app_main(void)
{
    if (esp_reset_reason() == ESP_RST_POWERON)
    {
        printf("reset count to zero\n");
        count = 0;
        /*start_wifi();
        initializeSntp();
        obtainTime();*/
        init_ulp_program();
    }
    sema = xSemaphoreCreateBinary();
    assert(sema);

    create_ulp_interface_queue();
    xTaskCreatePinnedToCore(gpio_task, "gpio task", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(send_task, "read rtc data", 2048, NULL, 2, NULL, 1);
    send_gpio_evt();
    esp_err_t err = rtc_isr_register(&ulp_isr, NULL, RTC_CNTL_SAR_INT_ST_M);
    ESP_ERROR_CHECK(err);
    REG_SET_BIT(RTC_CNTL_INT_ENA_REG, RTC_CNTL_ULP_CP_INT_ENA_M);

    create_sleep_timer();
    initDisplay();
    char text[20];
    while (1)
    {
        sprintf(text, "count %d", count);
        displayText(text);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
