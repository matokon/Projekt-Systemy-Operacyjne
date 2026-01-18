#pragma once
#include <stdint.h>
#include "ipc.h"

int tourist_get_env_int(const char *name);
int tourist_handle_children(int *tickets_nbr, int *discount_tickets_nbr);
void tourist_fill_ticket_request(ticket_msg_t *req, int age, int is_vip, int is_biker,
                                 int tickets_nbr, int discount_tickets_nbr);
int tourist_do_lower_gate(uint32_t pass_id, pass_type_t pass_type, int valid_until,
                           int sem_inside, int sem_gate4, int group_size);
int tourist_do_platform_stage(uint32_t pass_id, int is_biker, int is_vip, int group_size, int platform_qid,
                              int sem_inside, int sem_gate3);
int tourist_do_upper_exit(int is_biker, int sem_exit2);
