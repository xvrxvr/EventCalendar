#pragma once

#include <string.h>

inline const char* memstr(const char* where, const char* what, size_t size)
{
    const size_t len = strlen(what);
    const char* endp = where + size - len;
    while(where < endp)
    {
        where = (const char*)memchr(where, what[0], endp - where);
        if (!where) return NULL;
        if (memcmp(where, what, len) == 0) return where;
        ++where;
    }
    return NULL;
}

inline char* memstr(char* where, const char* what, size_t size) {return const_cast<char*>(memstr(const_cast<const char*>(where), what, size));}

inline const char* mempbrk(const char* where, const char* what, size_t size)
{
    const char* endp = where + size;
    while(where < endp)
    {
        if (strchr(what, *where)) return where;
        ++where;
    }
    return NULL;
}

inline char* mempbrk(char* where, const char* what, size_t size) {return const_cast<char*>(mempbrk(const_cast<const char*>(where), what, size));}
