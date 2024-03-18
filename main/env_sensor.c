#include <env_sensor.h>
#include "esp_err.h"
#include "state.h"

static const char *TAG = "env_sensor";


bmx280_t* bmx280_primary;
bmx280_t* bmx280_secondary;

bool calibrated = false;


void esp_env_task() {
    int cnt = 0;
    float offset_temp = 0, offset_pres = 0; 
    
    ESP_ERROR_CHECK(bmx280_setMode(bmx280_primary, BMX280_MODE_CYCLE));
    ESP_ERROR_CHECK(bmx280_setMode(bmx280_secondary, BMX280_MODE_CYCLE));

    while (1)
    {
        cnt++;

        do {
            vTaskDelay(pdMS_TO_TICKS(1));
        } while(bmx280_isSampling(bmx280_primary) || bmx280_isSampling(bmx280_secondary));

        float temp1 = 0, pres1 = 0, hum1 = 0;
        float temp2 = 0, pres2 = 0, hum2 = 0;
        ESP_ERROR_CHECK(bmx280_readoutFloat(bmx280_primary, &temp1, &pres1, &hum1));
        ESP_ERROR_CHECK(bmx280_readoutFloat(bmx280_secondary, &temp2, &pres2, &hum2));

        if(cnt % 10 == 0) {
            ESP_LOGI(TAG, "Read Values: DeltaTemp = %f, DeltaPraes = %f", temp1 - offset_temp - temp2, pres1 - offset_pres - pres2);
        }

        ac400_set_preasure_diff(abs(pres1-offset_pres-pres2));
        if(ac400_get_status() == OFF)  {
            if (cnt % 30 == 29) { 
                calibrated = true;
                offset_temp = temp1 - temp2;
                offset_pres = pres1 - pres2;
            }
        } else {
            cnt = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(calibrated ? 1000 : 100));
    }
}

void env_start() {
    bmx280_primary = bmx280_create(I2C_NUM_0);
    bmx280_secondary = bmx280_create(I2C_NUM_0);

    if (!bmx280_primary || !bmx280_secondary) { 
        ESP_LOGE("test", "Could not create bmx280 driver.");
        return;
    }

    ESP_ERROR_CHECK(bmx280_init_with_address(bmx280_primary, 0xEC));
    ESP_ERROR_CHECK(bmx280_init_with_address(bmx280_secondary, 0xEE));

    bmx280_config_t bmx_cfg = {
        .iir_filter = BMX280_IIR_NONE,
        .p_sampling = BMX280_PRESSURE_OVERSAMPLING_X16,
        .t_sampling = BMX280_TEMPERATURE_OVERSAMPLING_X16,
        .t_standby = BMP280_STANDBY_4000M,
    };
    ESP_ERROR_CHECK(bmx280_configure(bmx280_primary, &bmx_cfg));
    ESP_ERROR_CHECK(bmx280_configure(bmx280_secondary, &bmx_cfg));

    xTaskCreate(esp_env_task, "Env Tasker", 4096, NULL, 5, NULL);
}