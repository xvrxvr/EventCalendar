#pragma once

#include "zip_streamer.priv.h"

namespace ZipStreamer {

class ZipError: public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Read from external stream ZIP archive and unpack it to files
class Reader {
    ZipInternals::Buffer user_buffer, fname, uncompress_buffer;
    ZipInternals::BufferWithPtr rcv_buffer;

    enum Where {
        FHeader,
        Body,
        CDIR
    } where = FHeader;

    size_t packeted_size;
    size_t unpacketed_size;
    bool do_deflate;

public:
    virtual ~Reader() {}

    // Get buffer to recieve ZIP stream
    void* get_buffer(size_t size) {return user_buffer.get_buffer(size);}

    // Put chunk of input ZIP stream
    void push(const void*, size_t);

    ///// Callbacks to store files

    // Write output file.
    // 'name' is a file name (with path) in UTF8
    // 'buffer' is a file image
    // 'size' is a image size
    virtual void write_file(const char* name, const void* data, size_t size) = 0;
};


// Created ZIP file and emit it
class Writer {
    ZipInternals::Buffer user_buffer, pck_file_buffer;
    ZipInternals::BufferWithPtr cdir_buffer, file_buffer;
    uint32_t written = 0;
    uint16_t files_count = 0;
    size_t cdir_file_start;
    uint32_t crc;

public:
    virtual ~Writer() {}

    // Get buffer to store input data
    void* get_buffer(size_t size) {return user_buffer.get_buffer(size);}

    // Finalize ZIP archive
    void finish();

    // Start writing file to ZIP stream
    // 'name' is a file name in UTF8 (with path, delimitered by '/' symbol)
    void open(const char* name);

    // Write chunk of data
    void write(const void* data, size_t size);

    // Finalize file currently writen
    void close();

    ////// Callback to send ZIP data
    virtual void send_data(const void* data, size_t size) = 0;
    virtual void send_close() = 0;
};


}
