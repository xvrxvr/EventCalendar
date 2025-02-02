#include <string.h>
#include "tar_stream.h"

#define BLOCKSIZE 512

struct TarPosixHeader
{                              /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */   // Size in octal (with leading zeros) - 11 digits, padded by space (not zero).
  char mtime[12];               /* 136 */   // space terminated
  char chksum[8];               /* 148 */   // 6 octal digits, zero, space
  char typeflag;                /* 156 */   // '0'
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
                                /* 500 */
  char padding[12];
};
static_assert(sizeof(TarPosixHeader) == BLOCKSIZE);

#define TMAGIC   "ustar"        /* ustar and a null */

#define TAR_DEFAULT {.mode="00666 ", .uid="000000 ", .gid="000000 ", .mtime={'0','0','0','0','0','0','0','0','0','0','0',' '}, .typeflag='0', .magic=TMAGIC, .version={'0','0'}, .devmajor="000000 ", .devminor="000000 "}

/* Convert VALUE to an octal representation suitable for tar headers.
   Output to buffer WHERE with size SIZE.
   The result is undefined if SIZE is 0 or if VALUE is too large to fit.  */

static void to_octal(uint32_t value, char *where, size_t size)
{
  do {
    where[--size] = '0' + (value & 7);
    value >>= 3;
  } while (size);
}

static uint32_t chksum_(const void* data, int size)
{
    uint32_t acc=0;
    const uint8_t* d = (const uint8_t*)data;
    while(size--) acc += *d++;
    return acc;
}

static uint32_t checksumm(TarPosixHeader& header)
{
    return chksum_(&header, 500) - chksum_(header.chksum, 8) + chksum_("        ", 8);
}

static const TarPosixHeader init = TAR_DEFAULT;

// ??? Do we need to create PAX header for UTF8 file name ???
void TarWriter::open(const char* fname, uint32_t size)
{
    write_pax_hdr(fname);
    TarPosixHeader& H = *(TarPosixHeader*)buffer.get();
    H = init;
    strcpy(H.name, fname);
    to_octal(size, H.size, sizeof(H.size)-1);
    sprintf(H.chksum, "%06lo", checksumm(H));
    blocks_to_write = (size + BLOCKSIZE - 1) / BLOCKSIZE;
    send_buffer();
}

static bool is_utf8(const char* str)
{
    while(*str) if (*str++ & 0x80) return true;
    return false;
}

void TarWriter::write_pax_hdr(const char* fname)
{
    if (!is_utf8(fname)) return;
    TarPosixHeader& H = *(TarPosixHeader*)buffer.get();
    H = init;
    strcat(strcpy(H.name, "PaxHeader/"), fname);
    H.typeflag = 'x';
    // "%d path=%s\n"
    // 1(2)+1+5+strlen+1 = 8(9)+strlen
    int pax_size = 8+strlen(fname);
    if (pax_size > 9) ++pax_size; // Two digit %d
    to_octal(pax_size, H.size, sizeof(H.size) - 1);
    sprintf(H.chksum, "%06lo", checksumm(H));
    send_buffer();
    memset(buffer.get(), 0, 512);
    sprintf((char*)buffer.get(), "%d path=%s\n", pax_size, fname);
    send_buffer();
}

bool TarWriter::write_block(uint32_t size)
{
    if (size > BLOCKSIZE) size = BLOCKSIZE;
    if (!blocks_to_write) return false;
    --blocks_to_write;
    auto b = buffer.get();
    if (size < BLOCKSIZE) memset(b+size, 0, BLOCKSIZE - size);
    send_buffer();
    return blocks_to_write != 0 && size == BLOCKSIZE;
}

void TarWriter::close()
{
    if (!blocks_to_write) return;
    memset(buffer.get(), 0, BLOCKSIZE);
    while(blocks_to_write--) send_buffer();
}


void TarWriter::finish()
{
    blocks_to_write += 2;
    close();
    send_close();
}
////////////////////////////////////////////

void TarReader::push(const void* data, uint32_t data_size)
{
    const uint8_t* src = (const uint8_t*)data;
    auto b = buffer.get();
    while(data_size)
    {
        uint16_t delta = uint16_t(std::min<uint32_t>(BLOCKSIZE-filled, data_size));
        memcpy(b+filled, src, delta);
        data_size -= delta;
        src += delta;
        filled += delta;
        if (filled >= BLOCKSIZE)
        {
            filled = 0;
            if (skip_blocks) {--skip_blocks; continue;}
            if (data_to_read)
            {
                uint16_t sz = uint16_t(std::min<uint32_t>(BLOCKSIZE, data_to_read));
                write(b, sz, wr_opaque);
                data_to_read -= sz;
                if (!data_to_read) close(wr_opaque);
                continue;
            }
            TarPosixHeader& H = *(TarPosixHeader*)b;
            if (checksumm(H) != strtoul(H.chksum, NULL, 8)) continue; // Not a header
            data_to_read = strtoul(H.size, NULL, 8);
            if (H.typeflag != '0' && H.typeflag != 0) // Some extended header - skip it and its data part
            {
                skip_blocks = (data_to_read + BLOCKSIZE - 1) / BLOCKSIZE;
                data_to_read = 0;
            }
            else // Start file
            {
                wr_opaque = open(H.name, data_to_read);
            }
        }
    }
}
