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

    if ((mutex = sem_open(SEM_NAME_MUTEX, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pid_principal = getpid();

    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    sem_wait(mutex);
    /* REGIÓN CRÍTICA */
    fd_shm_mon_compr = shm_open(SHM_MON_COMPR, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    
    if (fd_shm_mon_compr == -1 && errno == EEXIST)
    {
        /* MONITOR */
        if (getpid() == pid_principal)
        {
            wait(NULL);
        }
        fflush(stdout);
        printf("soy monitor\n");
        exit(EXIT_SUCCESS);
    }
    else if (fd_shm_mon_compr != -1)
    {
        /* COMPROBADOR */
        if (getpid() == pid_principal)
        {
            wait(NULL);
        }
        fflush(stdout);
        printf("soy comprobador\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        if (getpid() == pid_principal)
        {
            wait(NULL);
            exit(EXIT_FAILURE);
        }
        perror("Error creating the shared memory segment\n");
        exit(EXIT_FAILURE);
    }
    sem_post(mutex);

    sem_close(mutex);
    sem_unlink( SEM_NAME_MUTEX );
    close(fd_shm_mon_compr);
    shm_unlink( SHM_MON_COMPR );

    exit(EXIT_SUCCESS);
}
