#ifndef TYPES_H
#define TYPES_H

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <mqueue.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "minero.h"
#include "pow.h"

#define MQ_NAME "/mq_minero_comprobador"
#define MQ_SIZE 10
#define END_BLOCK -1

typedef struct {
    pid_t MinersWalletPID;  /*!< PID DEL PROPIETARIO DE LA CARTERA */
    unsigned long n_coins;  /*!< NÚMERO DE MONEDAS DE LA CARTERA */
} Wallet;

typedef enum VOTE_TYPES {
    NEGATIVE,   /*!< 0 */
    POSITIVE    /*!< 1 */
} Votes;

typedef struct {
    unsigned int ID;            /*!< ID DEL BLOQUE */
    long target;                /*!< TARGET DE ESTA RONDA */
    long solution;              /*!< SOLUTION PARA EL TARGET */
    pid_t FstMiner;             /*!< PID DEL PRIMER MINERO EN ENCONTRAR EL RESULTADO */
    Wallet wallets[MAX_MINER];  /*!< TODAS LAS CARTERAS ACTIVAS */
    int total_n_votes;          /*!< NÚMERO TOTAL DE VOTOS PARA EL MINERO QUE HA ENCONTRADO LA SOLUCIÓN */
    int positive_n_votes;       /*!< NÚMERO DE VOTOS POSITIVOS PARA EL MINERO QUE HA ENCONTRADO LA SOLUCIÓN */
} InfoBlock;

typedef struct {
    unsigned int minerCount;        /*!< NÚMERO DE MINEROS ACTIVOS */
    pid_t MinersPIDS[MAX_MINER];    /*!< ALMACENAMIENTO DE LOS PIDS DE LOS MINEROS ACTIVOS */   
    Votes votes[MAX_MINER - 1];     /*!< 0 ó 1 DEPENDIENDO DE SI EL VOTO ES POSITIVO O NEGATIVO */
    Wallet wallets[MAX_MINER];      /*!< CARTERA ASIGNADA A CADA MINERO ACTIVO */
    InfoBlock last_infoBlock;       /*!< ÚLTIMO BLOQUE ALMACENADO */
    InfoBlock current_infoBlock;    /*!< BLOQUE ACTUAL ALMACENADO */     
} SystemMemory;

struct mq_attr attributes = {   .mq_flags = 0,                    /*!< FLAGS COMPARTIDAS ENTRE MINERO-COMPROBADOR */
                                .mq_maxmsg = MQ_SIZE,             /*!< TAMAÑO DE LA COLA */
                                .mq_curmsgs = 0,                  /*!< MENSAJES INICIALES EN LA COLA */
                                .mq_msgsize = sizeof(InfoBlock)   /*!< TAMAÑO DEL MENSAJE */
                            };

#endif
