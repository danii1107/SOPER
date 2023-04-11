#ifndef MONITOR_H
#define MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sem.h>

#define SEM_NAME_MUTEX ".mutex"                     /* NOMBRE DEL ARCHIVO TEMPORAL PARA EL SEMÁFORO DE EXCLUSIÓN MUTUA */
#define SHM_MON_COMPR "/shm_monitor_comprobante"    /* NOMBRE DEL SEGMENTO DE MEMORIA COMPARTIDA ENTRE COMPROBADOR-MONITOR */
#define MQ_SIZE 6                                   /* TAMAÑO DE LA COLA DE MENSAJES */

typedef struct {
    long target;
    long solution;
} ShmData;

typedef struct {
    ShmData data[MQ_SIZE];
    sem_t sem_mutex;
    sem_t sem_empty;
    sem_t sem_fill;
} CompleteShMem;

#endif
