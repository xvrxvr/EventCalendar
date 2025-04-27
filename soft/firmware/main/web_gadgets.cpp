#include "common.h"
#include "protocol_examples_utils.h"

#include "web_gadgets.h"
#include "web_files_access.h"
#include "web_vars.h"

static const char* TAG = "web_gadgets";

Ans::Ans(httpd_req_t *r) :buffer(new char[BufSize]), req(r) {}

Ans::~Ans() 
{
    if (chunked_active) {flush(); httpd_resp_send_chunk(req, NULL, 0);} else
    if (buf_filled) httpd_resp_send(req, buffer.get(), buf_filled);
}

void Ans::write_int(uint32_t val) {char b[11]; sprintf(b, "%ld", val); write_string_utf8(b);}

void Ans::write_string_utf8(const char* ptr, int length)
{
    if (length == -1) length = strlen(ptr);
    if (buf_filled) // Try to append to buffer
    {
        int to_send = std::min<int>(length, BufSize - buf_filled);
        if (to_send)
        {
            memcpy(buffer.get() + buf_filled, ptr, to_send);
            length -= to_send;
            ptr += to_send;
            buf_filled += to_send;
        }
    }
    if (!length) return; // All fit in one buffer
    flush();
    // Do the rest fit in buffer ?
    if (length <= BufSize) // Yes - push to buffer
    {
        memcpy(buffer.get(), ptr, length);
        buf_filled += length;
    }
    else // Do not fit - send it directly as one chunk
    {
        chunked_active = true;
        httpd_resp_send_chunk(req, ptr, length);        
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
            write_string_utf8(dos_to_utf8(*ptr++).b);
        }
    }    
}

esp_err_t Ans::set_ans_type(const char* filename)
{
#define IS_FILE_EXT(filename, ext) (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)
#define FE(ext, tp) if (IS_FILE_EXT(filename, ext)) return httpd_resp_set_type(req, tp)
    FE(".html", "text/html");
    FE(".htm",  "text/html");
    FE(".jpeg", "image/jpeg");
    FE(".jpg",  "image/jpeg");
    FE(".ico",  "image/x-icon");
    FE(".png",  "image/png");
    FE(".css", 	"text/css");
    FE(".js",   "text/javascript");
    FE(".json",	"application/json");
    FE(".svg",  "image/svg+xml");
    FE(".xml",  "application/xml");
    FE(".tar",  "application/x-tar");
    FE(".elf",  "application/octet-stream");
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

void Ans::redirect(const char* path)
{
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", path);
    *this << UTF8 << "Redirected";
}


void Ans::write_cdn(const char* fname)
{
    CDNDef cdn = decode_web_files_access_function(fname);
    if (!cdn.start)
    {
        send_error(HTTPD_404_NOT_FOUND, "File not found");
        return;
    }
    set_ans_type(fname);
    if (cdn.start[0] != '$') // Send it directly
    {
        httpd_resp_send(req, cdn.start, cdn.end - cdn.start);
        return;
    }
    set_hdr("Cache-Control", "no-store, no-cache, max-age=0, must-revalidate, proxy-revalidate");
    ++cdn.start;
    auto cdn_org = cdn.start;
#define BOL (cdn.start == cdn_org || cdn.start[-1] == '\n')
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
        if (cdn.start[1] == '?' && BOL) // Conditional block
        {
            bool is_active = test_cond(cdn);
            if (!is_active)
            {
                int nest = 1;
                while(nest > 0)
                {
                    const char* e = memstr(cdn.start, "\n$", cdn.size());
                    assert(e);
                    if (e[2] == '-') --nest; else
                    if (e[2] == '?') ++nest;
                    cdn.start = e + 3;
                }
                bump_to_eol(cdn);
            }
            continue;
        }
        if (cdn.start[1] == '-' && BOL)  // Termination line of conditional block. We can went here only if condition was evaluated to true. Skip this line.
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
#undef BOL

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
//////////////////////////////////////////
void Ans::load_request_string()
{
    int buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len <= 1) return;
    request_string.reset(new char[buf_len]);
    ESP_ERROR_CHECK(httpd_req_get_url_query_str(req, request_string.get(), buf_len));
    ESP_LOGI(TAG, "Setup: URL query => %s", request_string.get());
}

Ans::ArgI    AnsGet::decode_I(const char* tag)
{
    char b[32];
    if (!request_string || httpd_query_key_value(request_string.get(), tag, b, sizeof(b)) != ESP_OK)
    {
        ESP_LOGE(TAG, "Expected tag '%s'", tag);
        return 0;
    }
    return strtoul(b, NULL, 10);
}

Ans::ArgOI AnsGet::decode_OI(const char* tag)
{
    char b[32];
    if (!request_string || httpd_query_key_value(request_string.get(), tag, b, sizeof(b)) != ESP_OK) return {};
    return strtoul(b, NULL, 10);
}

Ans::ArgSU AnsGet::decode_SU(const char* tag)
{
    if (!request_string || httpd_query_key_value(request_string.get(), tag, alloc_buf(), available_buf()) != ESP_OK)
    {
        ESP_LOGE(TAG, "Expected tag '%s'", tag);
        return "";
    }
    return adjust_buf();
}

Ans::ArgSD AnsGet::decode_SD(const char* tag)
{
    char* dst = alloc_buf();
    if (!request_string || httpd_query_key_value(request_string.get(), tag, dst, available_buf()) != ESP_OK)
    {
        ESP_LOGE(TAG, "Expected tag '%s'", tag);
        return "";
    }
    example_uri_decode(dst, dst, available_buf());
    utf8_to_dos(dst);
    return adjust_buf();
}

Ans::ArgOSU AnsGet::decode_OSU(const char* tag)
{
    char* dst = alloc_buf();
    if (!request_string || httpd_query_key_value(request_string.get(), tag, dst, available_buf()) != ESP_OK) return NULL;
    example_uri_decode(dst, dst, available_buf());
    return adjust_buf();

}

Ans::ArgOSU AnsGet::decode_OSD(const char* tag)
{
    char* dst = alloc_buf();
    if (!request_string || httpd_query_key_value(request_string.get(), tag, dst, available_buf()) != ESP_OK) return NULL;
    example_uri_decode(dst, dst, available_buf());
    utf8_to_dos(dst);
    return adjust_buf();
}

Ans::ArgU AnsGet::decode_U(const char*)
{
    char* dst = alloc_buf();
    if (!request_string) return 0;
    uint32_t result = 0;
    for(int i=0; i<32; ++i)
    {
        char tag[4];
        sprintf(tag, "u%d", i);
        if (httpd_query_key_value(request_string.get(), tag, dst, available_buf()) == ESP_OK) result |= 1 << i;
    }
    return result;
}

Ans::ArgOV AnsGet::decode_OV(const char* tag)
{
    if (!request_string || httpd_query_key_value(request_string.get(), tag, alloc_buf(), available_buf()) != ESP_OK) return false;
    return true;
}
/////////
void AnsPost::read_body()
{
    int remaining = req->content_len;
    assert(remaining >= 0);
    if (!remaining) return;
    char* buf = ensure_size(remaining+1);
    int total;
    do {total = httpd_req_recv(req, buf, remaining);} while (total ==  HTTPD_SOCK_ERR_TIMEOUT);
    assert(total >= 0 && total <= remaining);
    buf[total] = 0;
    decode_body();
}

void AnsPost::decode_body()
{
    char* ptr = alloc_buf();

    auto nxt_line = [&ptr]() -> char* {
        char* ret = ptr;
        ptr = strchr(ptr, '\n');
        if (!ptr) return NULL;
        *ptr++ = 0;
        return ret;
    };

    char* dlm = nxt_line();
    size_t dlm_len = strlen(dlm);

    assert(dlm);

    while(ptr && *ptr)
    {
        assert(total_fields<MAX_FLDS);
        FldDef& fld = flds[total_fields++];
        fld.name = NULL;
        fld.body = NULL;
        fld.body_size = 0;
        while(char* hdr = nxt_line())
        {
            if (!*hdr) break; // Start of body
#define CDISP "Content-Disposition:"
#define NAME "name=\""
            if (memcmp(hdr, CDISP, sizeof(CDISP)-1)==0)
            {
                char* fname = strstr(hdr, NAME);
                assert(fname);
                fname += sizeof(NAME) - 1;
                fld.name = fname;
                strchr(fname, '"')[0] = 0;
            }
        }
#undef NAME
#undef CDISP        
        fld.body = ptr;
        while(char* nxt = strchr(ptr, '\n'))
        {
            if (memcmp(nxt+1, dlm, dlm_len) == 0)
            {
                *nxt = 0;
                fld.body_size = nxt - fld.body;
                ptr = nxt + 1;
                break;
            }
            ptr = nxt + 1;
        }
    }
}


Ans::ArgI AnsPost::decode_I(const char* tag)
{
    char* f = find_field(tag);
    assert(f);
    return strtoul(f, NULL, 10);
}

Ans::ArgTU AnsPost::decode_TU(const char* tag)
{
    char* f = find_field(tag);
    assert(f);
    return f;
}

Ans::ArgTD AnsPost::decode_TD(const char* tag)
{
    char* f = find_field(tag);
    assert(f);
    utf8_to_dos(f);
    return f;
}

size_t AnsStream::read(size_t shift, size_t rest)
{
    int total, total_read = 0;
    do {
        if (rest > available_buf() - shift) rest = available_buf() - shift;
        do {total = httpd_req_recv(req, buf_start() + shift, rest);} while (total ==  HTTPD_SOCK_ERR_TIMEOUT);
        assert(total >= 0 && total <= rest);

        shift += total;
        rest -= total;
        total_read += total;
    } while(rest && shift < available_buf());
    return total_read;
}

// Try to read pack of data. retrun true if something was read (updates 'end' and 'remaining')
bool AnsStream::read_pack(size_t shift)
{
    if (!remaining) return false;
    size_t total = read(shift, remaining);
    end = buf_start() + shift + total;
    remaining -= total;
    return total != 0;
}

// Discard processed part (by 'ptr') from begining of buffer and read next portion of buffer
// Updates 'ptr', 'end' and 'remaining'
// Returns true if somethig was read
bool AnsStream::chop(int keep)
{
    if (keep > processed()) keep = processed();
    ptr -= keep;
    memmove(buf_start(), ptr, size());
    end -= ptr - buf_start();
    ptr = buf_start() + keep;
    return read_pack(size());
}

void AnsStream::skip_after_headers()
{
    for(;;)
    {
        char* e = (char*)memchr(ptr, '\n', size());
        if (!e)
        {
            bool some = chop();
            assert(some);
            continue;
        }
        ptr = e+1; // Skip header line
        if (ptr == end) // We need to load more
        {
            bool some = chop(1);
            assert(some);
        }
        if (ptr[1] == '\n') {++ptr; return;} // Empty line - end of headers
    }
}

void AnsStream::run()
{
    remaining = req->content_len; // Total number of unread yet data
    assert(remaining >= 0);
    if (!remaining) return;
    
    ptr = buf_start(); // Current position in buffer
    read_pack(0); // Read first buffer

/*    No header found ?
    skip_after_headers(); // Skip all headrs

    // Now file body starts
    chop(); // Discard header from buffer and load file contents (if any)
*/

    consume_stream(NULL, 0, false); // Start stream

    do {
        bool eof = !remaining;
        size_t buf_size = size();
        assert(buf_size || eof);
        if (buf_size)
        {
            size_t processed = consume_stream((uint8_t*)ptr, buf_size, eof);
            ptr += processed;
            chop();
            assert(processed < buf_size ? !eof : true);
        }
    } while(remaining || size());
    
    consume_stream(NULL, 0, true); // End stream
}

// Send command through WebSocket
// Replace all "'" inside resultring page to '"" (and vise versa)
void web_send_cmd(const char* json, ...)
{
    Prn buf;

    va_list l;
    va_start(l, json);
    buf.vprintf(json, l);
    va_end(l);

    char* p = buf.c_str();
    while( (p = strpbrk(p, "'\"")) != NULL) *p++ ^= '"' ^ '\'';
    websock_send(buf.c_str());
}
