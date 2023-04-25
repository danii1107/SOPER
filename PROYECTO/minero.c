#include "types.h"

int stop = 0;
unsigned int minerCount = 0;
long MinerResult = 0;

/**
 * @brief Busca la solución del target y la guarda en
 *        la variable global MinerResult
 *
 * @param data estructura de datos del proceso
 * @return NULL
 */
void* solve(void* data)
{
    if (!data) return NULL;

    long i = 0;
    MiningData* d = data;

    i = d->pow_ini;
    while(i < d->pow_fin && stop == 0)
    {
        if(d->target == pow_hash(i)) 
        {
            MinerResult = i;
            stop = 1;
        }
        i++;
    }
    return NULL;
}

/**
 * @brief crea los hilos y les adjudica su intervalo de búsqueda
 *
 * @param n_threads números de hilos
 * @param target objetivo del proceso
 * @return 0 si funciona correctamente o 1 si algo da error
 */
int minero(int n_threads, long target)
{
    int i;
    int error;
    MiningData data[n_threads];
    pthread_t* threads;

    stop = 0;

    if (n_threads <= 0 || target < 0 || target >= POW_LIMIT) return EXIT_FAILURE;

    threads = (pthread_t*) malloc(sizeof(pthread_t) * n_threads);
    if(!threads) return EXIT_FAILURE;

    /* CREACIÓN DE LOS INTERVALOS DE BÚSQUEDA PARA CADA HILO */
    for (i = 0; i < n_threads; i++)
    {
        data[i].pow_ini = i * POW_LIMIT/n_threads;
        data[i].pow_fin = (i+1) * POW_LIMIT/n_threads;
        data[i].target = target;
        if (data[i].pow_ini > 0) (data[i].pow_ini)++;
    }

    /* LANZAMIENTO DE LOS HILOS */
    for(i = 0; i < n_threads; i++)
    {
        error = pthread_create(&threads[i], NULL, solve, &data[i]);
        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            return EXIT_FAILURE;
        }
    }
    for(i = 0; i < n_threads; i++)
    {
        error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            return EXIT_FAILURE;
        }
    }

    free(threads);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{  
    int n_threads, exe_seconds;
    int fd[2], pipe_status;
    int fd_shm_miners;
    pid_t pid;
    InfoBlock* info_to_register, *winnerMinerInfo;
    sem_t* minerCount_mutex, *shmCreation_mutex;
    SystemMemory* sys_info;

    if (argc != 3)
    {
        fprintf(stderr, "Execution format: ./minero  < N_SECONDS > < N_THREADS >\n");
        exit(EXIT_FAILURE);
    }

    /* GUARDADO DE LOS PARÁMETROS EN VARIABLES LOCALES */
    exe_seconds = atoi(argv[1]);
    if (exe_seconds <= 0)
    {
        fprintf(stderr, "Invalid rounds\n");
        exit(EXIT_FAILURE);
    }
    n_threads = atoi(argv[2]);
    if (n_threads <= 0)
    {
        fprintf(stderr, "Invalid lag\n");
        exit(EXIT_FAILURE);
    }

    /* CREACIÓN E INICIALIZACIÓN A 1 DEL SEMÁFORO MUTEX */
    if ((minerCount_mutex = sem_open(SEM_NAME_MINER_COUNT, O_CREAT | O_RDWR , S_IRWXU, 1)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((shmCreation_mutex = sem_open(SEM_NAME_SHM_CREATION, O_CREAT | O_RDWR , S_IRWXU, 1)) == SEM_FAILED)
    {
        sem_close(minerCount_mutex);
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pipe_status = pipe(fd);
    if(pipe_status == -1)
    {
        sem_close(minerCount_mutex);
        sem_close(shmCreation_mutex);
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    sem_wait(shmCreation_mutex);
    fd_shm_miners = shm_open(SHM_NAME_MINERS, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_shm_miners == -1 && errno == EEXIST)
    {
        /**/
        sem_post(shmCreation_mutex);
    } 
    else if (fd_shm_miners != -1)
    {
        /**/
        sem_post(shmCreation_mutex);
    }
    else 
    {
        sem_close(minerCount_mutex);
        sem_close(shmCreation_mutex);
        close(fd[0]);
        close(fd[1]);
        exit(EXIT_FAILURE);
    }

    sem_wait(minerCount_mutex);
    



    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        /* REGISTRADOR */
        close(fd[1]);
        if(read(fd[0], info_to_register, sizeof(InfoBlock)) == -1)
        {
            close(fd[0]);
            perror("read");
            exit(EXIT_FAILURE);
        }
    } else {
        close(fd[0]);
        if(write(fd[1], winnerMinerInfo, sizeof(InfoBlock)) == -1)
        {
            close(fd[1]);
            perror("read");
            exit(EXIT_FAILURE);
        }
    }

}
