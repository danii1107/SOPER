#include "types.h"

int stop = 0;
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
    int i;
    pid_t pid;
    InfoBlock* info_to_register, *winnerMinerInfo;
    sem_t* shmCreation_mutex;
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

    pipe_status = pipe(fd);
    if(pipe_status == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

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

        /* CREACIÓN E INICIALIZACIÓN A 1 DEL SEMÁFORO MUTEX */
        if ((shmCreation_mutex = sem_open(SEM_NAME_SHM_CREATION, O_CREAT | O_RDWR , S_IRWXU, 1)) == SEM_FAILED)
        {
            close(fd[1]);
            perror("sem_open");
            exit(EXIT_FAILURE);
        }

        sem_wait(shmCreation_mutex);
        fd_shm_miners = shm_open(SHM_NAME_MINERS, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
        if (fd_shm_miners == -1 && errno == EEXIST)
        {
            fd_shm_miners = shm_open( SHM_NAME_MINERS, O_RDWR, S_IRUSR | S_IWUSR);
            if (fd_shm_miners == -1)
            {
                close(fd[1]);
                perror("shm_open");
                exit(EXIT_FAILURE);
            }

            sys_info = mmap(NULL, sizeof(SystemMemory), PROT_WRITE | PROT_READ, MAP_SHARED, fd_shm_miners, 0);
            close(fd_shm_miners); /* SE CIERRA EL DESCRIPTOR PORQUE YA NO NOS HACE FALTA */
            if (sys_info == MAP_FAILED)
            {
                perror("mmap");
                close(fd[1]);
                shm_unlink( SHM_NAME_MINERS );
                sem_close(shmCreation_mutex);
                sem_unlink( SEM_NAME_SHM_CREATION );
                exit(EXIT_FAILURE);
            }

            sem_post(shmCreation_mutex);
        } 
        else if (fd_shm_miners != -1)
        {
            /* REISZE DEL SEGMENTO DE MEMORIA COMPARTIDA */
            if (ftruncate(fd_shm_miners, sizeof(SystemMemory)) == -1)
            {
                perror("ftruncate");
                close(fd[1]);
                shm_unlink( SHM_NAME_MINERS );
                sem_close(shmCreation_mutex);
                sem_unlink( SEM_NAME_SHM_CREATION );
                exit(EXIT_FAILURE);
            }

            /* MAPEO DE LA MEMORIA COMPARTIDA DESDE MINERO */
            sys_info = mmap(NULL, sizeof(SystemMemory), PROT_WRITE | PROT_READ, MAP_SHARED, fd_shm_miners, 0);
            close(fd_shm_miners); /* SE CIERRA EL DESCRIPTOR PORQUE YA NO NOS HACE FALTA */
            if (sys_info == MAP_FAILED)
            {
                perror("mmap");
                close(fd[1]);
                shm_unlink( SHM_NAME_MINERS );
                sem_close(shmCreation_mutex);
                sem_unlink( SEM_NAME_SHM_CREATION );
                exit(EXIT_FAILURE);
            }

            sys_info->minerCount = 0;
            for(i = 0; i < MAX_MINER; i++)
            {
                sys_info->MinersPIDS[i] = (pid_t) 0;
                sys_info->wallets[i].n_coins = 0;
                sys_info->wallets[i].MinersWalletPID = (pid_t) 0;
                if (i != MAX_MINER - 1) sys_info->votes[i] = 0;
                sys_info->last_infoBlock->ID = 0;
                sys_info->last_infoBlock->target = INITIAL_TARGET;
                sys_info->last_infoBlock->solution = 0;
                sys_info->last_infoBlock->wallets[i].n_coins = 0;
                sys_info->last_infoBlock->wallets[i].MinersWalletPID = (pid_t) 0;
                sys_info->last_infoBlock->total_n_votes = 0;
                sys_info->last_infoBlock->positive_n_votes = 0;
                sys_info->current_infoBlock->ID = 0;
                sys_info->current_infoBlock->target = INITIAL_TARGET;
                sys_info->current_infoBlock->solution = 0;
                sys_info->current_infoBlock->wallets[i].n_coins = 0;
                sys_info->current_infoBlock->wallets[i].MinersWalletPID = (pid_t) 0;
                sys_info->current_infoBlock->total_n_votes = 0;
                sys_info->current_infoBlock->positive_n_votes = 0;
            }

            sem_post(shmCreation_mutex);
        }
        else 
        {
            sem_close(shmCreation_mutex);
            close(fd[0]);
            close(fd[1]);
            exit(EXIT_FAILURE);
        }
    }

}
