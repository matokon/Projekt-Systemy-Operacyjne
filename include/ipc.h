#pragma once
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>

// priorytety w kolejce
#define MT_VIP_OR_CTRL 1
#define MT_NORMAL      2

typedef enum {
    MSG_TICKET_REQ = 1,
    MSG_TICKET_RES = 2,
    MSG_SHUTDOWN   = 3,
    MSG_SHUTDOWN_ACK = 4
} msg_kind_t;

typedef enum {
    PASS_SINGLE = 1,
    PASS_TK1    = 2,
    PASS_TK2    = 3,
    PASS_TK3    = 4,
    PASS_DAY    = 5
} pass_type_t;

typedef enum {
    ST_OK = 0,
    ST_REJECTED_CLOSED,     // po Tk / kasa zamknięta
    ST_INVALID_REQUEST      // np. błędne dane
} status_t;

typedef struct {
    long mtype;            // REQUEST: MT_VIP_OR_CTRL lub MT_NORMAL (priorytet w kolejce)
                           // RESPONSE: pid klienta (adresowanie odpowiedzi)

    msg_kind_t kind;       // czy to prośba czy odpowiedź
    pid_t pid;             // PID klienta

    //dane klienta / request
    int age;               // wiek
    int is_vip;            // 1 = VIP, 0 = nie
    int is_biker;          // 1 = rowerzysta, 0 = pieszy
    pass_type_t requested_pass;   // co chce kupić 0 = nie podano / losowo

    int tickets_nbr;            // ile biletów (jeśli kupuje dla grupy)
    int discount_tickets_nbr;   // ile ze zniżką (opcjonalnie)

    //odpowiedź kasjera
    status_t status;        // ST_OK albo powód odrzucenia
    pass_type_t assigned_pass; // co kasjer przydzielił
    int discount_applied;   // np. 25 albo 0 (nie dotyczy dzieci <8 wtedy patrzymy z discount_tickets_nbr)
    uint32_t pass_id;       // ID karnetu do rejestracji na bramkach
    int issued_at;          // czas wydania (sekundy symulacji)
    int valid_until;        // czas waznosci (sekundy symulacji)

} ticket_msg_t;

#define TICKET_MSGSZ (sizeof(ticket_msg_t) - sizeof(long))

typedef enum {
    PLAT_REQ = 1,
    PLAT_RES = 2,
    PLAT_SHUTDOWN = 3,
    PLAT_SHUTDOWN_ACK = 4
} platform_kind_t;

typedef struct {
    long mtype;            // REQUEST: MT_VIP_OR_CTRL lub MT_NORMAL, RESPONSE: pid klienta
    platform_kind_t kind;
    pid_t pid;
    int is_biker;
    int group_size;        // liczba miejsc zajmowanych przez osobe + dzieci
    uint32_t pass_id;
    int issued_at;          // czas wydania (sekundy symulacji)
    int valid_until;        // czas waznosci (sekundy symulacji)
} platform_msg_t;

#define PLATFORM_MSGSZ (sizeof(platform_msg_t) - sizeof(long))

//ipc
/*
 * Tworzy prywatna kolejke komunikatow System V i zwraca jej identyfikator;
 * w razie bledu wypisuje perror i konczy proces.
 * Zwraca id kolejki komunikatow (qid).
 */
int  ipc_create_queue(void);
/*
 * Odczytuje maksymalny limit bajtow w kolejce (msg_qbytes) dla podanego qid.
 * qid - id kolejki komunikatow.
 * Zwraca wartosc msg_qbytes lub 0, gdy msgctl(IPC_STAT) zwroci blad.
 */
size_t ipc_get_qbytes(int qid);
/*
 * Zapisuje identyfikator kolejki w zmiennej srodowiskowej IPC_ENV_QID
 * (nadpisuje istniejaca).
 * qid - id kolejki do wpisania do env.
 */
void ipc_set_env_qid(int qid);
/*
 * Pobiera identyfikator kolejki z IPC_ENV_QID; konczy proces, jesli zmienna
 * nie istnieje.
 * Zwraca id kolejki odczytane z env.
 */
int  ipc_get_qid_from_env(void);
/*
 * Wylicza startowa wartosc semafora straznika kolejki na podstawie jej
 * pojemnosci i rozmiaru komunikatu.
 * qid - id kolejki, parametry pobierane przez msgctl(IPC_STAT).
 * msgsz_payload - rozmiar ladunku komunikatu (bez mtype).
 * Zwraca liczbe slotow do ustawienia w semaforze; minimum 1.
 */
int  ipc_calc_guard_init(int qid, size_t msgsz_payload);
/*
 * Usuwa kolejke komunikatow qid (IPC_RMID).
 * qid - id kolejki do skasowania.
 * Zwraca 0 przy sukcesie, -1 gdy msgctl zwroci blad.
 */
int  ipc_destroy_queue(int qid);
/*
 * Wysyla komunikat ticket_msg_t na kolejke; opcjonalnie czeka na semafor
 * straznika powiazany z qid.
 * qid - id kolejki komunikatow.
 * m   - wskaznik na wysylany komunikat.
 * Zwraca 0 przy sukcesie, -1 gdy msgsnd lub operacja na semaforze zwroci blad.
 */
int ipc_send(int qid, const ticket_msg_t *m);
/*
 * Stosuje backpressure: czeka az oblozenie kolejki spadnie ponizej progu
 * (domyslnie 0.7), nastepnie wysyla komunikat.
 * qid - id kolejki komunikatow.
 * m   - wskaznik na wysylany komunikat.
 * threshold - procent wypelnienia kolejki (0 < t < 1), po ktorym wysylka
 *             jest dozwolona.
 * Zwraca kod z ipc_send (0 lub -1).
 */
int ipc_send_with_backpressure(int qid, const ticket_msg_t *m, double threshold);
/*
 * Odbiera komunikat ticket_msg_t z kolejki; po udanym odbiorze zwalnia
 * ewentualny semafor straznika.
 * qid   - id kolejki komunikatow.
 * mtype - filtr typu (jak w msgrcv).
 * m     - bufor na komunikat.
 * Zwraca liczbe bajtow danych lub -1 przy bledzie.
 */
int ipc_recv(int qid, long mtype, ticket_msg_t *m, int flags);

/*
 * Wysyla komunikat platform_msg_t na kolejke platformowa, z uzyciem
 * opcjonalnego semafora straznika.
 * qid - id kolejki platformowej.
 * m   - wskaznik na wysylany komunikat.
 * Zwraca 0 przy sukcesie, -1 w razie bledu msgsnd lub semafora.
 */
int ipc_send_platform(int qid, const platform_msg_t *m);
/*
 * Odbiera komunikat platform_msg_t z kolejki platformowej; po sukcesie zwalnia
 * semafor straznika, jesli istnieje.
 * qid   - id kolejki platformowej.
 * mtype - filtr typu (jak w msgrcv).
 * m     - bufor na komunikat.
 * flags - flagi msgrcv.
 * Zwraca liczbe bajtow danych lub -1 przy bledzie.
 */
int ipc_recv_platform(int qid, long mtype, platform_msg_t *m, int flags);

//shared memory
/*
 * Tworzy segment pamieci wspoldzielonej System V (IPC_PRIVATE) o zadanym
 * rozmiarze; konczy proces przy bledzie.
 * size - rozmiar segmentu w bajtach.
 * Zwraca identyfikator segmentu pamieci.
 */
int  ipc_create_shm(size_t size);
/*
 * Dolacza segment pamieci shmid do przestrzeni adresowej procesu.
 * shmid - identyfikator segmentu pamieci.
 * Zwraca wskaznik na poczatek przydzielonego obszaru; konczy proces przy bledzie.
 */
void* ipc_attach_shm(int shmid);
/*
 * Odlacza poprzednio dolaczony segment pamieci wspoldzielonej.
 * addr - adres zwrocony przez ipc_attach_shm.
 * Zwraca 0 przy sukcesie, -1 gdy shmdt zwroci blad.
 */
int  ipc_detach_shm(void *addr);
/*
 * Usuwa segment pamieci wspoldzielonej (IPC_RMID).
 * shmid - identyfikator segmentu do skasowania.
 * Zwraca 0 przy sukcesie, -1 w razie bledu.
 */
int  ipc_destroy_shm(int shmid);

//semaphores
/*
 * Tworzy jednoelementowy semafor System V (IPC_PRIVATE) i ustawia jego wartosc
 * poczatkowa.
 * proj_id  - nieuzywany identyfikator projektu (dla zgodnosci interfejsu).
 * init_val - wartosc startowa semafora.
 * Zwraca id semafora; konczy proces przy bledzie.
 */
int  ipc_create_sem(char proj_id, int init_val);
/*
 * Zapisuje id semafora w zmiennej srodowiskowej o podanej nazwie.
 * env_name - nazwa zmiennej.
 * semid    - id semafora do zapisania.
 */
void ipc_set_env_sem(const char *env_name, int semid);
/*
 * Pobiera id semafora z podanej zmiennej srodowiskowej; konczy proces, jesli
 * zmienna nie istnieje.
 * env_name - nazwa zmiennej srodowiskowej.
 * Zwraca id semafora odczytany z env.
 */
int  ipc_get_sem_from_env(const char *env_name);
/*
 * Usuwa semafor System V (IPC_RMID).
 * semid - id semafora.
 * Zwraca 0 przy sukcesie, -1 w razie bledu.
 */
int  ipc_destroy_sem(int semid);
/*
 * Wykonuje operacje P (dekrementacje) na semaforze; ponawia probe po EINTR.
 * semid - id semafora.
 * Zwraca 0 przy sukcesie, -1 gdy semop zwroci blad inny niz EINTR.
 */
int  ipc_sem_wait(int semid);
/*
 * Wykonuje operacje V (inkrementacje) na semaforze; ponawia probe po EINTR.
 * semid - id semafora.
 * Zwraca 0 przy sukcesie, -1 przy bledzie semop.
 */
int  ipc_sem_post(int semid);


// data_randomization
/*
 * Zwraca zadany rodzaj karnetu, jesli jest poprawny; w przeciwnym razie losuje
 * typ PASS_SINGLE..PASS_DAY.
 * req - prosba uzytkownika o konkretny karnet (lub 0/niepoprawny).
 * Zwraca wybrany typ karnetu z zakresu PASS_SINGLE..PASS_DAY.
 */
pass_type_t pick_pass(pass_type_t req);
/*
 * Losuje karnet z prawdopodobienstwem 75%; w 25% przypadkow zwraca 0 (brak
 * wyboru).
 * Zwraca 0 lub wylosowany typ karnetu PASS_SINGLE..PASS_DAY.
 */
pass_type_t rand_pass_or_zero(void);
/*
 * Zwraca 1 z prawdopodobienstwem 1% (VIP), w przeciwnym razie 0.
 * Zwraca 1 dla VIP, 0 dla pozostalych.
 */
int rand_vip_1pct(void);
/*
 * Losuje wiek pasazera z przedzialu 5-80 lat.
 * Zwraca wylosowany wiek w latach.
 */
int rand_age(void);
