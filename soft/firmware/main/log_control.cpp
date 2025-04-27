#include "common.h"
#include "web_gadgets.h"
#include "setup_data.h"

#include <cJSON.h>
#include "lwip/sockets.h"
#include "esp_core_dump.h"

#include "log_control.h"

#include <esp_core_dump.h>

static const char* TAG = "LogMgr";

static vprintf_like_t prev_logger;
static bool remote_logger_active = false;
static uint32_t data_sent = 0;
static uint32_t max_remote_buffer = 0;
static int log_spliced = 0;

static TaskHandle_t log_task_handle;
static SemaphoreHandle_t log_sema;
static Prn log_bufs[2];
static int log_buf_idx=0;

static bool locked = true; // Limit max Prn buffer size to 1K

// JSON format (object):
// {
//    IP: '192.168.0.100',
//    UART: true,
//    Remote: true,
//    DefLL: 'Debug',
//    SoftLim: true,
//    MemLim: 1,
//    Locked: true,
//    Custom: [
//      {tag: 'Tag1', level:'Error'},
//      {tag: 'Tag2', level:'None'},
//      {tag: 'Tag3', level:'Verbose'}
//    ]
// }

static constexpr const char* log_file_name = "/data/logsetup.json";

static constexpr esp_log_level_t str2ll(const char* log_level)
{
    switch(upcase(log_level[0]))
    {
        case 'N': return ESP_LOG_NONE;   // None
        case 'E': return ESP_LOG_ERROR;  // Error
        case 'W': return ESP_LOG_WARN;   // Warning
        case 'I': return ESP_LOG_INFO;   // Info
        case 'D': return ESP_LOG_DEBUG;  // Debug
        case 'V': return ESP_LOG_VERBOSE;// Verbose
        default: return ESP_LOG_NONE;
    }
}

static constexpr const char* ll2str(esp_log_level_t ll)
{
    static constexpr const char* msg[] = {"None", "Error", "Warning", "Info", "Debug", "Verbose"};
    return msg[ll];
}

class LogLock {
public:
    LogLock() {xSemaphoreTake(log_sema, portMAX_DELAY);}
    ~LogLock() {xSemaphoreGive(log_sema);}
};
#define LOCK LogLock _ll

#define GET(nm, tp) obj_item = cJSON_GetObjectItem(J, nm); \
    if (!obj_item) \
    { \
        ESP_LOGE(TAG, "Invalid JSON - Root value \"" nm "\" not found"); \
        return; \
    } \
    if (!cJSON_Is##tp(obj_item)) \
    { \
        ESP_LOGE(TAG, "Invalid JSON - Root value \"" nm "\" not an " #tp); \
        return; \
    }

#define GET_B(var, nm) GET(nm, Bool) bool var = cJSON_IsTrue(obj_item)
#define GET_S(var, nm) GET(nm, String) const char* var = cJSON_GetStringValue(obj_item)
#define GET_I(var, nm) GET(nm, Number) int var = obj_item->valueint

inline uint32_t prn_buf_size()
{
    uint32_t size = global_setup.log_memsize_limit*256;
    return (locked && size > SC_LogStartLim) ? SC_LogStartLim : size;
}

static void process_defaults_level()
{
    esp_log_level_set("*", esp_log_level_t(global_setup.log_setup_options&LSO_DefLogLevelMask));
}

// Process setup for Log Level (from WEB and initial one)
static void process_custom_levels(cJSON* config)
{
    cJSON* J;
    cJSON* obj_item;
    esp_log_level_t logl = esp_log_level_t(global_setup.log_setup_options&LSO_DefLogLevelMask);

    process_defaults_level();
    if (!config)
    {
        ESP_LOGE(TAG, "Invalid Custom Setup JSON - Not found");
        return;
    }
    if (!cJSON_IsArray(config))
    {
        ESP_LOGE(TAG, "Invalid Custom Setup JSON - Not an array");
        return;
    }
    cJSON_ArrayForEach(J, config)
    {
        GET_S(tag, "tag");
        GET_S(level, "level");
        auto ll = str2ll(level);
        esp_log_level_set(tag, ll);
        logl = std::max(logl, ll);
    }
    esp_log_set_level_master(logl);
}

static void log_stop_ip_logger()
{
    if (!log_task_handle) return;
    if (!remote_logger_active) {vTaskDelete(log_task_handle); log_task_handle=NULL; return;}
    xTaskNotify(log_task_handle, 55, eSetValueWithOverwrite);
    for(int i=0; i<100; ++i)
    {
        vTaskDelay(100);
        if (!log_task_handle) return;
    }
    vTaskDelete(log_task_handle); 
    log_task_handle=NULL;
    remote_logger_active = false;
}

static void log_task( void * )
{
    int sock = -1;

    auto write = [&](const char* data, size_t size) {
        if (sock == -1) return;
        if (send(sock, data, size, 0) != size)
        {
            close(sock);
            sock = -1;
            remote_logger_active = false;
            printf("ERROR: RemoteLogger: Connection lost (or write error)");
        }
    };

    for(;;)
    {
        if (sock == -1)
        {
            struct sockaddr_in dest_addr;

            dest_addr.sin_addr.s_addr = global_setup.log_ip;
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(SC_LogPort);

            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if (sock < 0) 
            {
                printf("ERROR: RemoteLogger: Unable to create socket");
                vTaskDelay(1000);
                continue;
            }

            int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
            if (err) 
            {
          	    //printf("ERROR: RemoteLogger: Socket unable to connect: errno %d", errno);
                close(sock);
                sock = -1;
                vTaskDelay(s2ticks(SC_ConnectTout));
                continue;
            }
        }
        remote_logger_active = true;
        uint32_t index = ulTaskNotifyTake(pdTRUE, ms2ticks(SC_LogTaskWKUP));
        if (index == 55)
        {
            shutdown(sock, 0);
            close(sock);
            sock = -1;
            remote_logger_active = false;
            log_task_handle = NULL;
            vTaskDelete(NULL);
            return;
        }
        {
            LOCK;
            if (log_bufs[log_buf_idx].length()==0) continue;
            log_buf_idx ^= 1;
            auto& P = log_bufs[log_buf_idx];
            data_sent += P.length();
            max_remote_buffer = std::max<uint32_t>(max_remote_buffer, P.length());
        }
        auto& P = log_bufs[log_buf_idx];
        if (P.was_spliced()) {++log_spliced; write("...\n", 4);}
        write(P.c_str(), P.length());
        if (prn_buf_size() && P.length() > prn_buf_size()) P.zap();
        else P.clear();
    }
}

static void log_start_ip_logger()
{
    if (log_task_handle) return;
    xTaskCreate(log_task, "LogSender", TSS_LogSender, NULL, TP_LogSender, &log_task_handle);
}


static int my_logger(const char *fmt, va_list lst)
{
    static bool in_sender = false;
    int result = 0;
    if (global_setup.log_setup_options & LSO_UseUART)
    {
        va_list cpy;
        va_copy(cpy, lst);
        result = prev_logger(fmt, cpy);
        va_end(cpy);
    }
    if (global_setup.log_ip && global_setup.log_setup_options&LSO_UseIP)
    {
        if (!in_sender)
        {
            LOCK;
            in_sender = true;
            auto & L = log_bufs[log_buf_idx];
            int ll = L.length();
            L.cat_vprintf(fmt, lst);
            result = L.length() - ll;
            if (prn_buf_size() && (!remote_logger_active || !(global_setup.log_setup_options&LSO_SoftLimit)))
            {
                L.fit_in(prn_buf_size());
            }
            if (log_task_handle) xTaskNotify(log_task_handle, 1, eSetValueWithOverwrite);
            in_sender = false;
        }
        else
        {
            printf("ERROR: RemoteLogger: Recursive call to PRN function\n");
        }    
    }
    va_end(lst);
    return result;
}


#define JSON_PARSER(str) std::unique_ptr<cJSON, void (*)(cJSON *item)> JJ(cJSON_Parse(str), cJSON_Delete); cJSON* J = JJ.get()

// Extract info from EEPROM and file and setup Log
static bool process_initial_log_setup_imp()
{
    log_sema = xSemaphoreCreateMutex();
    prev_logger = esp_log_set_vprintf(my_logger);
    if (global_setup.log_ip && global_setup.log_setup_options&LSO_UseIP) log_start_ip_logger();

    Prn dst;
    if (read_file(dst, log_file_name))
    {
        JSON_PARSER(dst.c_str());
        process_custom_levels(J);
    }
    else
    {
        process_defaults_level();
    }
    return true;
}

void process_initial_log_setup()
{
    static bool dummy = process_initial_log_setup_imp();
    (void) dummy;
}

// Process JSON with Log setup from WEB
void process_log_setup(const char* json)
{
    process_initial_log_setup();
    JSON_PARSER(json);
    if (!J)
    {
        ESP_LOGE(TAG, "Invalid JSON - Parse failed");
        return;
    }

    cJSON* obj_item;

    GET_B(uart, "UART");
    GET_B(remote, "Remote");
    GET_B(soft_limit, "SoftLim");
    GET_S(ip, "IP");
    GET_S(def_ll, "DefLL");
    GET_I(mem_lim, "MemLim");
    GET_B(lck, "Locked");
    locked = lck;

    bool upd_glob_setup = false;
    bool upd_ip_setup = false;

#define BIT_SET(loc_name, opt_name, pref) \
    if (loc_name != (global_setup.log_setup_options&opt_name)) \
    {                                                          \
        global_setup.log_setup_options ^= opt_name;            \
        pref upd_glob_setup = true;                            \
    }
#define VAL_SET(loc_name, opt_name, pref) \
    if (loc_name != global_setup.opt_name)     \
    {                                          \
        global_setup.opt_name = loc_name;      \
        pref upd_glob_setup = true;            \
    }

    BIT_SET(remote, LSO_UseIP, upd_ip_setup =)
    BIT_SET(uart,  LSO_UseUART, )
    BIT_SET(soft_limit, LSO_SoftLimit,)
    VAL_SET(mem_lim, log_memsize_limit,)
    auto default_log_level = str2ll(def_ll);
    if (default_log_level != (global_setup.log_setup_options&LSO_DefLogLevelMask))
    {
        global_setup.log_setup_options &= ~LSO_DefLogLevelMask;
        global_setup.log_setup_options |= default_log_level;
        upd_glob_setup = true;
    }
    uint32_t ip_addr = inet_addr(ip);
    VAL_SET(ip_addr, log_ip, upd_ip_setup = )

    if (upd_glob_setup) global_setup.sync();
    if (upd_ip_setup)
    {
        log_stop_ip_logger();
        if (remote) log_start_ip_logger();
    }

    auto custom = cJSON_GetObjectItem(J, "Custom");
    process_custom_levels(custom);
    if (!custom) return;

    std::unique_ptr<FILE, int (*)(FILE*)> f(fopen(log_file_name, "wt"), fclose);
    if (!f)
    {
        ESP_LOGE(TAG, "Can't open file to store Custom Log setup");
        return;
    }
    std::unique_ptr<char, void (*)(void*)> json_str(cJSON_PrintUnformatted(custom), free);
    fputs(json_str.get(), f.get());
}

void log_send_setup(Ans& ans)
{
    process_initial_log_setup();
    ans << UTF8 << "{IP:'" << (global_setup.log_ip ? inet_ntoa(global_setup.log_ip) : "") << "',";
#define BIT_VAL(json, fld) ans << UTF8 << #json ":" << (global_setup.log_setup_options & fld ? "true":"false") << ","
#define INT_VAL(json, fld) ans << UTF8 << #json ":" << global_setup.fld << ","
    BIT_VAL(UART, LSO_UseUART);
    BIT_VAL(Remote, LSO_UseIP);
    BIT_VAL(SoftLim, LSO_SoftLimit);
    INT_VAL(MemLim, log_memsize_limit);
#define LL(json, val) ans << UTF8 << #json ":'" << ll2str(esp_log_level_t(val)) << "',"
    LL(DefLL, global_setup.log_setup_options & LSO_DefLogLevelMask);
    ans << UTF8 << "Locked:" << (locked ? "true,":"false,");
    ans << UTF8 << "Custom:";
    Prn dst;
    if (read_file(dst, log_file_name)) ans << UTF8 << dst.c_str();
    else ans << UTF8 << "[]";
    ans << UTF8 << "}";
}

void log_send_status(Ans& ans, bool with_clear)
{
    process_initial_log_setup();
    LOCK;
    ans << UTF8 
        << "{"
            << "RemoteActive:" << (remote_logger_active ? "true," : "false,")
            << "Locked:" << (locked ? "true,":"false,")
            << "DataSent:" << data_sent << ","
            << "MaxRemoteBuffer:" << max_remote_buffer << ","
            << "Sliced:" << log_spliced << ","
            << "CoreDump:" << (esp_core_dump_image_check() == ESP_OK ? "true" : "false")
        << "}";
    if (with_clear)
    {
        data_sent = 0;
        max_remote_buffer = 0;
        log_spliced = 0;
    }
}

extern "C" esp_err_t esp_core_dump_partition_and_size_get(const esp_partition_t **partition, uint32_t* size);

void log_send_coredump(Ans& ans, char* buf)
{
    const esp_partition_t *pt;
    uint32_t size;
    if (ESP_OK != esp_core_dump_image_check() || ESP_OK != esp_core_dump_partition_and_size_get(&pt, &size))
    {
        ans.send_error(HTTPD_404_NOT_FOUND, "No core file");
        return;
    }
    size_t shift = 24; // Header is 6 4byte words
    ans.set_ans_type("code.elf");
    sprintf(buf, "%lu", size);
    ans.set_hdr("content-length", buf);
    while(size)
    {
        uint32_t rest = std::min<uint32_t>(size, SC_CoreFileChunk);
        if (ESP_OK != esp_partition_read(pt, shift, buf, rest))
        {
            ans.send_error(HTTPD_500_INTERNAL_SERVER_ERROR, "Error reading Core partition");
            return;
        }
        ans.write_string_utf8(buf, rest);
        shift += rest;
        size -= rest;
    }
}

// Send accumulated so far Log
void log_send_data(Ans& ans)
{
    process_initial_log_setup();
    Prn buf;
    {
        LOCK;
        if (log_bufs[log_buf_idx].length()==0) {ans << UTF8 << "-"; return;}
        log_buf_idx ^= 1;
        auto& P = log_bufs[log_buf_idx];
        data_sent += P.length();
        max_remote_buffer = std::max<uint32_t>(max_remote_buffer, P.length());
        if (P.was_spliced()) ++log_spliced;
        buf.swap(P);
    }
    if (buf.was_spliced()) {ans << UTF8 << "...<br>";}
    ans << UTF8 << "<pre>";
    // replace:
    //  < => &lt;
    //  > => &gt;
    //  & => &amp;
    //  '\n' => <br>
    //  \033[ ... m => ANSI colors
    // ANSI colors is <color>{';'<color>}
    //  <color> is:
    //   30..39 - FG colors
    //   40..49 - BG color
    //   0..4   - Styles
    //  => <span class="ansi-30 ansi-40 ansi-1 ..."> ... </span>
    //  \033[0m - Close <span>
    const char* p = buf.c_str();
    bool in_ansi_span = false;
    while(p)
    {
        const char* e = strpbrk(p, "<>&\033");
        if (!e) {ans << UTF8 << p; break;}
        if (e != p) {ans.write_string_utf8(p, e-p); p=e;}
        switch(*p++)
        {
            case '<': ans << UTF8 << "&lt;"; break;
            case '>': ans << UTF8 << "&gt;"; break;
            case '&': ans << UTF8 << "&amp;"; break;
            case '\n': ans << UTF8 << "<br>"; break;
            default: 
                if (memcmp(p, "[0m", 3)==0) 
                {
                    if (in_ansi_span) ans << UTF8 << "</span>";
                    in_ansi_span=false;
                    p+=3;
                    break;
                }
                if (*p != '[') break;
                e = strchr(++p, 'm');
                if (!e) break;
                // <digits>{';'<digits>}m   => <span class="ansi-<digits>{ ansi-<digits>}">
                if (in_ansi_span) ans << UTF8 << "</span>";
                in_ansi_span = true;
                ans << UTF8 << "<span class=\"ansi-";
                for(;p != e; ++p)
                {
                    if (*p == ';') ans << UTF8 << " ansi-";
                    else ans.write_string_utf8(p,1);
                }
                ans << UTF8 << "\">";
                p=e+1;
                break;
        }
    }
    ans << UTF8 << "</pre>";

/*
#define LOG_ANSI_COLOR_BLACK                                        "30"
#define LOG_ANSI_COLOR_RED                                          "31"
#define LOG_ANSI_COLOR_GREEN                                        "32"
#define LOG_ANSI_COLOR_YELLOW                                       "33"
#define LOG_ANSI_COLOR_BLUE                                         "34"
#define LOG_ANSI_COLOR_MAGENTA                                      "35"
#define LOG_ANSI_COLOR_CYAN                                         "36"
#define LOG_ANSI_COLOR_WHITE                                        "37"
#define LOG_ANSI_COLOR_DEFAULT                                      "39"
// Macros for defining background colors.
#define LOG_ANSI_COLOR_BG_BLACK                                     "40"
#define LOG_ANSI_COLOR_BG_RED                                       "41"
#define LOG_ANSI_COLOR_BG_GREEN                                     "42"
#define LOG_ANSI_COLOR_BG_YELLOW                                    "43"
#define LOG_ANSI_COLOR_BG_BLUE                                      "44"
#define LOG_ANSI_COLOR_BG_MAGENTA                                   "45"
#define LOG_ANSI_COLOR_BG_CYAN                                      "46"
#define LOG_ANSI_COLOR_BG_WHITE                                     "47"
#define LOG_ANSI_COLOR_BG_DEFAULT                                   "49"
// Macros for defining text styles like bold, italic, and underline.
#define LOG_ANSI_COLOR_STYLE_RESET                                  "0"      << reset all ANSI colors to default (all color lines ended by this)
#define LOG_ANSI_COLOR_STYLE_BOLD                                   "1"
#define LOG_ANSI_COLOR_STYLE_ITALIC                                 "3"
#define LOG_ANSI_COLOR_STYLE_UNDERLINE                              "4"
*/

}
