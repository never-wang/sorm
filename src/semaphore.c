#include <sys/sem.h>
#include <string.h>
#include <errno.h>

#include "semaphore.h"
#include "log.h"


int sem_init(int sem_key)
{
    union semun sem_union;
    int sem_id; 

    sem_id = semget(sem_key, 1, 0);
    if(sem_id == -1){   /* need create a new one */
        sem_id = semget(sem_key, 1, 0666 | IPC_CREAT);
        sem_union.val = 1;
        if(semctl(sem_id, 0, SETVAL, sem_union) == -1){
            printf("semctl fail : %s\n", strerror(errno));
            return SEM_INIT_FAIL;
        }
    }
    return SEM_OK;
}

void sem_final()
{
    //int sem_id;
    //union semun sem_union;
    //if((sem_id = semget(SEM_KEY, 1, 0)) == -1){
    //    return;
    //}

    //semctl(sem_id, 0, IPC_RMID, sem_union);
}

int sem_p(int sem_key)
{
    struct sembuf sem_b;
    union semun sem_union;
    int sem_id;

    sem_id = semget(sem_key, 1, 0);
    if(sem_id == -1){   /* need create a new one */
        sem_id = semget(sem_key, 1, 0666 | IPC_CREAT);
        sem_union.val = 1;
        if(semctl(sem_id, 0, SETVAL, sem_union) == -1){
            printf("semctl fail : %s\n", strerror(errno));
            return SEM_INIT_FAIL;
        }
    }

    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(sem_id, &sem_b, 1) == -1){
        log_error("semop fail : %s", strerror(errno));
        return SEM_OP_FAIL;
    }

    return SEM_OK;
}

int sem_v(int sem_key)
{
    struct sembuf sem_b;
    union semun sem_union;
    int sem_id;
    
    sem_id = semget(sem_key, 1, 0);
    if(sem_id == -1){   /* need create a new one */
        sem_id = semget(sem_key, 1, 0666 | IPC_CREAT);
        sem_union.val = 1;
        if(semctl(sem_id, 0, SETVAL, sem_union) == -1){
            printf("semctl fail : %s\n", strerror(errno));
            return SEM_INIT_FAIL;
        }
    }

    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;

    if(semop(sem_id, &sem_b, 1) == -1){
        log_error("semop fail : %s", strerror(errno));
        return SEM_OP_FAIL;
    }
    
    return SEM_OK;
}

