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
            int written = sprintf(newBuffer, buffer, get_low_limit(), get_high_limit(), get_afterrun_seconds(), get_wlan_ap(), get_wlan_pass());
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
