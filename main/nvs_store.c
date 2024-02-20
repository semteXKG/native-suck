#include <nvs_store.h>

#define DEFAULT_LOW_LIMIT 50;
#define DEFAULT_HIGH_LIMIT 100;

#define LOW_LIMIT "low_limit"
#define HIGH_LIMIT "high_limit"
#define AFTERRUN_SECONDS "afterrun"
#define AP_NAME "username"
#define PASSWORD "password"

int16_t cached_low_limit;
int16_t cached_high_limit;
int16_t cached_afterrun_seconds;

char cached_ap_name[30];
char cached_password[30];

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
        readString(AP_NAME, cached_ap_name, "");
        readString(PASSWORD, cached_password, "");

        ESP_LOGI(TAG, "Read lower [%d] and higher [%d] with afterrun [%d]", cached_low_limit, cached_high_limit, cached_afterrun_seconds);
        ESP_LOGI(TAG, "Read user [%s] with password [%s]", cached_ap_name, cached_password);
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

char* get_wlan_ap(){
    return cached_ap_name;
}
char* get_wlan_pass(){
    return cached_password;
}