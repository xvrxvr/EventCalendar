#pragma once

#include <stdint.h>
#include <memory>

class TarWriter {
    std::unique_ptr<uint8_t> buffer;
    uint32_t blocks_to_write;

    void send_buffer() {send_data(buffer.get(), BLOCKSIZE);}

    void write_pax_hdr(const char* fname);
public:
    static constexpr int BLOCKSIZE = 512;

    TarWriter() : buffer(new uint8_t[BLOCKSIZE]) {}
    virtual ~TarWriter() {}

    char* get_buffer() {return (char*)buffer.get();}

    void open(const char* fname, uint32_t size);
    bool write_block(uint32_t); // retrun false if last block was written
    void close();

    void finish();

    virtual void send_data(const void* data, size_t size) = 0;
    virtual void send_close() = 0;
};

class TarReader {
    static constexpr int BLOCKSIZE = 512;
    std::unique_ptr<uint8_t> buffer;
    uint16_t filled = 0;
    uint16_t skip_blocks = 0;
    uint32_t data_to_read;
    void* wr_opaque;

public:
    TarReader() : buffer(new uint8_t[BLOCKSIZE]) {}
    virtual ~TarReader() {}
    
    void push(const void*, uint32_t);

    virtual void* open(const char* fname, uint32_t size) = 0;
    virtual void write(const void* data, uint32_t size, void* opaque) = 0;
    virtual void close(void* opaque) = 0;
};
