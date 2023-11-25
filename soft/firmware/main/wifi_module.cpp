#include "common.h"
#include "wifi_module.h"
#include "setup_data.h"
#include "hadrware.h"

#include "esp_netif_sntp.h"
#include "esp_sntp.h"

static const char* TAG = "wifi";

static bool sta_connected = false;
static int s_retry_num = 0;

static void event_handler_wifi(void* arg, esp_event_base_t, int32_t event_id, void* event_data)
{
    switch(event_id)
    {
        case WIFI_EVENT_STA_START: esp_wifi_connect(); break;
        case WIFI_EVENT_STA_DISCONNECTED: sta_connected=false; if (s_retry_num < 10) {esp_wifi_connect(); ++s_retry_num;} /* else - schedule pospointed esp_wifi_connect */ break;
        case WIFI_EVENT_AP_STACONNECTED:
        {
            // wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            /* AP connected with MAC event->mac and ID event->aid
                Run WEB server for Setup */
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            // wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            /* AP with ID event->aid and MAC event->mac disconnected.
                Stop Setup WEB server */
            break;
        }
    }
}

static void sync_time_0()
{
    RTC rtc;

    if (!rtc.read() || rtc.year < 20) return;

    struct tm stm{
        .tm_sec = rtc.seconds,
        .tm_min = rtc.minutes,
        .tm_hour = rtc.hours,
        .tm_mday = rtc.date,
        .tm_mon = rtc.month,
        .tm_year = rtc.year+(2000-1900),
        .tm_isdst = 0
    };

    ESP_LOGI(TAG, "System time sync from RTC - %s", asctime(&stm));

    timeval tv{.tv_sec=mktime(&stm)};
    settimeofday(&tv, NULL);
}

static void sync_time_1(timeval *tv)
{
    auto stm = gmtime(&tv->tv_sec);
    if (!stm)
    {
        ESP_LOGE(TAG, "SNTP got invalid time");
        return;
    }
    ESP_LOGI(TAG, "RTC sync from SNTP - %s", asctime(stm));

    RTC rtc;
    rtc.seconds = stm->tm_sec;
    rtc.minutes = stm->tm_min;
    rtc.hours = stm->tm_hour;
    rtc.date = stm->tm_mday;
    rtc.month = stm->tm_mon;
    rtc.year = stm->tm_year - (2000-1900);
    rtc.day = stm->tm_wday;
    rtc.write();
}

static void event_handler_ip(void* arg, esp_event_base_t, int32_t event_id, void* event_data)
{
    switch(event_id)
    {
        case IP_EVENT_STA_GOT_IP: 
        {
            sta_connected=true; s_retry_num = 0;
            
//            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

//            lcd.text("STA IP: ", 0, 60);
//            lcd.text(inet_ntoa(event->ip_info.ip.addr), 8*8, 60);

            esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
            config.sync_cb = sync_time_1;
            esp_netif_sntp_init(&config);
            break;
        }
    }
}

void wifi_init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    if (ssid[0]) 
    {
        esp_netif_create_default_wifi_sta();
    }
    esp_netif_create_default_wifi_ap();
    lcd.text("SSID: '" MASTER_WIFI_SSID "', Passwd: '" MASTER_WIFI_PASSWD "'", 0, 0);
    lcd.text("AP: http://192.168.4.1:8080", 0, 20);
    lcd.text("AP: http://event-calendar[.local]:8080", 0, 40);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler_wifi, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler_ip, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(ssid[0] ? WIFI_MODE_APSTA : WIFI_MODE_AP) );

    if (ssid[0]) 
    {
        ESP_LOGI(TAG, "Connect to SSID '%s', password is '%s'", (char*)ssid, (char*)passwd);

        wifi_config_t wifi_config = {
            .sta = {
                .threshold = {.authmode = WIFI_AUTH_WEP},
                .sae_pwe_h2e = WPA3_SAE_PWE_HUNT_AND_PECK,
                .sae_h2e_identifier = ""
            }
        };
        strcpy((char*)wifi_config.sta.ssid, (char*)ssid);
        strcpy((char*)wifi_config.sta.password, (char*)passwd);
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = MASTER_WIFI_SSID,
            .password = MASTER_WIFI_PASSWD,
            .channel = 10,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 1,
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    sync_time_0();

    ESP_ERROR_CHECK(esp_wifi_start() );
}
