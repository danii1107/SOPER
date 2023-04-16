#ifndef MINERO_H
#define MINERO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <mqueue.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "pow.h"

#define INITIAL_TARGET 0                        /* OBJETIVO INICIAL DEL MINERO */
#define END_BLOCK -1                            /* BLOQUE DE FINALIZACIÓN (CUANDO ACABEN LAS RONDAS, 
                                                    MINERO LO METE A LA MQ PARA ACABAR COMPROBANTE Y MONITOR) */
#define NUM_THREADS 10                          /* NÚMERO DE HILOS A UTILIZAR */
#define MQ_NAME "/mq_minero_comprobador"        /* NOMBRE DE LA COLA DE MENSAJES MINERO-COMPROBADOR */
#define MQ_SIZE 7                               /* TAMAÑO DE LA COLA DE MENSAJES MINERO-COMPROBADOR */

typedef struct {
    long target;                                                /*!< OBJETIVO DEL MINERO */
    long solution;                                              /*!< SOLUCIÓN PARA DICHO OBJETIVO */
} ShmData;          

typedef struct _Data{
    long pow_ini;           /*!< INICIO INTERVALO DE BÚSQUEDA DEL HILO */
    long pow_fin;           /*!< FIN INTERVALO DE BÚSQUEDA DEL HILO */
    long target;            /*!< TARGET DEL MINERO */
}MinData;

struct mq_attr attributes = {   .mq_flags = 0,                  /*!< FLAGS COMPARTIDAS ENTRE MINERO-COMPROBADOR */
                                .mq_maxmsg = MQ_SIZE,           /*!< TAMAÑO DE LA COLA */
                                .mq_curmsgs = 0,                /*!< MENSAJES INICIALES EN LA COLA */
                                .mq_msgsize = sizeof(ShmData)   /*!< TAMAÑO DEL MENSAJE */
                            };

#endif 