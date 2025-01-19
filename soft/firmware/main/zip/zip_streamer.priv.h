#pragma once

#include <stdexcept>
#include <stdint.h>
#include <vector>

namespace ZipInternals {

enum class Tag : uint32_t {
    Local = 0x04034b50,
    File  = 0x02014b50,
    EOD   = 0x06054b50
};

#ifdef _WIN32
#pragma pack( push , 1 )
#define PACK
#else
#define PACK __attribute__ ((packed))
#endif

struct PACK LocalHdr {
    Tag       sign;             //0x04034b50  Tag::Local
    uint16_t  vers2extr;        //version needed to extract
    uint16_t  flags;            //general purpose bit flag
    uint16_t  comp_metod;       //compression method
    uint32_t  ftime_date;       //last mod file time/date
    uint32_t  crc32;
    uint32_t  comp_size;        //compressed size
    uint32_t  uncomp_size;      //uncompressed size
    uint16_t  fname_len;        //filename length
    uint16_t  extra_len;        //extra field length
//	filename (variable size)
//	extra field (variable size)
};
static_assert(sizeof(LocalHdr) == 30);

struct PACK FileHdr {
    Tag       sign;             //central file header signature (0x02014b50)  Tag::File
    uint16_t  ver_made_by;	    //version made by
    uint16_t  vers2extr;        //version needed to extract
    uint16_t  flags;            //general purpose bit flag
    uint16_t  comp_metod;       //compression method
    uint32_t  ftime_date;       //last mod file time/date
    uint32_t  crc32;
    uint32_t  comp_size;        //compressed size
    uint32_t  uncomp_size;      //uncompressed size
    uint16_t  fname_len;        //filename length
    uint16_t  extra_len;        //extra field length
    uint16_t  comm_len;         //file comment length
    uint16_t  dsk_num_strt;     //disk number start
    uint16_t  int_fattr;        //internal file attributes
    uint32_t  ext_fattr;        //external file attributes
    uint32_t  loc_off;          //relative offset of local header
//	filename (variable size)
//	extra field (variable size)
//	file comment (variable size)
};
static_assert(sizeof(FileHdr) == 46);

struct PACK EOD {
    Tag       sign;	            //end of central dir signature (0x06054b50) Tag::EOD
    uint16_t  dsk_num;          //number of this disk
    uint16_t  dir_dsk_num;      //number of the disk with the start of the central directory
    uint16_t  dir_entries;      //total number of entries in the central dir on this disk
    uint16_t  dir_total;        //total number of entries in the central dir
    uint32_t  dir_size;         //size of the central directory
    uint32_t  dir_off;          //offset of start of central directory with respect to the starting disk number
    uint16_t  comm_len;         //zipfile comment length
//	zipfile comment (variable size)
};
static_assert(sizeof(EOD) == 22);

#ifdef _WIN32
#pragma pack( pop )
#endif

constexpr uint16_t ZipVersion = 0x0020;

enum Flags : uint16_t {
    F_NotSet        = 0xF0FF,
    F_UTF8          = 1 << 11
};

enum CompressionMethod : uint16_t {
    CM_Store    = 0,
    CM_Deflate  = 8
};

enum InternalAttrs : uint16_t {
    IA_ASCII    = 0x0001,
    IA_VLCtrl   = 0x0002
};

class Buffer {
    std::vector<uint8_t> buffer;
public:
    Buffer() {}
    Buffer(size_t size) {get_buffer(size);}

    uint8_t* get_buffer(size_t size)
    {
        size = std::max<unsigned>(1024, size + (size>>2));
        if (size > buffer.size()) buffer.resize(size);
        return buffer.data();
    }

    uint8_t* get_buffer() {return buffer.data();}

    void put_string(const void* str, size_t size=0)
    {
        if (!size) size=strlen((const char*)str);
        auto buf = get_buffer(size+1);
        memcpy(buf, str, size);
        buf[size]=0;
    }
};

class BufferWithPtr : public Buffer {
    size_t ptr=0;
public:
    void reset() {ptr=0;}

    void put(const void* data, size_t size)
    {
        auto buf = get_buffer(size+ptr);
        memcpy(buf+ptr, data, size);
        ptr+=size;
    }

    template<class Item>
    void put(const Item& data) {put(&data, sizeof(Item));}

    uint8_t* get(size_t& size)
    {
        size = ptr;
        return get_buffer();
    }

    void remove(size_t size)
    {
        if (size >= ptr) {reset(); return;}
        auto buf  = get_buffer();
        ptr -= size;
        memmove(buf, buf+size, ptr);
    }

    bool empty() const {return ptr == 0;}

    size_t size() const {return ptr;}
};


}
