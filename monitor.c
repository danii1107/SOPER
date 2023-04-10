#include "monitor.h"

int main(int argc, char **argv)
{
    int lag = 0;
    int fd_shm_mon_compr;
    int i = 1;
    sem_t *mutex = NULL;
    sem_t *waitBothProcs = NULL;

    if (argc != 2)
    {
        fprintf(stderr, "Execution format: ./monitor <LAG>\n");
        exit(EXIT_SUCCESS);
    }

    lag = atoi(argv[1]);
    if (lag < 0)
    {
        fprintf(stderr, "Invalid lag\n");
        exit(EXIT_SUCCESS);
    }

    if ((mutex = sem_open(SEM_NAME_MUTEX, O_CREAT | O_RDWR , S_IRWXU, 1)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((waitBothProcs = sem_open(SEM_NAME_WAIT, O_CREAT | O_RDWR , S_IRWXU, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sem_wait(mutex);
    /* SECCIÓN CRÍTICA */
    fd_shm_mon_compr = shm_open(SHM_MON_COMPR, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd_shm_mon_compr == -1 && errno == EEXIST)
    {
        /* MONITOR */
        sem_post(mutex);
        fflush(stdout);
        printf("MONITOR\n");
        /* PERMITE A COMPROBADOR TERMINAR */
        sem_post(waitBothProcs);


       
        close(fd_shm_mon_compr);
        shm_unlink( SHM_MON_COMPR );
        sem_close(waitBothProcs);
        sem_unlink( SEM_NAME_WAIT );
        sem_close(mutex);
        sem_unlink( SEM_NAME_MUTEX );
       
        exit(EXIT_SUCCESS);
    }
    else if (fd_shm_mon_compr != -1)
    {
        /* COMPROBADOR */
        fflush(stdout);
        printf("COMPROBADOR\n");
        sem_post(mutex);
        /* ESPERA A QUE MONITOR ACABE */
        sem_wait(waitBothProcs);

        i = 1;
        while(i != 25 /* BLOQUE NO SEA BLOQUE DE FINALIZACIÓN */)
        {
            /* RECIBIR BLOQUE COLA DE MENSAJES */
            /* COMPROBAR BLOQUE */
            i++;
        }
        
        close(fd_shm_mon_compr);
        shm_unlink( SHM_MON_COMPR );
        sem_close(waitBothProcs);
        sem_unlink( SEM_NAME_WAIT );
        sem_close(mutex);
        sem_unlink( SEM_NAME_MUTEX );
        
        exit(EXIT_SUCCESS);
    }
    else
    {
        sem_close(waitBothProcs);
        sem_unlink( SEM_NAME_WAIT );
        sem_close(mutex);
        sem_unlink( SEM_NAME_MUTEX );
        perror("Error creating the shared memory segment\n");
        exit(EXIT_FAILURE);
    }
    /* FIN SECCIÓN CRÍTICA */
    sem_post(mutex); 

    sem_close(waitBothProcs);
    sem_unlink( SEM_NAME_WAIT );
    sem_close(mutex);
    sem_unlink( SEM_NAME_MUTEX );
    close(fd_shm_mon_compr);
    shm_unlink( SHM_MON_COMPR );

    exit(EXIT_SUCCESS);
}
