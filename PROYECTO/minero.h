#ifndef MINERO_H
#define MINERO_H

#define MAX_MINER 100
#define INITIAL_TARGET 0
#define SEM_NAME_SHM_CREATION ".shmCreation_mutex"
#define SEM_NAME_SEND_SIGNAL ".shmSignal_mutex"
#define SHM_NAME_MINERS "./shm_miners"

typedef struct {
    long pow_ini;
    long pow_fin;
    long target;
} MiningData;

#endif
