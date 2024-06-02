#include "webserver.h"
#include <sys/stat.h>
#include "state.h"

static const char* TAG = "WEBSERVER";
#define BUF_SIZE 8192
#define INDEX_HTML_PATH "/spiffs/index.html"
char response_data[8192];
static const char* status_template = "{ \"mode\": \"%s\", \"machineStatus\": \"%s\", \"temperature\": %d, \"humidity\": %d, \"pmSensor\": %f }";

static void openSpiffs() {
    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}

static esp_err_t readFile(const char* filename, char* buffer, int* bytesRead) {    
    char finalPath[100];
    strcpy(finalPath, "/spiffs");
    if (strcmp(filename, "/") == 0) {
        strcat(finalPath, "/index.html");
    } else {
        strcat(finalPath, filename);
    }

    ESP_LOGI(TAG, "trying to read %s", filename);

    memset((void *)buffer, 0, sizeof(buffer));
    struct stat st;
    if (stat(finalPath, &st))
    {
        ESP_LOGE(TAG, "%s not found", finalPath);
        return ESP_FAIL;
    }

    FILE *fp = fopen(finalPath, "r");
    if (fread(buffer, st.st_size, 1, fp) == 0)
    {
        ESP_LOGE(TAG, "fread failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Loaded %ld bytes", st.st_size);
    *bytesRead = st.st_size+1;
    buffer[st.st_size] = 0;

    fclose(fp);
    return ESP_OK;
}

esp_err_t send_web_page(httpd_req_t *req)
{
    char buffer[BUF_SIZE];
    char filename[1000];
    int bytesRead = 0;
    
    strcpy(filename, req->uri);


    ESP_LOGI(TAG, "Serving Website %s", filename);  
    if(readFile(req->uri, buffer, &bytesRead) == ESP_OK) {
        if(strstr(req->uri, "index.html") || strcmp(req->uri, "/") == 0) {
            char newBuffer[BUF_SIZE];
            int written = sprintf(newBuffer, buffer, get_low_limit(), get_high_limit(), get_afterrun_seconds(), store_read_wlan_ap(), store_read_wlan_pass());
            return httpd_resp_send(req, newBuffer, written);
        } else {
            return httpd_resp_send(req, buffer, bytesRead);
        }
    }
    return ESP_FAIL;
}

esp_err_t get_other_req_handler(httpd_req_t *req)
{
    return send_web_page(req);
}

esp_err_t get_status_req_handler(httpd_req_t *req) {
    char response[200];
    measurements_t measurments = getMeasurements();
    sprintf(response, status_template, getOpModeString(), getStatusString(), measurments.temperature, measurments.humidity, measurments.pm25Level);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
}

esp_err_t get_css_req_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/css");
    return send_web_page(req);
}

esp_err_t get_png_req_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/png");
    return send_web_page(req);
}

esp_err_t get_operation_low_high_handler(httpd_req_t * req) {
    if(strcmp("low", req->user_ctx) == 0) {
        setStatus(ON_LOW);
        setOpMode(API);
    } else if (strcmp("high", req->user_ctx) == 0) {
        setStatus(ON_HIGH);
        setOpMode(API);
    } else if (strcmp("off", req->user_ctx) == 0) {
        setStatus(OFF);
        setOpMode(API);
    } else if (strcmp ("auto", req->user_ctx) == 0) {
        setOpMode(AUTO);
    }

    httpd_resp_set_status(req, "302 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/index.html");
    return httpd_resp_send(req, "Redirect to index", HTTPD_RESP_USE_STRLEN);
}

void handle_limits(char* buffer) {
    char *outer, *inner;    
    char* token = strtok_r(buffer, "&", &outer);
    uint16_t lower = 0, upper = 0, afterrun_seconds = 0;

    while(token != NULL) {
        char *key_value_pair = strtok_r(token, "=", &inner);
        if(strcmp(key_value_pair, "low") == 0) {
            key_value_pair = strtok_r(NULL, "=", &inner);
            lower = atoi(key_value_pair);
        } else if (strcmp(key_value_pair, "high") == 0) {
            key_value_pair = strtok_r(NULL, "=", &inner);
            upper = atoi(key_value_pair);
        }  else if (strcmp(key_value_pair, "afterrun_seconds") == 0) {
            key_value_pair = strtok_r(NULL, "=", &inner);
            afterrun_seconds = atoi(key_value_pair);
        }
        token = strtok_r(NULL, "&", &outer);
    }

    ESP_LOGI(TAG, "New limits: %d - %d - %ds", lower, upper, afterrun_seconds);
    set_limit(lower, upper, afterrun_seconds);    
}

void handle_credentials(char* buffer) {
    char *outer, *inner;    
    char* token = strtok_r(buffer, "&", &outer);
    char ap_name[50];
    char password[50];

    while(token != NULL) {
        char *key_value_pair = strtok_r(token, "=", &inner);
        if(strcmp(key_value_pair, "access_point") == 0) {
            key_value_pair = strtok_r(NULL, "=", &inner);
            strcpy(ap_name, key_value_pair);
        } else if (strcmp(key_value_pair, "password") == 0) {
            key_value_pair = strtok_r(NULL, "=", &inner);
            strcpy(password, key_value_pair);
        }
        token = strtok_r(NULL, "&", &outer);
    }

    ESP_LOGI(TAG, "New Credentials: %s - %s", ap_name, password);
    set_wlan_credentials(ap_name, password);    
    esp_restart();
}

esp_err_t post_handler(httpd_req_t * req) {
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Readback: %s", content);
    if (strstr(req->uri, "limits")) {
        handle_limits(content);
    } else if (strstr(req->uri, "credentials")) {
        handle_credentials(content);
    } 

    /* Send a simple response */
    httpd_resp_set_status(req, "302 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/index.html");
    return httpd_resp_send(req, "Redirect to index", HTTPD_RESP_USE_STRLEN);
}

char* find_delimiter(httpd_req_t* req) {
    char* field = "Content-Type";
    char* boundary = "boundary=";
    size_t len = httpd_req_get_hdr_value_len(req, field);
    if(len == 0) {
        return NULL;
    }

    char header_value[len+1];
    httpd_req_get_hdr_value_str(req, field, header_value, len+1);
    char* new_start = strstr(header_value, boundary);
    if (new_start == NULL) {
        return NULL;
    }

    new_start += strlen(boundary);

    char* buffer_with_padding = malloc(3 + strlen(new_start));

    strcpy(buffer_with_padding, "--");
    strcat(buffer_with_padding, new_start);

    ESP_LOGI(TAG, "Header: %s with len %d", buffer_with_padding, strlen(buffer_with_padding));    
    return buffer_with_padding  ;
}

void ac400_firmware_updater(httpd_req_t *req) {
    bool start_found = false;
    bool end_found = false;
    int bufSize = 100;
    size_t already_fetched = 0;
    char buffer[bufSize];
    esp_ota_handle_t handle;
    esp_partition_t* partition = esp_ota_get_next_update_partition(esp_ota_get_running_partition());
    ESP_LOGI(TAG, "STARTING upload to partition %s", partition->label);

    //ESP_ERROR_CHECK(esp_ota_begin(partition, req->content_len, &handle));
    
    char* needle = find_delimiter(req);
    int segment = 0;
    while (true) {
        if (segment++ % 100 == 0) {
            ESP_LOGI(TAG, "Loading segment %d", segment);
        }

//        size_t request_size = MIN(req->content_len - already_fetched, sizeof(buffer));

        char* effective_buffer = buffer;
        size_t data_received = httpd_req_recv(req, buffer, bufSize);
        
        if (data_received != bufSize) {
            ESP_LOGI(TAG, "Got %d byte but requested %d", data_received, bufSize);
        }

        if (data_received <= 0) {
            ESP_LOGI(TAG, "No more data avail, bailing (code %d), total bytes: %d", data_received, already_fetched);
            break;
        }
        int effective_data_received = data_received;
        if (!start_found) {
            find_result needle_match = find_match(buffer, effective_data_received, needle);
            find_result start_match = find_match(buffer, effective_data_received, "\r\n\r\n");
            if (needle_match.found_at != NULL) {
                ESP_LOGI(TAG, "Found START needle");
            }

            if (start_match.found_at != NULL) {
                start_found = true;
                ESP_LOGI(TAG, "Found START pos");
                effective_buffer = start_match.found_at + 4;
                effective_data_received = effective_data_received - (start_match.found_at - effective_buffer);
            }
        }
       
        if (start_found) {
            find_result end_match = find_match(effective_buffer, effective_data_received, needle);
            if (end_match.found_at != NULL) {
                end_found = true;
                ESP_LOGI(TAG, "Found END pos");
                // remove new line
                
                effective_data_received = end_match.found_at - effective_buffer - 2;
                ESP_LOGI(TAG, "New buffer length: %d", effective_data_received);
                //ESP_LOGI(TAG, "payload: %s",effective_buffer);
                printBuffer(effective_buffer, effective_data_received - 10, 10);
            }
        }
        
        if(effective_data_received <= 0) {
            if (effective_data_received == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            httpd_resp_send_500(req);
        }
        //ESP_ERROR_CHECK(esp_ota_write(handle, effective_buffer, effective_data_received));  
        already_fetched += data_received;
        if (end_found) {
            break;
        }
    }

    printBuffer(buffer, 0, 50);

    //ESP_ERROR_CHECK(esp_ota_end(handle));
    ESP_LOGI(TAG, "DONE upload");
}


esp_err_t post_upload_handler(httpd_req_t *req) {
    ac400_firmware_updater(req);

    httpd_resp_set_status(req, "302 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/index.html");
    return httpd_resp_send(req, "Redirect to index", HTTPD_RESP_USE_STRLEN);
}


httpd_uri_t uri_get_status = {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = get_status_req_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_get_operation_low = {
    .uri = "/operation/on_low",
    .method = HTTP_GET,
    .handler = get_operation_low_high_handler,
    .user_ctx = "low"
};

httpd_uri_t uri_get_operation_high = {
    .uri = "/operation/on_high",
    .method = HTTP_GET,
    .handler = get_operation_low_high_handler,
    .user_ctx = "high"
};

httpd_uri_t uri_get_operation_off = {
    .uri = "/operation/off",
    .method = HTTP_GET,
    .handler = get_operation_low_high_handler,
    .user_ctx = "off"
};

httpd_uri_t uri_get_operation_auto = {
    .uri = "/operation/auto",
    .method = HTTP_GET,
    .handler = get_operation_low_high_handler,
    .user_ctx = "auto"
};

httpd_uri_t uri_post_limits = {
    .uri = "/operation/limits",
    .method = HTTP_POST,
    .handler = post_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_post_credentials = {
    .uri = "/operation/credentials",
    .method = HTTP_POST,
    .handler = post_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_get_style = {
    .uri = "/style.css",
    .method = HTTP_GET,
    .handler = get_css_req_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_get_png = {
    .uri = "/favicon.png",
    .method = HTTP_GET,
    .handler = get_png_req_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_post_upload = {
    .uri = "/update",
    .method = HTTP_POST,
    .handler = post_upload_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_get_wild = {
    .uri = "/*",
    .method = HTTP_GET,
    .handler = get_other_req_handler,
    .user_ctx = NULL
};


httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 40960;
    config.max_open_sockets = 7;
    config.max_uri_handlers = 20;
    config.recv_wait_timeout = 120;
    config.send_wait_timeout = 120;

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_post_credentials);
        httpd_register_uri_handler(server, &uri_post_limits);
        httpd_register_uri_handler(server, &uri_get_status);
        httpd_register_uri_handler(server, &uri_get_operation_low);
        httpd_register_uri_handler(server, &uri_get_operation_high);
        httpd_register_uri_handler(server, &uri_get_operation_off);
        httpd_register_uri_handler(server, &uri_get_operation_auto);
        httpd_register_uri_handler(server, &uri_get_style);
        httpd_register_uri_handler(server, &uri_get_png);     
        httpd_register_uri_handler(server, &uri_post_upload);      
        httpd_register_uri_handler(server, &uri_get_wild);
    }
    return server;
}

void webserver_start() {
    ESP_LOGI(TAG, "starting up");
    openSpiffs();   
    ESP_LOGI(TAG, "starting server");
    setup_server();
}
