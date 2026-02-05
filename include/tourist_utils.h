#pragma once
#include <stdint.h>
#include "ipc.h"

/*
 * Pobiera zmienna srodowiskowa name i zwraca jej wartosc jako int; brak -> exit(1).
 */
int tourist_get_env_int(const char *name);
/*
 * Losuje wiek turysty i ewentualnych dzieci; aktualizuje tickets_nbr oraz
 * discount_tickets_nbr. Zwraca wiek doroslego.
 */
int tourist_handle_children(int *tickets_nbr, int *discount_tickets_nbr);
/*
 * Wypelnia strukture ticket_msg_t danymi turysty (wiek, VIP, rowerzysta,
 * liczba biletow, znizki) i losuje requested_pass.
 */
void tourist_fill_ticket_request(ticket_msg_t *req, int age, int is_vip, int is_biker,
                                 int tickets_nbr, int discount_tickets_nbr);
/*
 * Probuje przejsc dolne bramki: pobiera tokeny inside/gate4, obsluguje VIP,
 * loguje przejscie; zwraca liczbe miejsc zajetych w grupie lub 0/-1 przy
 * odmowie/bledzie.
 */
int tourist_do_lower_gate(uint32_t pass_id, pass_type_t pass_type, int valid_until,
                           int sem_inside, int sem_gate4, int group_size, int is_vip);
/*
 * Obsluguje etap peronu: wysyla PLAT_REQ, czeka na odpowiedz, loguje wejscie na
 * platforme, zwalnia tokeny inside; zwraca 0, 1 przy PLAT_SHUTDOWN lub -1 przy bledzie.
 */
int tourist_do_platform_stage(uint32_t pass_id, int is_biker, int is_vip, int group_size, int platform_qid,
                              int sem_inside, int sem_gate3);
/*
 * Przechodzi gorne wyjscie: rezerwuje sem_exit2, loguje, zwraca 0/-1.
 */
int tourist_do_upper_exit(int is_biker, int sem_exit2);
