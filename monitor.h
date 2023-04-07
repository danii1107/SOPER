#ifndef MONITOR_H
#define MONITOR_H

#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>

#define SEM_NAME_MUTEX "/mutex"
#define SHM_MON_COMPR "/shm_monitor_comprobante"

#endif