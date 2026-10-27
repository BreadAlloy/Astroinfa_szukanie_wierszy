#pragma once

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <inttypes.h>

#include "io_ops.h"

#include <vector>

struct czytacz_pliku_t{
    static constexpr size_t rozmiar_czytania = rozmiar_bloku_dysku;
    static constexpr size_t rozmiar_bufora = rozmiar_czytania * 3;
    static constexpr size_t ile_powinno_byc = 64;

    std::vector<char> buffer;
    
    uint32_t gdzie_czytane = 0;
    uint32_t dokad_wczytane = 0;

    FILE* fd = nullptr;
    bool koniec_pliku = false;

    czytacz_pliku_t(): buffer(rozmiar_bufora){}

    czytacz_pliku_t(std::string lokalizacja_pliku)
        : buffer(rozmiar_bufora){
        otworz(lokalizacja_pliku);
    }

    void otworz(std::string lokalizacja_pliku){
        assert(fd == nullptr);
        fd = moje_io::fopen(lokalizacja_pliku, "rb");
    }

    char* tu_wczytywac() {
        return &(buffer[dokad_wczytane]);
    }

    char* tu_czytac() {
        return &(buffer[gdzie_czytane]);
    }

    void przeczytano(uint32_t ile_przeczytano) {
        gdzie_czytane += ile_przeczytano;
        assert(gdzie_czytane < rozmiar_bufora);

        if((dokad_wczytane - gdzie_czytane) < ile_powinno_byc){
            wczytaj_wiecej();
        }
    }

    size_t przygotuj_nastepny_text(){
        size_t rozmiar_textu = 0;
        char* ptr = tu_czytac();
        while((ptr[rozmiar_textu] != '\n') && (ptr[rozmiar_textu] != '\t')){
            rozmiar_textu++;
        }
        ptr[rozmiar_textu] = '\0';
        return rozmiar_textu+1;
    }

    void wczytaj_wiecej(){
        if((rozmiar_czytania + dokad_wczytane) > rozmiar_bufora){
            memcpy(buffer.data(), tu_czytac(), dokad_wczytane - gdzie_czytane);
            dokad_wczytane = dokad_wczytane - gdzie_czytane;
            gdzie_czytane = 0;
        }
        size_t ret = fread(tu_wczytywac(), 1, rozmiar_czytania, fd);
        dokad_wczytane += ret;
        if(ret == 0) koniec_pliku = true;
        // assert(ret == rozmiar_czytania);
    }

    ~czytacz_pliku_t(){
        fclose(fd);
        fd = nullptr;
    }
};

static size_t przeczytaj_blok(char* dane, FILE* fd, size_t idx_bloku){
    assert(0 == fseek(fd, idx_bloku * rozmiar_bloku_dysku, SEEK_SET));
    return fread(dane, 1, rozmiar_bloku_dysku, fd);
}
