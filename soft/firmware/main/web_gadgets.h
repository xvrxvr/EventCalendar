#pragma once

struct CDNDef {
    const char* start = nullptr;
    const char* end = nullptr;

    size_t size() const {return end-start;}
};

class AnsDOS;
class AnsUTF;

enum SelectorDOS {DOS};
enum SelectorUTF8 {UTF8}; 
class Ans {
    static constexpr size_t VarNameSize = 64;
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
protected:
    static constexpr size_t BufSize = 1024;
    httpd_req_t *req;
    std::unique_ptr<char> request_string;

    char* ensure_size(size_t size)
    {
        if (size > BufSize) {assert(buf_filled==0); buffer.reset(new char[size+1]);}
        return buffer.get();
    }

    char* alloc_buf(size_t size = 0)
    {
        assert(buf_filled + size <= BufSize);
        return buffer.get() + buf_filled;
    }

    size_t available_buf() {return BufSize - buf_filled;}

    char* adjust_buf()
    {
        char* result = buffer.get() + buf_filled;
        buf_filled += strlen(result)+1;
        return result;
    }

    void buf_reset() {buf_filled = 0;}

    void load_request_string();
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
    void redirect(const char* path);

// Stream output
    AnsDOS operator <<(SelectorDOS);
    AnsUTF operator <<(SelectorUTF8);
    Ans& operator <<(uint32_t val) { write_int(val); return *this;}

// Members for AJAX handlers
    using ArgI = uint32_t;
    using ArgOI = std::optional<uint32_t>;
    using ArgSU = const char*;
    using ArgSD = const char*;
    using ArgOSU = const char*;
    using ArgOSD = const char*;
    using ArgTU = const char*;
    using ArgTD = const char*;
    using ArgU = uint32_t;
    using ArgOV = bool;
};

class AnsDOS {
    Ans* owner;
public:
    AnsDOS(Ans* owner) : owner(owner) {}


    AnsDOS& operator <<(SelectorDOS) {return *this;}
    AnsUTF operator <<(SelectorUTF8);

    AnsDOS& operator <<(uint32_t val) { owner->write_int(val); return *this;}
    AnsDOS& operator <<(const char* str) {if (str) owner->write_string_dos(str); return *this;}
    AnsDOS& operator <<(uint8_t* str) {if (str && *str != 0xFF)  owner->write_string_dos((const char*)str); return *this;}
};

class AnsUTF {
    Ans* owner;
public:
    AnsUTF(Ans* owner) : owner(owner) {}

    AnsDOS operator <<(SelectorDOS) {return AnsDOS(owner);}
    AnsUTF& operator <<(SelectorUTF8)  {return *this;}
    AnsUTF& operator <<(uint32_t val) { owner->write_int(val); return *this;}
    AnsUTF& operator <<(const char* str) {if (str) owner->write_string_utf8(str); return *this;}
    AnsUTF& operator <<(uint8_t* str) {if (str) owner->write_string_utf8((const char*)str); return *this;}
};

inline AnsDOS Ans::operator <<(SelectorDOS) {return AnsDOS(this);}
inline AnsUTF Ans::operator <<(SelectorUTF8) {return AnsUTF(this);}
inline AnsUTF AnsDOS::operator <<(SelectorUTF8) {return AnsUTF(owner);}

// GET request
class AnsGet : public Ans {
public:    
    AnsGet(httpd_req_t *req) : Ans(req) {load_request_string();}

    ArgI    decode_I(const char* tag);
    ArgOI   decode_OI(const char* tag);
    ArgSU   decode_SU(const char* tag);
    ArgSD   decode_SD(const char* tag);
    ArgOSU  decode_OSU(const char* tag);
    ArgOSU  decode_OSD(const char* tag);
    ArgU    decode_U(const char* tag);
    ArgOV   decode_OV(const char* tag);
};

class AnsPost : public Ans {
    static constexpr int MAX_FLDS = 4;
    struct FldDef {
        const char* name;
        char* body;
        size_t body_size;
    } flds[MAX_FLDS];    
    size_t total_fields = 0;

    void read_body();
    void decode_body();

    char* find_field(const char* tag)
    {
        for(const auto& f: flds)
        {
            if (!f.name) return NULL;
            if (strcmp(f.name, tag)==0) return f.body;
        }
        return NULL;
    }

public:
    AnsPost(httpd_req_t *req) : Ans(req) {read_body();}

// Overrdie decoding methods for POST request
    ArgTU   decode_TU(const char* tag);
    ArgI    decode_I(const char* tag);
    ArgTD   decode_TD(const char* tag);
};

class AnsStream : public Ans {
    int remaining ; // Total number of unread yet data
    
    char* ptr; // Current position in buffer
    char* end; // End of filled part of buffer
    char* dlm; // Delimiter line will be here
    size_t dlm_size = 0; // Size of delimiter - initial part of buffer of this size will be preserved

    // Total number of bytes in unprocessed buffer (from 'ptr' to 'end')
    size_t size() {return end-ptr;}
    size_t processed() {return ptr-buf_start();}

    size_t read(size_t shift, size_t rest);

    char* buf_start() {return alloc_buf() + dlm_size;}

    // Try to read pack of data. retrun true if something was read (updates 'end' and 'remaining')
    bool read_pack(size_t shift);

    // Discard processed part (by 'ptr') from begining of buffer and read next portion of buffer
    // 'keep' defines how many bytes of processed space keep in buffer
    // Updates 'ptr', 'end' and 'remaining'
    // Returns true if somethig was read
    bool chop(int keep=0);

    void extract_dlm();

    void skip_after_headers();

    size_t find_eof(bool& eof);

public:
    AnsStream(httpd_req_t *req) : Ans(req) {}

    void run();

    virtual size_t consume_stream(uint8_t* buffer, size_t size, bool eof) = 0;
};

// Send command through WebSocket
// Replace all "'" inside resultring page to '"" (and vise versa)
void web_send_cmd(const char* json, ...); // Wrapper around 'websock_send'

//// Those functions are in 'web_server.cpp' file
void set_web_root(const char*); // Setup WEB root page
void websock_send(const char*); // Send JSON via WebSockets (all opened)
