#include <nvs_store.h>

#define DEFAULT_LOW_LIMIT 50;
#define DEFAULT_HIGH_LIMIT 100;

#define LOW_LIMIT "low_limit"
#define HIGH_LIMIT "high_limit"
#define AFTERRUN_SECONDS "afterrun"
#define AP_NAME "username"
#define PASSWORD "password"
#define LAST_OPMODE "last_opmode"
#define UDP_SERVER "udp_server"
#define UDP_PORT "udp_port"

int16_t cached_low_limit = 0;
int16_t cached_high_limit = 0;
int16_t cached_afterrun_seconds = 0;
int16_t cached_udp_port = 0;

char cached_ap_name[30];
char cached_password[30];
char cached_udp_server[20];


int16_t cached_lastOpmode = 0;

static const char* TAG = "STORE";

nvs_handle_t my_handle;

void readInt(char* key, int16_t* values) {
    esp_err_t err = nvs_get_i16(my_handle, key, values);
    if(err != 0) {
        ESP_LOGW(TAG, "Error reading: %s", esp_err_to_name(err));
    }
}

void readString(char* key, char* buffer, char* def_value) {
    size_t length = 30;
    esp_err_t err = nvs_get_str(my_handle, key, buffer, &length);
    if(err != 0) {
        ESP_LOGW(TAG, "Error reading: %s", esp_err_to_name(err));
        strcpy(buffer, def_value);
    }
}

void writeInt(char* key, int16_t value) {
    esp_err_t err = nvs_set_i16(my_handle, key, value);
    if(err != 0) {
        ESP_LOGW(TAG, "Error writing: %s", esp_err_to_name(err));
    }
}

void writeString(char* key, char* value) {
    esp_err_t err = nvs_set_str(my_handle, key, value);
    if(err != 0) {
        ESP_LOGW(TAG, "Error writing: %s", esp_err_to_name(err));
    }
}


void store_start() {
    esp_err_t err = nvs_open("sucker", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "NVS Mounted");
        cached_low_limit = DEFAULT_LOW_LIMIT;
        cached_high_limit = DEFAULT_HIGH_LIMIT;
        readInt(LOW_LIMIT, &cached_low_limit);
        readInt(HIGH_LIMIT, &cached_high_limit);
        readInt(AFTERRUN_SECONDS, &cached_afterrun_seconds);
        readInt(LAST_OPMODE, &cached_lastOpmode);
        readInt(UDP_PORT, &cached_udp_port);

        readString(AP_NAME, cached_ap_name, "");
        readString(PASSWORD, cached_password, "");
        readString(UDP_SERVER, cached_udp_server, "");

        ESP_LOGI(TAG, "Read lower [%d] and higher [%d] with afterrun [%d]", cached_low_limit, cached_high_limit, cached_afterrun_seconds);
        ESP_LOGI(TAG, "Read user [%s] with password [%s]", cached_ap_name, cached_password);
        ESP_LOGI(TAG, "Read UDP log server [%s] with port [%d]", cached_udp_server, cached_udp_port);
        ESP_LOGI(TAG, "Read last OpMode as [%s]", OpModeStr[cached_lastOpmode]);
    }        
}


void set_limit(uint16_t low_limit, uint16_t high_limit, uint16_t afterrun_seconds) {
    writeInt(LOW_LIMIT, low_limit);
    writeInt(HIGH_LIMIT, high_limit);
    writeInt(AFTERRUN_SECONDS, afterrun_seconds);
    cached_low_limit = low_limit;
    cached_high_limit = high_limit;
    cached_afterrun_seconds = afterrun_seconds;
    nvs_commit(my_handle);
}

uint16_t get_low_limit() {
    return cached_low_limit;
}

uint16_t get_high_limit() {
    return cached_high_limit;
}

uint16_t get_afterrun_seconds() {
    return cached_afterrun_seconds;
}

void set_wlan_credentials(char* ap_name, char* password) {
    writeString(AP_NAME, ap_name);
    writeString(PASSWORD, password);
    strcpy(cached_ap_name, ap_name);
    strcpy(cached_password, password);
    nvs_commit(my_handle);
}

char* store_read_wlan_ap(){
    return cached_ap_name;
}
char* store_read_wlan_pass(){
    return cached_password;
}

void store_save_last_op_mode(enum OpMode opMode) {
    cached_lastOpmode = (int16_t) opMode;
    writeInt(LAST_OPMODE, cached_lastOpmode);
}

enum OpMode store_read_last_op_mode() {
    return (enum OpMode) cached_lastOpmode;
}

char* store_read_udp_server() {
    return cached_udp_server;
}

uint16_t store_read_udp_port() {
    return cached_udp_port;
}

void store_set_udp_server(char* server, uint16_t port) {
    writeInt(UDP_PORT, port);
    writeString(UDP_SERVER, server);
    strcpy(cached_udp_server, server);
    cached_udp_port = port;
    nvs_commit(my_handle);
}