#include "webserver.h"
#include <sys/stat.h>

static const char* TAG = "WEBSERVER";

#define INDEX_HTML_PATH "/spiffs/index.html"
char response_data[8192];

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
    strcat(finalPath, filename);

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
    *bytesRead = st.st_size;

    fclose(fp);
    return ESP_OK;
}

esp_err_t send_web_page(httpd_req_t *req)
{
    char buffer[8192];
    char filename[1000];
    int bytesRead = 0;
    
    strcpy(filename, req->uri);


    ESP_LOGI(TAG, "Serving Website %s", filename);  
    if(readFile(req->uri, buffer, &bytesRead) == ESP_OK) {
        return httpd_resp_send(req, buffer, bytesRead);
    }
    return ESP_FAIL;
}

esp_err_t get_other_req_handler(httpd_req_t *req)
{
    return send_web_page(req);
}

esp_err_t get_status_req_handler(httpd_req_t *req) {
    char* response = "{ \"mode\": \"bla\", \"machineStatus\": \"status\", \"pmSensors\": [1, 3, 5] }";
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
}

esp_err_t get_css_req_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/css");
    return send_web_page(req);
}

httpd_uri_t uri_get_status = {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = get_status_req_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_get_style = {
    .uri = "/style.css",
    .method = HTTP_GET,
    .handler = get_css_req_handler,
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

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get_status);
        httpd_register_uri_handler(server, &uri_get_style);
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
