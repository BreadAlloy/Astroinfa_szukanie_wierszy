#pragma once

#include <iostream>
#include <cassert>

constexpr size_t rozmiar_bloku_dysku = 4 * 1024; //4kb

namespace moje_io{

    static FILE* fopen(std::string lokalizacja_pliku, const char* Mode){
        FILE* fd = std::fopen(lokalizacja_pliku.c_str(), Mode);
        if(fd == nullptr){
            printf("%s\n", strerror(errno));
            assert(false);
        }
        return fd;
    }

}


