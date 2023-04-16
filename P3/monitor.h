#ifndef MONITOR_H
#define MONITOR_H

#include "minero.h"

#define SEM_NAME_MUTEX ".mutex"                     /* NOMBRE DEL ARCHIVO TEMPORAL PARA EL SEMÁFORO DE EXCLUSIÓN MUTUA */
#define SHM_MON_COMPR "/shm_monitor_comprobante"    /* NOMBRE DEL SEGMENTO DE MEMORIA COMPARTIDA ENTRE COMPROBADOR-MONITOR */
#define BUFF_SIZE 6                                 /* TAMAÑO DEL BUFFER (COLA CIRCULAR MONITOR-COMPROBADOR) */

typedef struct {
    int front;                       /*!< PRIMER ELEMENTO DE LA COLA CIRCULAR MONITOR-COMPROBADOR */
    int rear;                        /*!< ÍNDICE DE LA SIGUIENTE POSICIÓN LIBRE DE LA COLA CIRCULAR MONITOR-COMPROBADOR */
    int solutionStatus;              /*!< FLAG DE CONTROL DE LA SOLUCIÓN RECIBIDA PARA SU CORRESPONDIENTE TARGET; CORRECTO = 1, INCORRECTO = 0 */
    ShmData data[BUFF_SIZE];         /*!< COLA CIRCULAR DE BUFF_SIZE ELEMENTOS (TARGET Y SOLUCIÓN), COMUNICA MONITOR-COMPROBADOR */
    sem_t sem_mutex;                 /*!< SEMÁFORO DE EXCLUSIÓN MUTUA ANÓNIMO ALGORITMO PRODUCTOR-CONSUMIDOR */
    sem_t sem_empty;                 /*!< SEMÁFORO DE COLA VACÍA ANÓNIMO ALGORITMO PRODUCTOR-CONSUMIDOR */
    sem_t sem_fill;                  /*!< SEMÁFORO DE COLA LLENA ANÓNIMO ALGORITMO PRODUCTOR-CONSUMIDOR */
} CompleteShMem;

#endif
