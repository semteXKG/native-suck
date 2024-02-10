/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: LicenseRef-Included
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Espressif Systems
 *    integrated circuit in a product or a software update for such product,
 *    must reproduce the above copyright notice, this list of conditions and
 *    the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "esp_zigbee_gateway.h"

measurements_t measurements = {
    .humidity = 0,
    .pm25Level = 0,
    .temperature = 0,
    .lastHumidityUpdate = 0,
    .lastPM25Update = 0,
    .lastTempUpdate = 0
};

bind_ctx_t g_bind_ctx = {};
device_t g_device = {};
uint16_t g_clusters[3] = {
    ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
    ESP_ZB_ZCL_CLUSTER_ID_PM2_5_MEASUREMENT,
    ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT
};

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_ERROR_CHECK(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}


static void configureReport(device_t* device, esp_zb_zcl_cluster_id_t cluster, u_int16_t attributeId, esp_zb_zcl_attr_type_t attrType) {
        int reportableChange = 1;
        esp_zb_zcl_config_report_record_t record1 = {
          .attributeID = attributeId,
          .attrType = attrType,
          .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV,
          .min_interval = 0,
          .max_interval = 5,
          .reportable_change = (void*) &reportableChange
        };
      
        esp_zb_zcl_config_report_cmd_t reportCommand = {
            .clusterID = cluster,    
            .zcl_basic_cmd.dst_addr_u.addr_short = device->short_addr,
            .zcl_basic_cmd.src_endpoint = MEASUREMENT_ENDPOINT,
            .zcl_basic_cmd.dst_endpoint = 1,
            .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            .record_number = 1,
            .record_field = &record1   
        };
        esp_zb_zcl_config_report_cmd_req(&reportCommand);
}

int endpointsBound = 0;

static void bind_cb(esp_zb_zdp_status_t zdo_status, void *user_ctx)
{
    device_t* device = (device_t*) user_ctx;

    if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS) {
        endpointsBound++;
        if(endpointsBound < 3) {
            ESP_LOGI(TAG, "Still waiting for endpoints!");
            return; 
        }

        ESP_LOGI(TAG, "All Endpoints Bound successfully!");
        configureReport(device, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, ESP_ZB_ZCL_ATTR_TYPE_S16);
        configureReport(device, ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT, ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID, ESP_ZB_ZCL_ATTR_TYPE_S16);
        configureReport(device, ESP_ZB_ZCL_CLUSTER_ID_PM2_5_MEASUREMENT, ESP_ZB_ZCL_ATTR_PM2_5_MEASUREMENT_MEASURED_VALUE_ID, ESP_ZB_ZCL_ATTR_TYPE_SINGLE);
    }
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p       = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    esp_zb_zdo_signal_device_annce_params_t *dev_annce_params = NULL;
    esp_zb_zdo_signal_macsplit_dev_boot_params_t *rcp_version = NULL;

    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Zigbee stack initialized");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_MACSPLIT_DEVICE_BOOT:
        ESP_LOGI(TAG, "Zigbee rcp device booted");
        rcp_version = (esp_zb_zdo_signal_macsplit_dev_boot_params_t *)esp_zb_app_signal_get_params(p_sg_p);
        ESP_LOGI(TAG, "Running RCP Version: %s", rcp_version->version_str);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Start network formation");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        } else {
            ESP_LOGE(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
        }
        break;
    case ESP_ZB_BDB_SIGNAL_FORMATION:
        if (err_status == ESP_OK) {
            esp_zb_ieee_addr_t ieee_address;
            esp_zb_get_long_address(ieee_address);
            ESP_LOGI(TAG, "Formed network successfully (ieee_address: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d)",
                     ieee_address[7], ieee_address[6], ieee_address[5], ieee_address[4],
                     ieee_address[3], ieee_address[2], ieee_address[1], ieee_address[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel());
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        } else {
            ESP_LOGI(TAG, "Restart network formation (status: %s)", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_FORMATION, 1000);
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Network steering started");
        }
        break;
    case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        dev_annce_params = (esp_zb_zdo_signal_device_annce_params_t *)esp_zb_app_signal_get_params(p_sg_p);
        ESP_LOGI(TAG, "New device commissioned or rejoined (short: 0x%04hx)", dev_annce_params->device_short_addr);


        device_t *tempSensor = (device_t *)malloc(sizeof(device_t));
        tempSensor->endpoint = 1;
        tempSensor->short_addr = dev_annce_params->device_short_addr;
        esp_zb_ieee_address_by_short(tempSensor->short_addr, tempSensor->ieee_addr);

        esp_zb_zdo_bind_req_param_t bind_req;
        memcpy(&(bind_req.src_address), tempSensor->ieee_addr, sizeof(esp_zb_ieee_addr_t));
        bind_req.src_endp = tempSensor->endpoint;
        bind_req.cluster_id = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
        bind_req.dst_addr_mode = ESP_ZB_ZDO_BIND_DST_ADDR_MODE_64_BIT_EXTENDED;
        esp_zb_get_long_address(bind_req.dst_address_u.addr_long);
        bind_req.dst_endp = MEASUREMENT_ENDPOINT;
        bind_req.req_dst_addr = tempSensor->short_addr;
        
        esp_zb_zdo_bind_req_param_t* hum_bind_req = (esp_zb_zdo_bind_req_param_t *)malloc(sizeof(esp_zb_zdo_bind_req_param_t));
        memcpy(hum_bind_req, &bind_req, sizeof(esp_zb_zdo_bind_req_param_t));
        hum_bind_req->cluster_id = ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT;

        esp_zb_zdo_bind_req_param_t* pm25_bind_req = (esp_zb_zdo_bind_req_param_t *)malloc(sizeof(esp_zb_zdo_bind_req_param_t));
        memcpy(pm25_bind_req, &bind_req, sizeof(esp_zb_zdo_bind_req_param_t));
        pm25_bind_req->cluster_id = ESP_ZB_ZCL_CLUSTER_ID_PM2_5_MEASUREMENT;

        esp_zb_zdo_device_bind_req(&bind_req, bind_cb, (void *)tempSensor); 
        esp_zb_zdo_device_bind_req(hum_bind_req, bind_cb, (void *)tempSensor); 
        esp_zb_zdo_device_bind_req(pm25_bind_req, bind_cb, (void *)tempSensor); 

        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
    esp_err_t ret = ESP_OK;
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                        message->info.status);
    ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d)", message->info.dst_endpoint, message->info.cluster,
             message->attribute.id, message->attribute.data.size);
    return ret;
}

static esp_err_t zb_read_attr_handler(esp_zb_zcl_cmd_read_attr_resp_message_t* message) {
        esp_err_t ret = ESP_OK;
        ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
        ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                            message->info.status);
        ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d), type (%d)", message->info.dst_endpoint, message->info.cluster,
                message->variables->attribute.id, message->variables->attribute.data.size, message->variables->attribute.data.type);

        ESP_LOGI(TAG, "Value: %d", *(uint16_t*)message->variables->attribute.data.value);
        return ret;
}

static void printMeasurments() {
    ESP_LOGI(TAG, "Temp: %d (%"PRIu64"); Humidity: %d (%"PRIu64"); PM25 Levels: %f (%" PRIu64 ")", 
    measurements.temperature, measurements.lastTempUpdate, 
    measurements.humidity, measurements.lastHumidityUpdate, 
    measurements.pm25Level, measurements.lastPM25Update);
}

static esp_err_t zb_report_attribute_handler(esp_zb_zcl_report_attr_message_t* message) {
        ESP_LOGD(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d), type (0x%x)", message->status, message->cluster,
                message->attribute.id, message->attribute.data.size, message->attribute.data.type);
        int64_t timestamp = esp_timer_get_time() / 1000;
        if (message->cluster == ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT) {
            short value = *(short*)message->attribute.data.value / 100;
            if (measurements.temperature != value) {
                measurements.temperature = value;
                measurements.lastTempUpdate = timestamp;
                printMeasurments();
            }
        } else if (message->cluster == ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT) {
            short value = *(short*)message->attribute.data.value / 100;
            if (measurements.humidity != value) {
                measurements.humidity = value;
                measurements.lastHumidityUpdate = timestamp;
                printMeasurments();
            }
        } else if (message->cluster == ESP_ZB_ZCL_CLUSTER_ID_PM2_5_MEASUREMENT) {
            float value = *(float*)message->attribute.data.value;
            if (measurements.pm25Level != value) {
                measurements.pm25Level = value;
                measurements.lastPM25Update = timestamp;
                printMeasurments();
            }
        } else {
            if(message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_S16 ) {
                ESP_LOGI(TAG, "Value: %d", *(short*)message->attribute.data.value);
            } else if (message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_SINGLE) {
                ESP_LOGI(TAG, "Value: %f", *(float*)message->attribute.data.value);
            }       
        }
        return ESP_OK;
}

static esp_err_t zb_read_config_response_handler(esp_zb_zcl_cmd_config_report_resp_message_t* message) {
    ESP_LOGI(TAG, "Received Config Response: Status(%d), Cluster(0x%x) Attribute: (0x%x), Attribute Status (0x%x)", message->info.status, message->info.cluster, message->variables->attribute_id, message->variables->status);
    return ESP_OK;
}

static void logDiscoveredAttribute(esp_zb_zcl_disc_attr_variable_t* attribute) {
    if (!attribute) {
        return ;
    }
    ESP_LOGI(TAG, "ID (0x%x), Type: (0x%x)", attribute->attr_id, attribute->data_type);
    logDiscoveredAttribute(attribute->next);
}

static esp_err_t zb_disc_attr_handler(esp_zb_zcl_cmd_discover_attributes_resp_message_t* message) {
    ESP_LOGI(TAG, "Received Attribute Discover: Status: %s", esp_err_to_name(message->info.status));
    logDiscoveredAttribute(message->variables);
    return ESP_OK;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id) {    
    case ESP_ZB_CORE_REPORT_ATTR_CB_ID:
        ret = zb_report_attribute_handler((esp_zb_zcl_report_attr_message_t*) message);
        break;
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    case ESP_ZB_CORE_CMD_READ_ATTR_RESP_CB_ID:
        ret = zb_read_attr_handler((esp_zb_zcl_cmd_read_attr_resp_message_t *)message);
        break;
    case ESP_ZB_CORE_CMD_REPORT_CONFIG_RESP_CB_ID:
        ret = zb_read_config_response_handler((esp_zb_zcl_cmd_config_report_resp_message_t *)message);
        break;
        case ESP_ZB_CORE_CMD_DISC_ATTR_RESP_CB_ID:
        ret = zb_disc_attr_handler((esp_zb_zcl_cmd_discover_attributes_resp_message_t*) message);
        break;
    default:
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}

static void esp_zb_task(void *pvParameters)
{
    /* initialize Zigbee stack with Zigbee coordinator config */
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZC_CONFIG();
    esp_zb_init(&zb_nwk_cfg);

    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
  
    esp_zb_temperature_meas_cluster_cfg_t temp_cfg = {
        .measured_value = 0x954d,
        .min_value = 0x954d,
        .max_value = 0x7fff,
    };

    esp_zb_humidity_meas_cluster_cfg_t humi_cfg = {
        .measured_value = ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_MIN_VALUE_MIN_VALUE + 1,
        .min_value = ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_MIN_VALUE_MIN_VALUE,
        .max_value = ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_MAX_VALUE_MAX_VALUE,
    };

    esp_zb_pm2_5_measurement_cluster_cfg_t pm25Cfg = {
        .measured_value = ESP_ZB_ZCL_PM2_5_MEASUREMENT_MIN_MEASURED_VALUE_MIN_VALUE + 1,
        .min_measured_value = ESP_ZB_ZCL_PM2_5_MEASUREMENT_MIN_MEASURED_VALUE_MIN_VALUE,
        .max_measured_value = ESP_ZB_ZCL_PM2_5_MEASUREMENT_MAX_MEASURED_VALUE_MAX_VALUE,
    };
    
    esp_zb_cluster_list_add_basic_cluster(cluster_list, esp_zb_basic_cluster_create((NULL)), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
    esp_zb_cluster_list_add_identify_cluster(cluster_list, esp_zb_identify_cluster_create((NULL)), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
    esp_zb_cluster_list_add_temperature_meas_cluster(cluster_list, esp_zb_temperature_meas_cluster_create(&(temp_cfg)),
                                                     ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
    esp_zb_cluster_list_add_humidity_meas_cluster(cluster_list, esp_zb_humidity_meas_cluster_create(&(humi_cfg)), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
    esp_zb_cluster_list_add_pm2_5_measurement_cluster(cluster_list, esp_zb_pm2_5_measurement_cluster_create(&(pm25Cfg)), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);


    esp_zb_ep_list_add_ep(ep_list, cluster_list, MEASUREMENT_ENDPOINT, ESP_ZB_AF_HA_PROFILE_ID, ESP_ZB_HA_TEST_DEVICE_ID);
    esp_zb_device_register(ep_list);

    /* initiate Zigbee Stack start without zb_send_no_autostart_signal auto-start */
    esp_zb_core_action_handler_register(zb_action_handler);
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_main_loop_iteration();
}

void zigbee_start()
{
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    /* load Zigbee gateway platform config to initialization */
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
#if CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(example_connect());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
#endif
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}
