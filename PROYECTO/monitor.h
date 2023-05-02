#ifndef MONITOR_H
#define MONITOR_H

#include "types.h"

#define SHM_MON_COMPR "./shm_monitor_comprobador"
#define BUFF_SIZE 5

typedef struct {
    int front;                       /*!< PRIMER ELEMENTO DE LA COLA CIRCULAR MONITOR-COMPROBADOR */
    int rear;                        /*!< ÍNDICE DE LA SIGUIENTE POSICIÓN LIBRE DE LA COLA CIRCULAR MONITOR-COMPROBADOR */
    int solStatus;                   /* */
    InfoBlock data[BUFF_SIZE];       /*!< COLA CIRCULAR DE BUFF_SIZE ELEMENTOS (BLOQUE DEL MINERO), COMUNICA MONITOR-COMPROBADOR */
    sem_t sem_mutex;                 /*!< SEMÁFORO DE EXCLUSIÓN MUTUA ANÓNIMO ALGORITMO PRODUCTOR-CONSUMIDOR */
    sem_t sem_empty;                 /*!< SEMÁFORO DE COLA VACÍA ANÓNIMO ALGORITMO PRODUCTOR-CONSUMIDOR */
    sem_t sem_fill;                  /*!< SEMÁFORO DE COLA LLENA ANÓNIMO ALGORITMO PRODUCTOR-CONSUMIDOR */
} CompleteShMem;

#endif
