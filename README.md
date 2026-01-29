# Kolej linowa - symulacja procesow i IPC

## Nowe mechanizmy po konsultacjach (stabilność IPC)
- **Semafor kredytowy turystów (`sem_tourists`, 200 slotów)**: generator bierze token przed forkiem, turysta oddaje przy wyjściu (`atexit`). Zapobiega lawinie procesów i zapychaniu kolejek bez żadnych sleepów.
- **Łagodne zamykanie peronu**: `employee1` po `PLAT_SHUTDOWN` budzi wszystkie semafory (bramki, limity, krzesła, wyjścia) i przez ~0.5 s drenuje kolejkę peronu, odsyłając spóźnionym `PLAT_SHUTDOWN` w trybie `IPC_NOWAIT`.
- **Nieblokujące sendy do kolejek (IPC_NOWAIT)**: turysta rezygnuje, gdy kolejka biletowa/peronowa jest pełna zamiast się blokować; brak wiszących `msgsnd`.
- **Timeouty po stronie turystów**: oczekiwanie na odpowiedź peronu z limitem czasu (5 s) i zwrotem semaforów, żeby nie blokować strefy inside.
- **Porządki w logach**: ciche traktowanie ENOMSG/EIDRM/EINVAL w IPC, brak spamujących perrorów po zamknięciu.

Symulacja nadal odwzorowuje kolej krzesełkową z VIP-ami, dziećmi, rowerzystami, STOP/WZNOWIENIE itp., ale powyższe zmiany eliminują zwisy i przecieki kolejek.

## 1. Srodowisko i narzedzia

- System operacyjny: Windows 11 + WSL2 (Ubuntu 24.04.3 LTS)
- Język: C11
- Kompilator: GCC (toolchain Ubuntu)
- Zarządzanie kompilacją: Makefile / GNU Make
- Edytor: VS Code (setup WSL)

## 2. Budowanie i uruchomienie

Budowanie:

```
make
```

Uruchomienie (argumenty: Tp, Tk, opcjonalnie stop_once):

```
./projekt <Tp> <Tk> [stop_once]
```

Przyklad:

```
./projekt 0 10
```

Parametr `stop_once`:
- `1` - po 5 sekundach symulacji wysylany jest STOP i nastepnie WZNOWIENIE.
- `0` lub brak - brak wymuszonego zatrzymania.

## 3. Pokrycie wymagan (skrot)

- Krzeselka 4-osobowe, 72 sztuki, max 36 zajetych naraz.
- Dopuszczalne sklady: 2 rowerzystow, albo 1 rowerzysta + 2 pieszych, albo 4 pieszych.
- Losowe przyjscia, czesc turystow moze zrezygnowac.
- Sprzedaz biletow/karnetow w kasie.
- Karnety: jednorazowe, czasowe (Tk1/Tk2/Tk3), dzienne.
- Znizka 25% dla wieku <10 i >65.
- Dzieci <8 pod opieka doroslego; dorosly max 2 dzieci (4-8).
- Bramki dolne: 4 rownolegle.
- Bramki peronu: 3 rownolegle, otwierane przez pracownika1.
- Limit N osob w strefie miedzy bramkami.
- Wyjscie na gorze: 2 rownolegle pasy.
- Pracownik1 i Pracownik2, STOP/WZNOWIENIE sygnalami.
- Trasy zjazdowe T1 < T2 < T3.
- Godziny pracy Tp..Tk, po Tk brak wejsc na bramkach, peron sie oproznia, po 3 s koniec.
- Logowanie uzyc karnetu na bramkach.
- VIP (~1%) bez kolejki, ale z waznym karnetem.
- Raport z liczby przejazdow na koniec.

## 4. Struktura kodu i odpowiedzialnosci

### `src/main.c`
- Parsuje Tp/Tk (i stop_once), sprawdza zakres, ustawia czas startu.
- Tworzy kolejki, semafory (bramki 4/3, limit N, 36 krzesel, 2 wyjscia, mutex shm, kredyt turystów 200) i pamiec dzielona; publikuje je w ENV.
- Inicjalizuje stan kolei w shm (ring 72 krzesel, liczniki, pid pracownikow), czyści `report.txt`.
- Uruchamia kasjera, pracownikow, generator turystow (spawn przez czas Tk-Tp z kontrolą sem_tourists).
- Po Tk: wysyła `PLAT_SHUTDOWN`, czeka na opróżnienie krzeseł, wysyła shutdown do kasy/peronu (z krótkimi timeoutami na ACK), ubija employee2, czeka na procesy, generuje raport, sprząta IPC.

### `src/cashier.c`
- Slucha kolejki biletowej; kazdy request ma pid, wiek, VIP, rowerzysta, liczbe biletow (w tym ulgowych).
- Losuje typ karnetu (lub honoruje podany), nalicza znizke 25% dla <10 i >65, ustawia `issued_at` i `valid_until` (Tk1/Tk2/Tk3/dzienny -> Tk).
- Po uplywie Tk odrzuca sprzedaz (status != ST_OK).
- Odsyla wynik do pid turysty przez msgrcv/msgrcv z mtype=pid.

### `src/employee1.c`
- Rejestruje PID w shm, czeka na PID emp2.
- Obsługuje kolejkę peronu (priorytet VIP): wpuszcza do Tk, po Tk odsyła `PLAT_SHUTDOWN`.
- Kolejkuje rowerzystów i pieszych, formuje składy (2 rowery / 1 rower+2 pieszych / 4 pieszych), rezerwuje miejsca w shm (limit 36).
- Sygnały STOP/WZNOWIENIE (SIGUSR1/2), tryb `stop_once` (pauza po 5 s, wznowienie po ~9 s).
- Na `PLAT_SHUTDOWN`: budzi wszystkie semafory, flushuje oczekujących, drenuje kolejkę ~0.5 s w trybie IPC_NOWAIT i wysyła ACK.

### `src/employee2.c`
- Rejestruje PID w shm, czeka na PID emp1.
- Sygnały STOP/WZNOWIENIE z handshake.
- Opróżnia krzesła z ringu (head++, zwalnia semafor krzeseł); zatrzymuje się na sygnał/koniec.

### `src/tourist.c`
- Pobiera kredyt z `sem_tourists` (przed forkiem), oddaje przy wyjściu (`atexit`).
- Losuje dzieci (max 2); wtedy wyłącza rower.
- Wysyła żądanie do kasy (IPC_NOWAIT — rezygnacja gdy pełna), czeka na odpowiedź.
- Etapy: dolne bramki (limit N + 4 bramki), peron (PLAT_REQ z timeoutem, priorytet VIP), górne wyjście (2 pasy). Loguje do `report.txt`.
- Jednorazowy przejazd (po zmianach: jeden zjazd i koniec procesu).

### `src/ipc.c`
- Wrappery na IPC: kolejki (blokujące i IPC_NOWAIT), semafory (wait/post), shm; helpery ENV; ciche ENOMSG/EIDRM/EINVAL po zamknięciu.

### `src/platform_queue.c`
- Bufory oczekujących: rowerzyści osobno, piesi z rozmiarem grupy.
- Algorytm: 2 rowery -> 1 rower + 2 pieszych -> 4 pieszych. Rezerwacja miejsca (sem krzeseł, mutex shm) i wysyłka PLAT_RES.

### `src/utils/tourist_utils.c`
- Bramki dolne: weryfikuje ważność karnetu, rezerwuje limit N i gate4, loguje.
- Peron: PLAT_REQ (IPC_NOWAIT, timeout 5 s), w razie rezygnacji oddaje sem_inside; loguje przejście.
- Wyjście górne: sem_exit2 (2 pasy), loguje; helpery ENV/log/losowania dzieci.

### `src/utils/main_utils.c`
- Setenvy dla semaforów/liczb, cleanup IPC (kolejki, semy, shm).
- `generate_report`: zlicza wjazdy na peron i dopisuje podsumowanie.

### `include/*.h`
- Struktury, stale, prototypy funkcji, klucze IPC i zmienne srodowiskowe.

## 5. Jak to dziala (opis mechaniki)

### IPC
- Kolejki komunikatow: turysta <-> kasjer, turysta <-> peron.
- Semafory: limit N osob, bramki (4 i 3), krzeselka (36), wyjscia (2), dostep do shm.
- Pamiec dzielona: stan kolei (ring krzeselek, liczniki, pid pracownikow).

### Kolejka i krzeselka
- Peron sklada grupy w zgodzie z zasadami (piesi/rowerzysci, dzieci).
- Krzeselka trzymane w ringu o dlugosci 72, z limitem 36 jednoczesnie zajetych.

### Czas i wygasanie karnetow
- Tp i Tk sa parametrami uruchomienia.
- Karnety czasowe: `valid_until = issued_at + TkX`.
- Karnet dzienny: `valid_until = Tk`.
- Turysta jezdzi w petli do przekroczenia `valid_until`.

### STOP/WZNOWIENIE
- STOP (SIGUSR1) wysylany do drugiego pracownika.
- Odpowiedz WZNOWIENIE (SIGUSR2) po potwierdzeniu gotowosci.
- `stop_once` wymusza pojedyncze zatrzymanie po 5 sekundach.

## 6. Logi i raport

- `report.txt` zawiera log przejść przez bramki (pass_id + czas + gate).
- Na końcu pliku dopisywane jest podsumowanie liczby przejazdów na karnet (gate=platform).

## 7. Testy (manualne)

- Poprawne składy i limit 36 miejsc – kontrolowane w `platform_try_form_groups` + semafor krzeseł, zwalniany w `employee2`.
- Natychmiastowe zatrzymanie/wznowienie – sygnały SIGUSR1/2 między pracownikami, opcjonalny `stop_once`.
- VIP przed zwykłymi – peron odbiera z priorytetem VIP (mtype ujemny).
- Sprzątanie zasobów – `cleanup_ipc` usuwa kolejki/semafory/shm; po zakończeniu `ipcs` puste.

## 8. Funkcje wymagane przez projekt (gdzie szukać)

- Tworzenie procesów: `fork`/`execl` i `waitpid` w [process_utils.c](src/utils/process_utils.c#L10-L117); pętlowy spawn w [spawn_processes_for_seconds_collect](src/utils/process_utils.c#L25-L80).
- Sygnały: obsługa `kill`/`signal` w [employee1.c](src/employee1.c#L58-L125) i [employee2.c](src/employee2.c#L17-L80).
- Kolejki komunikatów: `msgget`, `msgsnd`, `msgrcv`, `msgctl` w [ipc.c](src/ipc.c#L20-L196).
- Semafory: `semget`, `semctl`, `semop` (wait/post) w [ipc.c](src/ipc.c#L60-L133).
- Pamięć dzielona: `shmget`, `shmat`, `shmdt`, `shmctl` w [ipc.c](src/ipc.c#L198-L233).
- Pliki/raporty: `fopen`/`fclose` w [tourist_utils.c](src/utils/tourist_utils.c#L14-L31), `fprintf` w [main_utils.c](src/utils/main_utils.c#L76-L106).
