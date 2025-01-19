#include <zlib.h>
#include <string.h>

#include "zip_streamer.h"

namespace ZipStreamer {

using namespace ZipInternals;

inline int err_end(int err)
{
    return err == Z_STREAM_END ? Z_OK :
           err == Z_OK ? Z_DATA_ERROR :
           err;
}

int uncompress3(Bytef* dest, size_t& destLen, const Bytef* source, uLong len) 
{
    z_stream stream{};

    int err = inflateInit2(&stream, -MAX_WBITS);
    if (err != Z_OK) return err;

    stream.next_in = (z_const Bytef*)source;
    stream.next_out = dest;
    stream.avail_out = destLen;
    stream.avail_in = len;

    err = inflate(&stream, Z_NO_FLUSH);

    destLen = stream.total_out;
    inflateEnd(&stream);
    return err_end(err);
}

int compress3(Bytef* dest, uLongf& destLen, const Bytef* source, uLong sourceLen, int level= Z_DEFAULT_COMPRESSION) 
{
    z_stream stream{};

    int err = deflateInit2(&stream, level, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
    if (err != Z_OK) return err;

    stream.next_out = dest;
    stream.avail_out = destLen;
    stream.next_in = (z_const Bytef*)source;
    stream.avail_in = sourceLen;

    err = deflate(&stream, Z_FINISH);

    destLen = stream.total_out;
    deflateEnd(&stream);
    return err_end(err);
}

///////////////////////////
// Put chunk of input ZIP stream
void Reader::push(const void* data, size_t size)
{
    if (where == CDIR) return;
    rcv_buffer.put(data, size);

    size_t buf_size;
    uint8_t* buf = rcv_buffer.get(buf_size);

    if (where == FHeader)
    {
        if (buf_size < sizeof(LocalHdr)) return;
        LocalHdr& L = *(LocalHdr*)buf;
        if (buf_size >= sizeof(Tag))
        {
            if (L.sign == Tag::File || L.sign == Tag::EOD)
            {
                where = CDIR;
                return;
            }
            if (L.sign != Tag::Local) throw ZipError("Unexpected record in ZIP file");
        }
        if (buf_size < sizeof(LocalHdr) + L.fname_len + L.extra_len) return;
        fname.put_string(1+(&L), L.fname_len);
        packeted_size = L.comp_size;
        unpacketed_size = L.uncomp_size;
        do_deflate = L.comp_metod == CM_Deflate;
        if (L.comp_metod != CM_Deflate && L.comp_metod != CM_Store) throw ZipError("Unsupported compression method");
        if (L.flags & F_NotSet) throw ZipError("Unsupported file Flags");
        if (packeted_size == 0xFFFF || unpacketed_size == 0xFFFF) throw ZipError("ZIP64 not supported");
        rcv_buffer.remove(sizeof(LocalHdr) + L.fname_len + L.extra_len);
        where = Body;
        buf = rcv_buffer.get(buf_size);
    }
    if (buf_size < packeted_size) return;
    if (!do_deflate)
    {
        write_file((const char*)fname.get_buffer(), buf, packeted_size);
    }
    else
    {
        auto dest = uncompress_buffer.get_buffer(unpacketed_size);
        int err = uncompress3(dest, unpacketed_size, buf, packeted_size);
        if (err != Z_OK) throw ZipError("zlib error on unpack");
        write_file((const char*)fname.get_buffer(), dest, unpacketed_size);
    }
    rcv_buffer.remove(packeted_size);
    where = FHeader;
}

///////////////////////////

// Finalize ZIP archive
void Writer::finish()
{
    if (cdir_buffer.empty()) return;

    EOD eod {
        .sign           = Tag::EOD,
        .dsk_num        = 0,
        .dir_dsk_num    = 0,
        .dir_entries    = files_count,
        .dir_total      = files_count,
        .dir_size       = uint32_t(cdir_buffer.size()),
        .dir_off        = written,
        .comm_len       = 0
    };

    cdir_buffer.put(eod);
    size_t size;
    auto buf = cdir_buffer.get(size);
    send_data(buf, size);
    send_close();
    cdir_buffer.reset();
}

// Start writing file to ZIP stream
// 'name' is a file name in UTF8 (with path, delimitered by '/' symbol)
void Writer::open(const char* name)
{
    cdir_file_start = cdir_buffer.size();

    FileHdr cdir_hdr {
        .sign           = Tag::File,
        .ver_made_by    = ZipVersion,
        .vers2extr      = ZipVersion,
        .flags          = F_UTF8,
        .ftime_date     = 0,
        .fname_len      = uint16_t(strlen(name)),
        .extra_len      = 0,
        .comm_len       = 0,
        .dsk_num_strt   = 0,
        .int_fattr      = 0,
        .ext_fattr      = 0,
        .loc_off        = written
    };
    cdir_buffer.put(cdir_hdr);
    cdir_buffer.put(name, strlen(name));
    crc = crc32_z(0, NULL, 0);
}

// Write chunk of data
void Writer::write(const void* data, size_t size)
{
    file_buffer.put(data, size);
    crc = crc32_z(crc, (Bytef*)data, size);
}

// Finalize file currently writen
void Writer::close()
{
    uint8_t* to_save;
    size_t size;
    auto buf = file_buffer.get(size);
    uLong pck_size = compressBound(size);
    auto pck_buf = pck_file_buffer.get_buffer(pck_size);

    FileHdr fhdr;
    uint8_t* cdir_fhdr_img = cdir_buffer.get_buffer() + cdir_file_start;
    memcpy(&fhdr, cdir_fhdr_img, sizeof(FileHdr));
    fhdr.crc32 = crc;
    fhdr.uncomp_size = size;

    int err = compress3(pck_buf, pck_size, buf, size);
    if (err !=  Z_OK) throw ZipError("zlib error on pack");
    if (pck_size >= size) // Do Store
    {
        fhdr.comp_size = size;
        fhdr.comp_metod = CM_Store;
        to_save = buf;
    }
    else // Deflate
    {
        fhdr.comp_size = pck_size;
        fhdr.comp_metod = CM_Deflate;
        to_save = pck_buf;
    }
    memcpy(cdir_fhdr_img, &fhdr, sizeof(FileHdr));

    LocalHdr lhdr {
        .sign           = Tag::Local,
        .vers2extr      = ZipVersion,
        .flags          = F_UTF8,
        .comp_metod     = fhdr.comp_metod,
        .ftime_date     = 0,
        .crc32          = fhdr.crc32,
        .comp_size      = fhdr.comp_size,
        .uncomp_size    = fhdr.uncomp_size,
        .fname_len      = fhdr.fname_len,
        .extra_len      = 0
    };
    send_data(&lhdr, sizeof(lhdr));
    send_data(cdir_fhdr_img + sizeof(FileHdr), lhdr.fname_len);
    send_data(to_save, lhdr.comp_size);

    written += sizeof(lhdr) + lhdr.fname_len + lhdr.comp_size;
    ++files_count;
    file_buffer.reset();
}

}
