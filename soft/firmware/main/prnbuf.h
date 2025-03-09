#pragma once

class Prn {
    char* buffer  = NULL;   // Pointer to buffer in memory
    size_t size = 0;        // Filled size of buffer
    size_t allocated = 0;   // Allocated size of buffer (allocated >= size)
    bool spliced = false;   // Front of buffer was removed

    // Round up requested buffer size. Use to avoid too often buffer reallocation
    static size_t new_size(size_t size) {return std::max<size_t>(32, size*3/2 + 8);}

    // Resize buffer.
    //  sz - New requested size
    //  preserve - Set to true to preserve contents of buffer on resize
    // Field 'size' not changed
    void resize(size_t sz, bool preserve)
    {
        if (sz <= allocated) return;
        size_t new_allocated = new_size(sz);
        char* new_buf = new char[new_allocated];
        if (preserve && buffer) memcpy(new_buf, buffer, size+1); 
        delete[] buffer;
        buffer = new_buf;
        allocated = new_allocated;
    }

    // Implementation of formatted print.
    //  shift - Where to print in buffer. Used to implement 2 versions of printf - just simple print and print with concatenation
    void vprintf_imp(size_t shift, const char* fmt, va_list args)
    {
        va_list args2;
        va_copy(args2, args);
        size_t len = allocated - shift;
        resize(shift + 1024, shift != 0);
        size_t str_size = vsnprintf( buffer + shift, len, fmt, args);
        if (str_size >= len)
        {
            resize(shift + str_size + 1, shift != 0);
            vsnprintf( buffer + shift, str_size + 1, fmt, args2);
        }
        va_end(args2);
        size = str_size + shift;
    }

public:
    Prn() {}

    // Create buffer filled with symbol 'sym' of length 'len'
    Prn(char sym, size_t len)
    {
        fill(sym, len);
    }

    // Create buffer with string copy (use instead of strdup C function)
    Prn(const char* str)
    {
        strcpy(str);
    }

    Prn(const std::string_view& str)
    {
        if (str.size() && str.size() != -1) // Compare to -1 - just to passify compiler :(
        {
            fill(0, str.size());
            memcpy(buffer, str.data(), str.size());
        }
    }

    Prn(const Prn&) = delete;
//    operator =(const Prn&) = delete;

    ~Prn() {zap();}

    // Return raw buffer (as char*)
    char* c_str() {return buffer ? buffer : (char*)"";}

    char& operator[](int idx) {assert(idx < length()); return buffer[idx];}

    // Returns right part of string in buffer
    const char* right(size_t len) {return c_str() + std::max<std::ptrdiff_t>(0, size - len);}

    // Returns length of string
    int length() {return size;}

    // Clear buffer. Memory not returned to system!
    void clear() {size=0; spliced=false;}

    // Clear buffer and returns memory to system
    void zap() {delete[] buffer; size=allocated=0; spliced=false;}

    void utf8_to_dos(int shift=0)
    {
        assert(shift<length());
        size = shift + ::utf8_to_dos(buffer+shift) - 1;
    }

    // Fill buffer with symbol 'sym' by 'len' length. Previous contents of buffer discarded
    Prn& fill(char sym, size_t len)
    {
        resize(len+1, false);
        memset(buffer, sym, len);
        buffer[len] = 0;
        size = len;
        return *this;
    }

    // Append string of 'sym' in length 'len' to current contents of buffer
    Prn& cat_fill(char sym, size_t len)
    {
        resize(size+len+1, false);
        memset(buffer+size, sym, len);
        size += len;
        buffer[size] = 0;
        return *this;
    }

    // Pad buffer by symbol 'sym' to desired length 'len'
    Prn& pad(char sym, size_t len)
    {
        if (len > size) cat_fill(sym, len - size);
        return *this;
    }

    // strcpy analog
    Prn& strcpy(const char* str)
    {
        auto l = strlen(str);
        resize(l+1, false);
        ::strcpy(buffer, str);
        size = l;
        return *this;
    }

    // strcat analog
    Prn& strcat(const char* str)
    {
        size_t l = strlen(str);
        resize(l+size+1, true);
        ::strcpy(buffer + size, str);
        size += l;
        return *this;
    }

    // vsprintf analog
    Prn& vprintf(const char* fmt, va_list args)
    {
        vprintf_imp( 0, fmt, args);
        return *this;
    }

    // Concatenate result of vsprintf to the current content of the buffer
    Prn& cat_vprintf(const char* fmt, va_list args)
    {
        vprintf_imp( size, fmt, args);
        return *this;
    }

    // sprintf analog
    Prn& printf(const char* fmt, ...)
    {
        va_list l;
        va_start(l, fmt);
        vprintf(fmt, l);
        va_end(l);
        return *this;
    }

    // Concatenate result of sprintf to the current content of the buffer
    Prn& cat_printf(const char* fmt, ...)
    {
        va_list l;
        va_start(l, fmt);
        cat_vprintf(fmt, l);
        va_end(l);
        return *this;
    }

    // Remove data in front of buffer to fit rest in 'req_size' bytes
    void fit_in(size_t req_size)
    {
        if (req_size >= size) return;
        spliced = true;
        char* from = strchr(buffer + (size-req_size), '\n');
        if (!from) {clear(); return;}
        ++from;
        ::strcpy(buffer, from);
        size = strlen(buffer);
    }

    bool was_spliced() const {return spliced;}

    void swap(Prn& other)
    {
        std::swap(buffer, other.buffer);
        std::swap(size, other.size);
        std::swap(allocated, other.allocated);
        std::swap(spliced, other.spliced);
    }

};
