#include "common.h"

#include <array>

#include "hadrware.h"
#include "setup_data.h"
#include "web_gadgets.h"
#include "activity.h"
#include "bg_image.h"

#include "web_ajax_classes.h"

static const char *TAG = "web_server";

#define G(id, args) static esp_err_t send_ajax_##id(httpd_req_t *req) {AJAXDecoder_##id(req).run(); return ESP_OK;}
#include "web_actions.inc"

static const char* web_root_url = NULL;
static httpd_handle_t server = NULL;

void set_web_root(const char* url) {web_root_url = url;}

static esp_err_t send_string(httpd_req_t *req)
{
    return httpd_resp_sendstr(req, (const char*)req->user_ctx);
}

static esp_err_t send_root(httpd_req_t *req)
{
    if (!web_root_url)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Entry page not set");
        return ESP_FAIL;
    }
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", web_root_url);
    httpd_resp_sendstr(req, "Redirecting ...");
    return ESP_OK;
}

static esp_err_t send_cdn(httpd_req_t *req)
{
    char fname[64];

    const char* uri = req->uri;
    const char* end = strpbrk(uri, "?#");
    assert(memcmp(uri, "/web/", 5)==0);
    uri += 5;
    if (end)
    {
        size_t len = std::min<size_t>(sizeof(fname)-1, end - uri);
        memcpy(fname, uri, len); fname[len]=0;
        uri = fname;
    }
    Ans(req).write_cdn(uri);
    return ESP_OK;
}

class FDArray {
    std::array<int, SC_MaxWS> fds;
    SemaphoreHandle_t fds_lock;

public:
    class FDIterator {
        const FDArray* owner;
        int index;

        bool eof() {return index >= owner->fds.size();}
        void adv()
        {
            while(!eof() && owner->fds[index] == -1) ++ index;
        }
    public:
        FDIterator(const FDArray* o, int i) : owner(o), index(i) {adv();}

        int operator*() {return eof() ? -1 : owner->fds[index];}

        void operator ++() {if (!eof()) {++index; adv();}}

        bool operator != (const FDIterator& other) {assert(owner == other.owner); return index != other.index;}
    };

    FDArray()
    {
        fds.fill(-1);
        fds_lock = xSemaphoreCreateMutex();
    }

    void add_fd(int fd)
    {
        if (fd == -1) return;
        xSemaphoreTake(fds_lock, portMAX_DELAY);
        for(auto& dst: fds)
        {
            if (dst == -1) {dst = fd; fd=-1; break;}
        }
        xSemaphoreGive(fds_lock);
        if (fd != -1)
        {
            ESP_LOGE(TAG, "WS: FD pool overflow");
        }
    }
    void remove_fd(int fd)
    {
        if (fd == -1) return;
        xSemaphoreTake(fds_lock, portMAX_DELAY);
        for(auto& dst: fds)
        {
            if (dst == fd) {dst = -1; break;}
        }
        xSemaphoreGive(fds_lock);
    }
    bool has_fd() const {return begin() != end();}

    FDIterator begin() const {return FDIterator(this, 0);}
    FDIterator end() const {return FDIterator(this, fds.size());}
};

static FDArray fd_array;

static void ws_async_send(void *arg)
{
    if (!fd_array.has_fd()) return;

    httpd_ws_frame_t ws_pkt{};
    ws_pkt.payload = (uint8_t*)arg;
    ws_pkt.len = strlen((const char*)arg);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    for(int fd: fd_array)
    {
        auto err = httpd_ws_send_frame_async(server, fd, &ws_pkt);
        if (err != ESP_OK)
        {
            if (err != ESP_FAIL)
            {
                ESP_LOGE(TAG, "WS failed: %d", err);
            }
            fd_array.remove_fd(fd);
        }
    }

    free(arg);

    if (!fd_array.has_fd())
    {
        Activity::push_action(Action{.type=AT_WEBEvent, .web={.event=WE_Logout}});
    }
}

void websock_send(const char* msg)
{
    char* m = strdup(msg);
    esp_err_t ret = httpd_queue_work(server, ws_async_send, m);
    if (ret != ESP_OK) 
    {
        free(m);
        ESP_LOGE(TAG, "WS: fail to queue data");
    }
}

static esp_err_t send_bg(httpd_req_t *req)
{
    Ans ans(req);

    const char* uri = req->uri;
    const char* idx = strstr(uri, "/bg/");
    if (!idx)
    {
        ans.send_error(HTTPD_404_NOT_FOUND, "Not found");
        return ESP_OK;
    }
    ans.set_ans_type("*.jpg");
    bg_images.send_bg_image(ans, strtol(idx+4, NULL, 10));
    return ESP_OK;
}


static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WS: Login");
        Activity::push_action(Action{.type=AT_WEBEvent, .web={.event=WE_Login}});
        fd_array.add_fd(httpd_req_to_sockfd(req));
        return ESP_OK;
    }
    // We do not expected anything from our WS - it 'send only', But we still need to read out data from socket.
    httpd_ws_frame_t ws_pkt{};
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    if (ws_pkt.len) 
    {
        ws_pkt.payload = (uint8_t*)malloc(ws_pkt.len);
        if (ws_pkt.payload == NULL) 
        {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        free(ws_pkt.payload);
    }
    return ESP_OK;
}

static esp_err_t start_http_data_server()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.ctrl_port++;
    config.max_uri_handlers = 32;
    config.max_open_sockets = 13;

    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    {
        httpd_uri_t index = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = send_root
        };
        httpd_register_uri_handler(server, &index);
    }

    {
        httpd_uri_t index = {
            .uri       = "/web/*",
            .method    = HTTP_GET,
            .handler   = send_cdn,
        };
        httpd_register_uri_handler(server, &index);
    }

    {
        httpd_uri_t index = {
            .uri       = "/bg/*",
            .method    = HTTP_GET,
            .handler   = send_bg,
        };
        httpd_register_uri_handler(server, &index);
    }

    {
        httpd_uri_t index = {
            .uri        = "/notify",
            .method     = HTTP_GET,
            .handler    = ws_handler,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &index);
    }

#define G(id, args)                                 \
    {                                               \
        httpd_uri_t index = {                       \
            .uri       = "/action/" #id ".html",    \
            .method    = HTTP_GET,                  \
            .handler   = send_ajax_##id,            \
        };                                          \
        httpd_register_uri_handler(server, &index); \
    }

#define P(id, args)                                 \
    {                                               \
        httpd_uri_t index = {                       \
            .uri       = "/action/" #id ".html",    \
            .method    = HTTP_POST,                 \
            .handler   = send_ajax_##id,            \
        };                                          \
        httpd_register_uri_handler(server, &index); \
    }

#include "web_actions.inc"

    return ESP_OK;
}


static const char* control_root_page = R"___(<HTML><TITLE>Event Calendar Setup</TITLE>
<BODY>
<form action="/setup">
SSID: <input type="text" name="ssid"><br>
Password: <input type="text" name="password"><br>
<input type="submit" value="Submit">
</form>
</HTML>
)___";

static const char* setup_ans_page = R"___(<HTML><TITLE>Event Calendar Setup applied</TITLE><BODY><H1>SSID Setup applied. System will be rebooted in 5 second</H1></HTML>)___";
static const char* setup_ans_error = R"___(<HTML><TITLE>Event Calendar Setup failed</TITLE><BODY><H1>Wrong setup data - try again</H1></HTML>)___";

static esp_err_t process_setup(httpd_req_t *req)
{
    char* buf = NULL;
    char param[128];
    do {
        int buf_len = httpd_req_get_url_query_len(req) + 1;
        if (buf_len <= 1) break;
        buf = (char*)malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) != ESP_OK) break;
        ESP_LOGI(TAG, "Setup: URL query => %s", buf);

        if (httpd_query_key_value(buf, "ssid", param, sizeof(param)) != ESP_OK) break;
        example_uri_decode(param, param, 33); param[32] = 0;
        EEPROM::write_pg(ES_SSID, param, 32);

        if (httpd_query_key_value(buf, "password", param, sizeof(param)) != ESP_OK) break;        
        example_uri_decode(param, param, 64); param[63] = 0;
        EEPROM::write_pg(ES_Passwd, param, 64);
        
        free(buf);
        httpd_resp_sendstr(req, setup_ans_page);

        reboot();

        return ESP_OK;
    } while(false);
    if (buf) free(buf);
    return httpd_resp_sendstr(req, setup_ans_error);
}

void reboot()
{
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 5000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,    // Bitmask of all cores
        .trigger_panic = true
    };
    ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));
    esp_task_wdt_add(NULL);
}

static esp_err_t start_http_management_server()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = 8080;

    ESP_LOGI(TAG, "Starting Control HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    httpd_uri_t index = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = send_string,
        .user_ctx  = (void*)control_root_page
    };
    httpd_register_uri_handler(server, &index);

    httpd_uri_t index2 = {
        .uri       = "/index.html",
        .method    = HTTP_GET,
        .handler   = send_string,
        .user_ctx  = (void*)control_root_page
    };
    httpd_register_uri_handler(server, &index2);

    httpd_uri_t setup = {
        .uri       = "/setup",
        .method    = HTTP_GET,
        .handler   = process_setup
    };
    httpd_register_uri_handler(server, &setup);

    return ESP_OK;
}

/* Function to initialize SPIFFS */
static esp_err_t mount_storage(const char* base_path)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = base_path,
        .partition_label = NULL,
        .max_files = 5,   // This sets the maximum number of files that can be open at the same time
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

static void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set("event-calendar");
    mdns_instance_name_set("Event Calendar Box");

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

void start_http_servers()
{
    initialise_mdns();
    netbiosns_init();
    netbiosns_set_name("event-calendar");

    ESP_ERROR_CHECK(mount_storage("/data"));
    ESP_ERROR_CHECK(start_http_management_server());
    ESP_ERROR_CHECK(start_http_data_server());
}
