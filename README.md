Biblioteka i format pliku do szybkiego szukania wierszy z wartością w przedziale i wczytania ich.

Format pozwala praktycznie natychmiastowo znaleźć które wiersze zawierają wartość w danym przedziale. Format mój pod względem czasu jest równoważny formatowi Filpa Kojro. Uważam, że mój jest lepszy, ale nie jest to tak ważne.

Nie należy za bardzo się przejmować czasem znaleziena indeksów wierszy. A to jest to co robiliśmy. O wiele dłużej zajmuje samo wczytanie wierszy. Czasy co pokazaliśmy na zajęciach nie są prawdziwe(~ 50 ms). System operacyjny te pliki cacheował i czytaliśmy z ramu.

Przy pomocy komendy echo 3 | sudo tee /proc/sys/vm/drop_caches która usuwa te cache. Sprawdziłem czas wykonania i okazuje się, że za pierwszym razem po wywołaniu komendy zwyczajnie czytając plik zajmuje to ~ 6s. Kolejne wywołanie programu już czyta z cachu w ramie i znowu jest to ~ 50ms.

Da się wczytać szybciej z dysku. Użyłem bibloteki do asynchronicznego IO i czas wczytywania wierszy wynosi teraz ~ 200ms.
Nie jest to czytanie z cachea. Szybciej się nie da. Użyłem syntetycznego testu random readów na moim dysku i ocenił go na ~ 200 kIOPS na 4KB rozmiaru bloku dysku. Co oznacza, że aby wczytać to testowe 50k wierszy potrzeba 1/4 sekundy(zakładam, że każde czytanie wymaga czytania osobnego bloku). Mój kod robi to w 1/5 sekundy. Minimalnie lepiej niż syntetyczny test, pewnie bo nie czytam losowo z dysku tylko jednego pliku. Uważam, że lepiej będzie ciężko zrobić, jeśli nie niemożliwe.

Nie kontynuuje rozwijania tego formatu. Chcę zajrzeć w CMC zamiast tego. Udało mi się natomiast znaleźć jak maksymalnie 
szybko wczytać pojedyńcze wiersze z pliku. I jak szybko znaleźć wiersze które mają wartość w kolumnie w danym przedziale.


