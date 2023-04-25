#ifndef MINERO_H
#define MINERO_H

#define INITIAL_TARGET 0
#define SEM_NAME_SHM_CREATION ".shmCreation_mutex"
#define SHM_NAME_MINERS "./shm_miners"

typedef struct {
    long pow_ini;
    long pow_fin;
    long target;
} MiningData;

#endif
