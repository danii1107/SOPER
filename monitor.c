#include "monitor.h"

int main(int argc, char **argv)
{
    int lag = 0;
    int fd_shm_mon_compr;
    pid_t pid = -1;
    pid_t pid_principal;
    sem_t *mutex = NULL;

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

    pid_principal = getpid();

    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if ((mutex = sem_open(SEM_NAME_MUTEX, O_CREAT | O_RDWR , S_IRWXU, 1)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sem_wait(mutex);
    /* SECCIÓN CRÍTICA */
    fd_shm_mon_compr = shm_open(SHM_MON_COMPR, O_CREAT | O_EXCL, 0200);
    if (fd_shm_mon_compr == -1 && errno == EEXIST)
    {
        /* MONITOR */
        sem_post(mutex);
        fflush(stdout);
        printf("soy monitor\n");
        sem_wait(mutex);
        if(pid_principal == getpid()) wait(NULL);
        close(fd_shm_mon_compr);
        sem_close(mutex);
        sem_unlink( SEM_NAME_MUTEX );
        shm_unlink( SHM_MON_COMPR );
        exit(EXIT_SUCCESS);
    }
    else if (fd_shm_mon_compr != -1)
    {
        /* COMPROBADOR */
        fflush(stdout);
        printf("soy comprobador\n");
        sem_post(mutex);
        if(pid_principal == getpid()) wait(NULL);
        close(fd_shm_mon_compr);
        sem_close(mutex);
        sem_unlink( SEM_NAME_MUTEX );
        shm_unlink( SHM_MON_COMPR );
        exit(EXIT_SUCCESS);
    }
    else
    {
        sem_close(mutex);
        sem_unlink( SEM_NAME_MUTEX );
        if (getpid() == pid_principal)
        {
            wait(NULL);
        }
        perror("Error creating the shared memory segment\n");
        exit(EXIT_FAILURE);
    }
    /* FIN SECCIÓN CRÍTICA */
    sem_post(mutex); 

    sem_close(mutex);
    sem_unlink( SEM_NAME_MUTEX );
    close(fd_shm_mon_compr);
    shm_unlink( SHM_MON_COMPR );

    exit(EXIT_SUCCESS);
}
