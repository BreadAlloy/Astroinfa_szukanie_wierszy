#include "json/value.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <inttypes.h>
#include <limits>
#include <stdio.h>
#include <string>
#include <vector>

#include "json/json.h"
#include <iostream>

#undef NDEBUG 

#include "czytacz_pliku.h"
#include "io_ops.h"
#include "zapisywacz_do_pliku.h"

/*
1. Tylko typy 8 bytowe dozwolone
*/

typedef uint8_t typ_t;
constexpr uint8_t kolumna_typ_brak = 0;
constexpr uint8_t kolumna_typ_double = 1; // double
constexpr uint8_t kolumna_typ_long = 2; // int64_t

typedef int64_t pamiec_rekordu;
constexpr size_t rozmiar_rekordu = sizeof(pamiec_rekordu);

typedef pamiec_rekordu row_idx_t;

template<typename typ_wartosci>
struct rekord{
    row_idx_t idx;
    typ_wartosci wartosc;

    bool operator<(const rekord& prawy){
        static_assert(sizeof(rekord<typ_wartosci>) == 2*rozmiar_rekordu);
        return wartosc < prawy.wartosc;
    }
};



struct kolumna_t{
    std::string nazwa;
    std::string opis;
    typ_t typ = kolumna_typ_brak;

    Json::Value do_json() const {
        Json::Value json;

        json["name"] = nazwa;
        json["description"] = opis;

        switch (typ) {
            case(kolumna_typ_double):{
                json["type"] = "DOUBLE";
                break;
            }
            case(kolumna_typ_long):{
                json["type"] = "LONG";
                break;
            }
        }

        return json;
    }

    void printf_rekord(pamiec_rekordu dane) const {
        switch (typ) {
            case(kolumna_typ_double):{
                printf("%lf", *(reinterpret_cast<double*>(&dane)));
                return;
            }
            case(kolumna_typ_long):{
                printf("%lld", (*(int64_t*)&dane));
                return;
            }
        }
        assert(false);
    }

    template<typename typ_cos>
    void znajdz_poczatek(czytacz_pliku_t& czytacz_pliku, const std::string folder, typ_cos wartosc_od) const {
        switch (typ) {
            case(kolumna_typ_double):{
                _znajdz_poczatek<double>(czytacz_pliku, folder, (double)wartosc_od);
                return;
            }
            case(kolumna_typ_long):{
                _znajdz_poczatek<int64_t>(czytacz_pliku, folder, (int64_t)wartosc_od);
                return;
            }
        }
        assert(false);
    }

    template<typename typ_kolumny>
    void _znajdz_poczatek(czytacz_pliku_t& czytacz_pliku, std::string folder, typ_kolumny wartosc_od) const {

        std::vector<FILE*> indeksy_fd;
        std::string prefix = folder + nazwa + "_indeks_";
        for(uint8_t i = 0; true; i++){
            FILE* fd = fopen((prefix + std::to_string(i)).c_str(), "rb");
            if(fd == nullptr){
                assert(i != 0);
                break;
            }

            indeksy_fd.push_back(fd);
        }

        constexpr size_t ilosc_indeksow_na_blok = (rozmiar_bloku_dysku / sizeof(typ_kolumny));

        std::vector<typ_kolumny> dane(rozmiar_bloku_dysku / sizeof(typ_kolumny));
        size_t blok_trafiany = 0;
        for(uint8_t i = indeksy_fd.size()-1; i < indeksy_fd.size(); i--){
            size_t ile_przeczytano = przeczytaj_blok(
                (char*)dane.data(), indeksy_fd[i], blok_trafiany);

            assert((ile_przeczytano % sizeof(typ_kolumny)) == 0);
            ile_przeczytano /= sizeof(typ_kolumny);

            assert(ile_przeczytano > 1);

            uint32_t j = 0;
            for(; j < ile_przeczytano; j++){
                const typ_kolumny& wartosc = dane[j];

                if((wartosc_od <= wartosc)){
                    break;
                }
            }

            blok_trafiany = blok_trafiany * ilosc_indeksow_na_blok + j;

            // - 1 bo w indeksie pisze czym blok się zaczyna czyli szukane jest w poprzednim bloku. Chyba, że jest początek.
            if(j != 0) blok_trafiany--;

            fclose(indeksy_fd[i]);
        }

        czytacz_pliku.otworz(folder+nazwa);
        fseek(czytacz_pliku.fd, blok_trafiany * rozmiar_bloku_dysku, SEEK_SET);
        czytacz_pliku.wczytaj_wiecej();

        // czytacz_pliku.przeczytano(sizeof(rekord<typ_kolumny>));

        while(true){
            const rekord<typ_kolumny>& wartosc = *(rekord<typ_kolumny>*)czytacz_pliku.tu_czytac();
            if((wartosc_od <= wartosc.wartosc)){
                break;
            }


            assert(!czytacz_pliku.koniec_pliku);
            czytacz_pliku.przeczytano(sizeof(rekord<typ_kolumny>));
        }
    }

    template<typename typ_kolumny>
    void stworz_indeks(std::string kolumna_posortowana) const {
        //stworz indeks zabiera idx kolumny z danych

        FILE* fd = moje_io::fopen(kolumna_posortowana, "rb");

        std::vector<zapiszywacz_do_pliku_t> zapisywacze_indeksow;

        size_t indeks_w_indeksie = 0;

        {
            int ret = fseek(fd, 0, SEEK_END);
            assert(ret == 0);
        }

        size_t rozmiar_pliku = ftell(fd);

        {
            int ret = fseek(fd, 0, SEEK_SET);
            assert(ret == 0);
        }

        uint8_t ilosc_indeksow = 0;

        while(rozmiar_pliku * sizeof(typ_kolumny) > rozmiar_bloku_dysku){
            rozmiar_pliku /= rozmiar_bloku_dysku;
            ilosc_indeksow++;
        }
        
        constexpr size_t ilosc_indeksow_na_blok = (rozmiar_bloku_dysku / sizeof(typ_kolumny));

        while(true){
            rekord<typ_kolumny> rekord_na_poczatku_bloku;

            size_t ile_przeczytano = fread(
                &rekord_na_poczatku_bloku, 1, sizeof(rekord<typ_kolumny>), fd);

            if(ile_przeczytano == 0) break;

            assert(ile_przeczytano == sizeof(rekord<typ_kolumny>));

            {
            int ret = fseek(fd, rozmiar_bloku_dysku - ile_przeczytano, SEEK_CUR);
            assert(ret == 0);
            }

            {
                size_t indeks_w_indeksie_kopia = indeks_w_indeksie;
                uint8_t i = 0;
                while(i < ilosc_indeksow){

                    if(i >= zapisywacze_indeksow.size()){
                        zapisywacze_indeksow.emplace_back(
                            kolumna_posortowana + "_indeks_" + std::to_string(i));
                    }
                    
                    zapisywacze_indeksow[i].dorzuc_dane((char*)&rekord_na_poczatku_bloku.wartosc, sizeof(typ_kolumny));

                    i++;
                    if((indeks_w_indeksie_kopia % ilosc_indeksow_na_blok) != 0) break;
                    indeks_w_indeksie_kopia /= ilosc_indeksow_na_blok;
                }
            }



            indeks_w_indeksie++;
        }
    }

    void stworz_indeks(std::string folder) const {
        std::string lokalizacja_kolumny = folder + nazwa;
        switch (typ) {
            case(kolumna_typ_double):{
                stworz_indeks<double>(lokalizacja_kolumny);
                break;
            }
            case(kolumna_typ_long):{
                stworz_indeks<int64_t>(lokalizacja_kolumny);
                break;
            }
        }
    }

    template<typename typ_kolumny>
    void posortuj_kolumne_w_ramie(std::string folder) const {
        std::vector<rekord<typ_kolumny>> dane;

        {
            FILE* fd = moje_io::fopen(folder + "unsorted_" + nazwa, "rb");

            fseek(fd, 0, SEEK_END);
            size_t rozmiar_pliku = ftell(fd);
            fseek(fd, 0, SEEK_SET);

            dane.resize(rozmiar_pliku/sizeof(rekord<typ_kolumny>));

            size_t ile_przeczytane = fread(dane.data(), 1, rozmiar_pliku, fd);
            assert(ile_przeczytane == rozmiar_pliku);

            fclose(fd);
        }

        std::sort(dane.begin(), dane.end());

        {
            FILE* fd = moje_io::fopen(folder + "unsorted_" + nazwa, "wb");
            if(fd == nullptr){
                printf("%s\n", strerror(errno));
                assert(false);
            }

            size_t ile_zapisac = dane.size() * sizeof(rekord<typ_kolumny>);

            size_t ile_zapisane = fwrite(dane.data(), 1, ile_zapisac, fd);
            assert(ile_zapisac == ile_zapisane);

            fclose(fd);
        }

        remove((folder + nazwa).c_str());

        rename((folder + "unsorted_" + nazwa).c_str()
              ,(folder + nazwa).c_str());
    }

    void posortuj_kolumne_w_ramie(std::string folder) const {
        switch (typ) {
            case(kolumna_typ_double):{
                posortuj_kolumne_w_ramie<double>(folder);
                break;
            }
            case(kolumna_typ_long):{
                posortuj_kolumne_w_ramie<int64_t>(folder);
                break;
            }
        }
    }


};

struct kolumna_long_t : kolumna_t {
    void wczytaj(pamiec_rekordu& pamiec, const char* text){
        int64_t dane = std::stoll(text);
        pamiec = *((pamiec_rekordu*)&dane);
    }
};


struct kolumna_double_t : kolumna_t {
    void wczytaj(pamiec_rekordu& pamiec, const char* text){
        double dane = std::stod(text);
        pamiec = *((pamiec_rekordu*)&dane);
    }
};


struct wiersz_t {
    std::vector<pamiec_rekordu> dane;

    wiersz_t(uint32_t liczba_kolumn = 0)
    : dane(liczba_kolumn){}

    size_t byte_size() const {
        return dane.size() * sizeof(pamiec_rekordu);
    }
};

static wiersz_t przeczytaj_wiersz(FILE* fd, uint32_t liczba_kolumn, row_idx_t idx_wiersza){
    wiersz_t wiersz(liczba_kolumn);
    fseek(fd, idx_wiersza * wiersz.byte_size(), SEEK_SET);
    size_t przeczytano = fread(wiersz.dane.data(), 1, wiersz.byte_size(), fd);
    if(przeczytano == 0){
        wiersz.dane.resize(0);
    } else {
        assert(przeczytano == wiersz.byte_size());
    }
    return wiersz;
}

    struct naglowek_t{
        std::vector<kolumna_t> kolumny;
        bool dodano_id = false;

        naglowek_t(std::string folder){ // mój format
            std::vector<char> naglowek;

            FILE* fd = moje_io::fopen(folder + "naglowek.json", "rb");

            fseek(fd, 0, SEEK_END);
            size_t rozmiar_pliku = ftell(fd);
            fseek(fd, 0, SEEK_SET);

            naglowek.resize(rozmiar_pliku);

            size_t ile_przeczytane = fread(naglowek.data(), 1, rozmiar_pliku, fd);
            assert(ile_przeczytane == rozmiar_pliku);

            fclose(fd);

            from_json_string(naglowek);
        }

        naglowek_t(FILE* fd){ // inny format

            std::vector<char> naglowek;

            bool znaleziona_nowa_linia = true;
            bool wczytany_naglowek = false;

            while (!wczytany_naglowek) {
                size_t wczytane_od = naglowek.size();
                naglowek.resize(czytacz_pliku_t::rozmiar_czytania + wczytane_od);
                char* tu_wczytac = &(naglowek[naglowek.size()-czytacz_pliku_t::rozmiar_czytania]);

                size_t ret = fread(tu_wczytac, 1, czytacz_pliku_t::rozmiar_czytania, fd);
                if(czytacz_pliku_t::rozmiar_czytania != ret){
                    assert(false);
                }

                for(size_t i = wczytane_od; (i < naglowek.size()); i++){
                    if(znaleziona_nowa_linia == true){
                        if(naglowek[i] == '#'){
                            naglowek[i] = ' ';
                            znaleziona_nowa_linia = false;
                        } else {
                            // Po nowej lini nie ma kolejnego #, jest cały naglowek
                            if(znaleziona_nowa_linia == true){
                                wczytany_naglowek = true;
                                naglowek.resize(i);
                                fseek(fd, naglowek.size(), SEEK_SET);
                                break;
                            }
                        }
                    } else {
                        if(naglowek[i] == '\n'){
                            znaleziona_nowa_linia = true;
                        }
                    }
                }
            }

            from_json_string(naglowek);
        }

        Json::Value do_json() const {
            Json::Value json;
            for(const auto& kolumna : kolumny){
                json.append(kolumna.do_json());
            }          

            return json;
        }

        void from_json_string(const std::vector<char>& naglowek){
            Json::Value json;

            {
                JSONCPP_STRING err;
                Json::CharReaderBuilder builder;
                const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                if (!reader->parse(naglowek.data(), &naglowek.back() + 1, &json,
                                &err)) {
                    std::cout << "error: " << err << std::endl;
                }
            }

            kolumny.resize(json.size());

            uint32_t offset = 0;
            for(uint32_t i = 0; i < kolumny.size(); i++){
                kolumna_t& kolumna = kolumny[i];
                Json::Value& kolumna_json = json[i-offset];

                if(i == 0){
                    if(kolumna_json["name"].asString() != "id"){
                        kolumny.push_back(kolumna_t());
                        kolumny[0].nazwa = "id";
                        kolumny[0].opis = "id aby sortowac";
                        kolumny[0].typ = kolumna_typ_long;
                        dodano_id = true;
                        offset = 1;
                        continue;
                    }
                }

                kolumna.nazwa = kolumna_json["name"].asString();
                kolumna.opis = kolumna_json["description"].asString();

                std::string typ_str = kolumna_json["type"].asString();

                if(typ_str == "DOUBLE"){ kolumna.typ = kolumna_typ_double; }
                if(typ_str == "LONG"){ kolumna.typ = kolumna_typ_long; }
                
                assert(kolumna.typ != kolumna_typ_brak);
            }
        }

        wiersz_t pusty_wiersz(){
            return wiersz_t(liczba_kolumn());
        }

        uint32_t liczba_kolumn() const {
            return (uint32_t)kolumny.size();
        }
    };

struct czytacz_tekstowego_t{
    czytacz_pliku_t czytacz_pliku;
    naglowek_t naglowek;
    int64_t indeks_wiersza = 0;

    czytacz_tekstowego_t(std::string lokalizacja_pliku)
    : czytacz_pliku(lokalizacja_pliku)
    , naglowek(czytacz_pliku.fd)
    {
        czytacz_pliku.wczytaj_wiecej();
    }

    wiersz_t daj_wiersz(){
        wiersz_t wiersz = naglowek.pusty_wiersz();
        for(uint32_t i = 0; i < naglowek.kolumny.size(); i++){
            if((i == 0) && (naglowek.dodano_id)){
                wiersz.dane[0] = *((pamiec_rekordu*)&indeks_wiersza);
                continue;
            }

            const kolumna_t& kolumna = naglowek.kolumny[i];

            size_t rozmiar_textu = czytacz_pliku.przygotuj_nastepny_text();
            char* text = czytacz_pliku.tu_czytac();

            switch(kolumna.typ){

            case kolumna_typ_double: {
                kolumna_double_t& kol_z_typem = *((kolumna_double_t*)(&kolumna));

                kol_z_typem.wczytaj(wiersz.dane[i], text);
                break;
            }

            case kolumna_typ_long: {
                kolumna_long_t& kol_z_typem = *((kolumna_long_t*)(&kolumna));

                kol_z_typem.wczytaj(wiersz.dane[i], text);
                break;
            }
            }   
            czytacz_pliku.przeczytano(rozmiar_textu);
        }
        indeks_wiersza++;
        return wiersz;
    }
};

void do_formatu_mojego(std::string lokalizacja_zrodla, std::string folder_rezultat){
    czytacz_tekstowego_t czytacz_tekstowego(lokalizacja_zrodla);

    // zapisz nagłówek
    {
        Json::StreamWriterBuilder builder;
        const std::string json_string = Json::writeString(builder, czytacz_tekstowego.naglowek.do_json());

        FILE* fd = moje_io::fopen(folder_rezultat + "naglowek.json", "wb");

        fwrite(json_string.data(), 1, json_string.size(), fd);

        fclose(fd);
    }

    // zapisz zawartość
    if(false) {
        zapiszywacz_do_pliku_t zapisywacz_calego(folder_rezultat + "cale.bin");

        std::vector<zapiszywacz_do_pliku_t> zapisywacze_kolumn;
        zapisywacze_kolumn.reserve(czytacz_tekstowego.naglowek.liczba_kolumn());

        for(uint32_t i = 1; i < czytacz_tekstowego.naglowek.liczba_kolumn(); i++){
            zapisywacze_kolumn.emplace_back(
                folder_rezultat + "unsorted_" + czytacz_tekstowego.naglowek.kolumny[i].nazwa);
        }

        while (!czytacz_tekstowego.czytacz_pliku.koniec_pliku) {
            wiersz_t wiersz = czytacz_tekstowego.daj_wiersz();

            zapisywacz_calego.dorzuc_dane((char*)wiersz.dane.data(), wiersz.byte_size());

            for(uint32_t i = 1; i < czytacz_tekstowego.naglowek.liczba_kolumn(); i++){
                zapiszywacz_do_pliku_t& zapisywacz_kolumny = zapisywacze_kolumn[i - 1];

                zapisywacz_kolumny.dorzuc_dane((char*)&(wiersz.dane[0]), rozmiar_rekordu);
                zapisywacz_kolumny.dorzuc_dane((char*)&(wiersz.dane[i]), rozmiar_rekordu);
            }
        }
    }

    // posortować kolumny    
    for(uint32_t i = 1; i < czytacz_tekstowego.naglowek.liczba_kolumn(); i++){
        const kolumna_t& kolumna = czytacz_tekstowego.naglowek.kolumny[i];
        //kolumna.posortuj_kolumne_w_ramie(folder_rezultat);

        kolumna.stworz_indeks(folder_rezultat);
    }
}

struct czytacz_mojego_t{
    std::string folder;
    naglowek_t naglowek;

    czytacz_mojego_t(std::string folder)
    : folder(folder)
    , naglowek(folder)
    {}

    void printf_wiersz(const wiersz_t& wiersz) const {
        for(uint32_t k = 0; k < naglowek.liczba_kolumn(); k++){
            naglowek.kolumny[k].printf_rekord(wiersz.dane[k]);
            if(k == (naglowek.liczba_kolumn() - 1)){
                printf("\n");
            } else {
                printf("\t");
            }
        }
    }

};

template<typename typ_cos>
struct znajdywacz_w_pliku_t{
    czytacz_pliku_t czytacz_kolumny;
    FILE* fd_cale = nullptr;

    typ_cos wartosc_od;
    typ_cos wartosc_do;

    uint32_t liczba_kolumn = 0;
    typ_t typ_kolumny = kolumna_typ_brak;

    znajdywacz_w_pliku_t(const czytacz_mojego_t& czytacz_mojego, std::string nazwa_kolumny
        , typ_cos wartosc)
        : wartosc_od(wartosc), wartosc_do(wartosc)
        , liczba_kolumn(czytacz_mojego.naglowek.liczba_kolumn()){

        znajdz_w_przedziale(czytacz_mojego, nazwa_kolumny);
        otworz_cale(czytacz_mojego);
    }

    znajdywacz_w_pliku_t(
        const czytacz_mojego_t& czytacz_mojego, std::string nazwa_kolumny
        , typ_cos wartosc_od, typ_cos wartosc_do)
        : wartosc_od(wartosc_od), wartosc_do(wartosc_do)
        , liczba_kolumn(czytacz_mojego.naglowek.liczba_kolumn()){

        znajdz_w_przedziale(czytacz_mojego, nazwa_kolumny);
        otworz_cale(czytacz_mojego);
    }

    void znajdz_w_przedziale(const czytacz_mojego_t& czytacz_mojego, std::string& nazwa_kolumny){
        // wartosc_od i wartosc_do włącznie
        assert(nazwa_kolumny != "id");

        for (const auto& kolumna : czytacz_mojego.naglowek.kolumny) {
            if(kolumna.nazwa == nazwa_kolumny){
                typ_kolumny = kolumna.typ;
                kolumna.znajdz_poczatek(czytacz_kolumny, czytacz_mojego.folder, wartosc_od);
                return;
            }
        }

        assert(false); // Nie znaleziono kolumny
    }

    void otworz_cale(const czytacz_mojego_t& czytacz_mojego){
        fd_cale = moje_io::fopen(czytacz_mojego.folder + "cale.bin", "rb");
    }

    ~znajdywacz_w_pliku_t(){
        fclose(fd_cale);
    }

    wiersz_t nastepny(){
        row_idx_t idx_wiersza;

        switch (typ_kolumny) {

            case(kolumna_typ_double):{
                rekord<double>& rekord_ = *(rekord<double>*)czytacz_kolumny.tu_czytac();
                if (rekord_.wartosc > wartosc_do) {
                    return wiersz_t();
                }
                idx_wiersza = rekord_.idx;
                break;
            }
            case(kolumna_typ_long):{
                rekord<int64_t>& rekord_ = *(rekord<int64_t>*)czytacz_kolumny.tu_czytac();
                if (rekord_.wartosc > wartosc_do) {
                    return wiersz_t();
                }
                idx_wiersza = rekord_.idx;
                break;
            }

            default:{
                assert(false);
            }
        }

        czytacz_kolumny.przeczytano(sizeof(rekord<int64_t>));
        return wiersz_t(liczba_kolumn);
        return przeczytaj_wiersz(fd_cale, liczba_kolumn, idx_wiersza);
    }
    
};

int main(){
    if constexpr (false){
        do_formatu_mojego(
            "C:\\Users\\kupcz\\Desktop\\Astroinfa\\lab1\\large-file.dat", 
            "C:\\Users\\kupcz\\Desktop\\Astroinfa\\lab1\\moj_format_duze\\");
    } else {
        czytacz_mojego_t czytacz_mojego("C:\\Users\\kupcz\\Desktop\\Astroinfa\\lab1\\moj_format_duze\\");
        znajdywacz_w_pliku_t znajdywacz(czytacz_mojego, "c4", 14);

        wiersz_t wiersz = znajdywacz.nastepny();
        uint32_t i = 1;
        while(wiersz.dane.size() > 0){
            // czytacz_mojego.printf_wiersz(wiersz);
            // printf("%lld\n", wiersz.dane[4]);
            wiersz = znajdywacz.nastepny();
            i++;
        }
        printf("znaleziono: %d\n", i);
    }
}






