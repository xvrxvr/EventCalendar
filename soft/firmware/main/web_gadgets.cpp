#include "estring.h"

#include <esp_log.h>

#include "web_gadgets.h"
#include "web_files_access.h"
#include "web_vars.h"

static const char* TAG = "web_gadgets";

Ans::Ans(httpd_req_t *r) :req(r), buffer(new char[BufSize]) {}

Ans::~Ans() 
{
    if (chunked_active) {flush(); httpd_resp_send_chunk(req, NULL, 0);} else
    if (buf_filled) httpd_resp_send(req, buffer.get(), buf_filled);
}

void Ans::write_int(uint32_t val) {char b[11]; sprintf(b, "%ld", val); write_string_utf8(b);}

void Ans::write_string_utf8(const char* ptr, int length)
{
    if (length == -1) length = strlen(ptr);
    while(length)
    {
        int to_send = std::min(length, BufSize - buf_filled);
        if (!to_send) {flush(); continue;}
        memcpy(buffer.get() + buf_filled, ptr, to_send);
        length -= to_send;
        ptr += to_send;
    }
}

void Ans::write_string_dos(const char* ptr, int length)
{
    if (length == -1) length = strlen(ptr);
    const char* end = ptr+length;
    while(ptr < end)
    {
        const char* b = ptr;
        while((*ptr & 0x80) == 0 && ptr < end) ++ptr;
        write_string_utf8(b, ptr-b);
        if (ptr < end) // We have DOS symbol
        {
            uint8_t s = *ptr++;
            char buf[2];
            if (s <= 0xAF) {buf[0] = 0xD0; buf[1] = s+0x10;} else
            if (s <= 0xEF) {buf[0] = 0xD1; buf[1] = s-0x60;} else
            if (s == 0xF0) {buf[0] = 0xD0; buf[1] = 0x81;}
            else {buf[0] = 0xD1, buf[1] = 0x91;}
            write_string_utf8(buf, 2);
        }
// 80-AF -> 0410-043F                | D0 90 - D0 BF
// B0-BF -> 2591 2592 2593 2502 2524 2561 2562 2556 2555 2563 2551 2557 255D 255C 255B 2510   X    
// C0-CF -> 2514 2534 252C 251C 2500 253C 255E 255F 255A 2554 2569 2566 2560 2550 256C 2567   X
// D0-DF -> 2568 2564 2565 2559 2558 2552 2553 256B 256A 2518 250C 2588 2584 258C 2590 2580   X
// E0-EF -> 0440-044F                | D1 80 - D1 8F
// F0 F1 -> 0401 0451                | D0 81,  D1 91
// F0-FF -> 0401 0451 0404 0454 0407 0457 040E 045E 00B0 2219 00B7 221A 2116 00A4 25A0 0020   X
    }    
}

esp_err_t Ans::set_ans_type(const char* filename)
{
#define IS_FILE_EXT(filename, ext) (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)
#define FE(ext, tp) if (IS_FILE_EXT(filename, ext)) return httpd_resp_set_type(req, tp)
    FE(".html", "text/html");
    FE(".htm", "text/html");
    FE(".jpeg", "image/jpeg");
    FE(".ico",  "image/x-icon");
    FE(".png",  "image/png");
    FE(".css", 	"text/css");
    FE(".js",   "text/javascript");
    FE(".json",	"application/json");
    FE(".svg",  "image/svg+xml");
    FE(".xml",  "application/xml");
    return httpd_resp_set_type(req, "text/plain");
}
#undef FE
#undef IS_FILE_EXT


void Ans::send_error(httpd_err_code_t err_code, const char* message)
{
    httpd_resp_send_err(req, err_code, message);
}

void Ans::set_hdr(const char* tag, const char* value)
{
    httpd_resp_set_hdr(req, tag, value);
}


void Ans::write_cdn(const char* fname)
{
    CDNDef cdn = decode_web_files_access_function(fname);
    if (!cdn.start)
    {
        send_error(HTTPD_404_NOT_FOUND, "File not found");
        return;
    }
    if (cdn.start[0] != '$') // Send it directly
    {
        httpd_resp_send(req, cdn.start, cdn.end - cdn.start);
        return;
    }
    set_hdr("Cache-Control", "no-store, no-cache, max-age=0, must-revalidate, proxy-revalidate");
    ++cdn.start;
    auto cdn_org = cdn.start;
    while(cdn.start < cdn.end)
    {
        const char* sym = (const char*)memchr(cdn.start, '$', cdn.size());
        if (!sym) // No more special symbols - emit rest directly
        {
            write_string_utf8(cdn.start, cdn.size());
            break;
        }
        if (sym != cdn.start)
        {
            write_string_utf8(cdn.start, sym - cdn.start);
            cdn.start = sym;
        }
        if (cdn.size() < 2) break; // Symbol '$' can't be last - skip it
        if (cdn.start[1] == '$') // Just $
        {
            write_string_utf8("$", 1);
            cdn.start+=2;
            continue;
        }
        if (cdn.start[1] == '?' && (cdn.start == cdn_org || cdn.start[-2] == '\n')) // Conditional block
        {
            bool is_active = test_cond(cdn);
            if (!is_active)
            {
                const char* e = memstr(cdn.start, "\n$-", cdn.size());
                if (!e) break;
                cdn.start = e + 3;
                bump_to_eol(cdn);
                continue;
            }
        }
        if (cdn.start[1] == '-')  // Termination line of conditional block. We can went here only if condition was evaluated to true. Skip this line.
        {
            bump_to_eol(cdn); 
            continue;
        }
        if (cdn.start[1] != '[') {write_string_utf8(cdn.start, 2); cdn.start += 2; continue;}

        // So, we got variable substitution here
        cdn.start += 2;
        const char* e = (const char*)memchr(cdn.start, ']', cdn.size());
        if (!e) // No termination bracket - error
        {
            ESP_LOGE(TAG, "Var substitution lost closed bracket in %s", fname);
            break;
        }
        cpy2var(cdn.start, e-cdn.start);
        cdn.start = e+1;
        web_options.decode_inline(var_name, *this);
    }
}

void Ans::flush()
{
    if (!buf_filled) return;
    chunked_active = true;
    httpd_resp_send_chunk(req, buffer.get(), buf_filled);
    buf_filled = 0;
}

// Advance pos.start to the end after current line
void Ans::bump_to_eol(CDNDef& pos)
{
    char* e = (char*)memchr(pos.start, '\n', pos.size());
    pos.start = e ? e+1 : pos.end;
}

// Test conditional at 'pos', skip 'pos' to next line
// Return true if condition held
bool Ans::test_cond(CDNDef& pos)
{
    CDNDef ptr = pos;
    bump_to_eol(pos);
    ptr.start += 2; // Skip $? at begining
    ptr.end = pos.start; // Constrain to seek inside line (including '\n' at end)
    while(ptr.size() && isspace(*ptr.start)) ++ptr.start;
    const char* p = mempbrk(ptr.start, ", \n", ptr.size());
    if (!p) p = ptr.end;
    cpy2var(ptr.start, p-ptr.start);
    auto cond = web_options.get_condition(var_name, *this);
    if (*p == ',') cond &= strtol(p+1, NULL, 16);
    return (cond != 0);
}
