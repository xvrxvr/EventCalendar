#pragma once

struct CDNDef {
    const unsigned char* start = nullptr;
    const unsigned char* end = nullptr;
};

CDNDef decode_web_files_access_function(const char* key);
