#pragma once

#include <memory>
#include <algorithm>
#include <optional>

#include <string.h>

#include <esp_http_server.h>

struct CDNDef {
    const char* start = nullptr;
    const char* end = nullptr;

    size_t size() const {return end-start;}
};

class Ans {
    static constexpr size_t BufSize = 1024;
    static constexpr size_t VarNameSize = 64;
    httpd_req_t *req;
    std::unique_ptr<char> buffer; // Chunk of data to send to httpd client. Allocated on heap, because it quite large to be allocated on stack
    int buf_filled = 0;
    bool chunked_active = false;
    char* var_name; // We reuse 'buffer' for temporary storage of VarName (in WEB substitution)

    void flush();

    // Test conditional at 'pos', skip 'pos' to next line
    // Return true if condition held
    bool test_cond(CDNDef& pos);

    // Advance pos.start to the end after current line
    static void bump_to_eol(CDNDef& pos);

    void cpy2var(const char* ptr, size_t length)
    {
        if (BufSize - buf_filled < VarNameSize) flush();
        var_name = buffer.get() + buf_filled;
        size_t len = std::min(VarNameSize-1, length);
        memcpy(var_name, ptr, len); var_name[len] = 0;
    }

public:
    Ans(httpd_req_t *);
    ~Ans();

    void write_cdn(const char* fname);

    void write_int(uint32_t);
    void write_string_dos(const char* ptr, int lentgth = -1);
    void write_string_utf8(const char* ptr, int lentgth = -1);

    esp_err_t set_ans_type(const char* file_name);
    void send_error(httpd_err_code_t, const char*);
    void set_hdr(const char* tag, const char* value);

// Members for AJAX handlers
    using ArgI = uint32_t;
    using ArgOI = std::optional<uint32_t>;
    using ArgSU = const char*;
    using ArgSD = const char*;
    using ArgOSU = const char*;
    using ArgOSD = const char*;
    using ArgTU = std::unique_ptr<char>;
    using ArgTD = std::unique_ptr<char>;
    using ArgU = uint32_t;
    using ArgOV = bool;
};

// GET request
class AnsGet : public Ans {
public:    
    AnsGet(httpd_req_t *req) : Ans(req) {}

    ArgI    decode_I(const char* tag);
    ArgOI   decode_OI(const char* tag);
    ArgSU   decode_SU(const char* tag);
    ArgSD   decode_SD(const char* tag);
    ArgOSU  decode_OSU(const char* tag);
    ArgOSU  decode_OSD(const char* tag);
    ArgTU   decode_TU(const char* tag);
    ArgTD   decode_TD(const char* tag);
    ArgU    decode_U(const char* tag);
    ArgOV   decode_OV(const char* tag);
};

class AnsPost : public Ans {
public:
    AnsPost(httpd_req_t *req) : Ans(req) {}

// Overrdie decoding methods for POST request
    ArgTU   decode_TU(const char* tag);
    ArgI    decode_I(const char* tag);
    ArgTD   decode_TD(const char* tag);
};
