#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#define SEM_KEY 1639

typedef enum
{
    SEM_OK      =   0,  
    SEM_INIT_FAIL,
    SEM_OP_FAIL,
} sem_error_t;

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
    (Linux specific) */
};

int sem_init();
void sem_final();
int sem_p();
int sem_v();



#endif //:~ UTIL_SEMAPHORE_H
