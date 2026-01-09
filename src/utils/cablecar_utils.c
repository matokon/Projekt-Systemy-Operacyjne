#include "cablecar.h"

void cablecar_init(cablecar_t *cablecar)
{
    cablecar->head = 0;
    cablecar->tail = 0;
    cablecar->occupied = 0;

    for (int i = 0; i < 72; i++) {
        cablecar->seats[i].state = SEAT_FREE;
        cablecar->seats[i].group_size = 0;
        for(int j = 0; j < 4; j++)
        {
            cablecar->seats[i].pids[j] = 0;
        }
    }
}
