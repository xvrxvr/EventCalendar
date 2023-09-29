#include "wifi_module.h"
#include "setup_data.h"
#include "hadrware.h"

#include <string.h>

#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <lwip/inet.h>

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
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            /* AP connected with MAC event->mac and ID event->aid
                Run WEB server for Setup */
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            /* AP with ID event->aid and MAC event->mac disconnected.
                Stop Setup WEB server */
        }
    }
}

static void event_handler_ip(void* arg, esp_event_base_t, int32_t event_id, void* event_data)
{
    switch(event_id)
    {
        case IP_EVENT_STA_GOT_IP: 
        {
            sta_connected=true; s_retry_num = 0;
            
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            lcd.text2("STA IP: ", 10, 20);
            lcd.text2(inet_ntoa(event->ip_info.ip.addr), 10, 20+16*8);
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
    lcd.text2("AP: http://192.168.4.1:8080", 30, 20);
    

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler_wifi, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler_ip, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(ssid[0] ? WIFI_MODE_APSTA : WIFI_MODE_AP) );

    if (ssid[0]) 
    {
        wifi_config_t wifi_config = {
            .sta = {
                .ssid = MASTER_WIFI_SSID,
                .password = MASTER_WIFI_PASSWD
            }
        };
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = uint8_t(strlen((char*)ssid)),
            .channel = 10,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 1,
        }
    };
    strncpy((char*)wifi_config.ap.ssid, (char*)ssid, sizeof(wifi_config.ap.ssid));
    strncpy((char*)wifi_config.ap.password, (char*)passwd, sizeof(wifi_config.ap.password));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start() );
}
