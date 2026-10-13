#pragma once

#include <vector>
#include "io_ops.h"

struct zapiszywacz_do_pliku_t{
    static constexpr size_t rozmiar_zapisu = 1024 * 1024; // 1MB
    static constexpr size_t rozmiar_bufora = rozmiar_zapisu + 1000;

    std::vector<char> bufor;
 
    uint32_t bufor_do = 0;

    FILE* fd = nullptr;

    zapiszywacz_do_pliku_t(std::string sciezka_pliku)
    : bufor(rozmiar_bufora){
        fd = moje_io::fopen(sciezka_pliku, "wb");
    }

    zapiszywacz_do_pliku_t(zapiszywacz_do_pliku_t&& old)
        : bufor(old.bufor){
        fd = old.fd;
        bufor_do = old.bufor_do;

        old.fd = nullptr;
        old.bufor_do = 0;
    }

    void dorzuc_dane(const char* dane, size_t ile_danych){
        memcpy(&(bufor[bufor_do]), dane, ile_danych);
        bufor_do += ile_danych;

        if(bufor_do >= rozmiar_zapisu) zapisz();
    }

    void zapisz(){
        size_t ret = fwrite(bufor.data(), 1, bufor_do, fd);
        assert(ret == bufor_do);
        bufor_do = 0;
    }

    ~zapiszywacz_do_pliku_t(){
        if (bufor_do > 0) {
            zapisz();
        }
        if(fd != nullptr) fclose(fd);
        fd = nullptr;
    }
};