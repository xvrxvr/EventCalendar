#include <string>
#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <locale>
#include <codecvt>

#include "../tar_stream.h"

constexpr int BSIZE = 1000;

#define UTF8 std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>()

class MyReader : public TarReader {
    std::wstring dst_dir;

public:
    MyReader(const char* dst) : dst_dir(UTF8.from_bytes(dst)) {}

    void unpack(const char* fname)
    {
        char buf[BSIZE];
        FILE* f = fopen(fname, "rb");
        assert(f);
        while (auto sz = fread(buf, 1, BSIZE, f))
        {
            push(buf, sz);
            if (sz < BSIZE) break;
        }
        fclose(f);
    }

    virtual void* open(const char* name, uint32_t size) override
    {
        std::wstring dfile = dst_dir + L"/" + UTF8.from_bytes(name);
        return new std::ofstream(dfile, std::ios_base::out | std::ios_base::binary);
    }

    virtual void write(const void* data, uint32_t size, void* opaque) override
    {
        ((std::ofstream*)opaque)->write((char*)data, size);
    }

    virtual void close(void* opaque) override
    {
        delete ((std::ofstream*)opaque);
    }

};

class MyWriter : public TarWriter {
    FILE* dstf;
    std::wstring dname;
public:
    MyWriter(const char* dst_file, const char* dir_name) : dstf(fopen(dst_file, "wb")), 
        dname(UTF8.from_bytes(dir_name))
    {
        assert(dstf);
    }

    void save_file(const std::wstring& fname, uint32_t size)
    {
        open(UTF8.to_bytes(fname).c_str(), size);
        std::ifstream fstr(dname + L"/" + fname, std::ios_base::in | std::ios_base::binary);
        assert(fstr);
        auto buf = get_buffer();
        do {
            fstr.read(buf, BLOCKSIZE);
        } while(write_block(fstr.gcount()));
        close();
    }

    virtual void send_data(const void* data, size_t size) override
    {
        fwrite(data, 1, size, dstf);
    }
    virtual void send_close() override { fclose(dstf); }
};


void test_unpack(const char* dst_dir, const char* fname)
{
    MyReader(dst_dir).unpack(fname);
}

#include <filesystem>

void test_pack(const char* dir, const char* out_file)
{
    MyWriter wrt(out_file, dir);
    for (auto& dent : std::filesystem::directory_iterator(dir))
    {
        if (dent.is_regular_file()) 
        {
            wrt.save_file(dent.path().filename().wstring(), dent.file_size());
        }
    }
    wrt.finish();
}

int main()
{
    test_pack("pack_test_dir", "ptfile.tar");
    test_unpack("unpack_test_dir", "t.tar"); // "ptfile.tar");
    return 0;
}
