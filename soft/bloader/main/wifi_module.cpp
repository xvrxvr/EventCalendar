#include "common.h"
#include "wifi_module.h"
#include "setup_data.h"
#include "hadrware.h"

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

uint32_t my_ip;

static void event_handler_ip(void* arg, esp_event_base_t, int32_t event_id, void* event_data)
{
    switch(event_id)
    {
        case IP_EVENT_STA_GOT_IP: 
        {
            sta_connected=true; s_retry_num = 0;
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            my_ip = event->ip_info.ip.addr;

            lcd.gotoxy(LCD_STA_ROW);
            lcd.set_fg(rgb(0, 0xFF, 0));
            lcd.print("IP: ");
            lcd.print(inet_ntoa(event->ip_info.ip.addr));
            lcd.print("\nAKA: event-calendar[.local]");
            lcd.set_fg(rgb(0xFF, 0xFF, 0xFF));

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

    ESP_ERROR_CHECK(esp_wifi_start() );
}
