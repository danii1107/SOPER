#include "monitor.h"

int solutionStatus = 1;                 /* VARIABLE GLOBAL PARA COMPROBAR LA SOLUCIÓN DEL MINERO */
int i = 0;                              /* INDEXACIÓN GLOBAL */

int main(int argc, char **argv)
{
    int lag = 0;                        /* RETARDO PASADO POR PARÁMETRO */
    int j = 0;                          /* INDEXACIÓN */    
    int fd_shm_mon_compr;               /* DESCRIPTOR PARA COMPROBADOR-MONITOR */
    int fd_mon;                         /* DESCRIPTOR PARA EL MONITOR */
    CompleteShMem *info_shm = NULL;     /* EdD ALMACENA SEMÁFOROS Y MQ */
    sem_t *mutex = NULL;                /* SEMÁFORO EXCLUSIÓN MUTUA */

    /* shm_unlink( SHM_MON_COMPR );
    sem_close(mutex);
    sem_unlink( SEM_NAME_MUTEX );
    exit(EXIT_SUCCESS); */

    /* CONTROL DE ERRORES DE PARÁMETRO */
    if (argc != 2)
    {
        fprintf(stderr, "Execution format: ./monitor <LAG>\n");
        exit(EXIT_FAILURE);
    }

    /* GUARDADO DEL PARÁMETRO EN VARIABLE LOCAL */
    lag = atoi(argv[1]);
    if (lag <= 0)
    {
        fprintf(stderr, "Invalid lag\n");
        exit(EXIT_FAILURE);
    }

    /* CREACIÓN E INICIALIZACIÓN A 1 DEL SEMÁFORO MUTEX */
    if ((mutex = sem_open(SEM_NAME_MUTEX, O_CREAT | O_RDWR , S_IRWXU, 1)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sem_wait(mutex);
    /* SECCIÓN CRÍTICA */
    fd_shm_mon_compr = shm_open(SHM_MON_COMPR, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd_shm_mon_compr == -1 && errno == EEXIST)
    {
        /* MONITOR */
        sem_post(mutex);
        fflush(stdout);
        printf("MONITOR %d\n", getpid());

        if ((fd_mon = shm_open(SHM_MON_COMPR, O_RDWR , S_IRUSR | S_IWUSR)) == -1)
        {
            perror("shm_open");
            close(fd_shm_mon_compr);
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            exit(EXIT_FAILURE);
        }

        info_shm = mmap(NULL, sizeof(CompleteShMem), PROT_WRITE | PROT_READ, MAP_SHARED, fd_mon, 0);
        close(fd_mon);
        if (info_shm == MAP_FAILED)
        {
            perror("mmap");
            close(fd_shm_mon_compr);
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            exit(EXIT_FAILURE);
        }

        while (1/* NO LEA BLOQUE DE FINALIZACIÓN */)
        {
            /* CONSUMIDOR */
            sem_wait(&info_shm->sem_fill);
            sem_wait(&info_shm->sem_mutex);
            if (solutionStatus == 1) printf("Solution accepted: %08ld --> %08ld\n", info_shm->data[i].target, info_shm->data[i].solution);
            else printf("Solution rejected: %08ld !-> %08ld\n", info_shm->data[i].target, info_shm->data[i].solution);
            i--;
            usleep(lag);
            sem_post(&info_shm->sem_mutex);
            sem_post(&info_shm->sem_empty);
        }

        munmap(info_shm, sizeof(CompleteShMem));

        exit(EXIT_SUCCESS);
    }
    else if (fd_shm_mon_compr != -1)
    {
        /* COMPROBADOR */
        fflush(stdout);
        printf("COMPROBADOR %d\n", getpid());
        sem_post(mutex); /* FIN SECCIÓN CRÍTICA */

        /* REISZE DEL SEGMENTO DE MEMORIA COMPARTIDA */
        if (ftruncate(fd_shm_mon_compr, sizeof(CompleteShMem)) == -1)
        {
            perror("ftruncate");
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            exit(EXIT_FAILURE);
        }

        /* MAPEO DE LA MEMORIA COMPARTIDA */
        info_shm = mmap(NULL, sizeof(CompleteShMem), PROT_WRITE | PROT_READ, MAP_SHARED, fd_shm_mon_compr, 0);
        close(fd_shm_mon_compr); /* SE CIERRA EL DESCRIPTOR COMPROBADOR-MONITOR PORQUE YA NO NOS HACE FALTA */
        if (info_shm == MAP_FAILED)
        {
            perror("mmap");
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            exit(EXIT_FAILURE);
        }

        /* CREACIÓN E INICIALIZACIÓN DE LOS SEMÁFOROS ANÓNIMOS DE LA MEMORIA COMPARTIDA */
        if (sem_init(&info_shm->sem_mutex, 1, 1))
        {
            munmap(info_shm, sizeof(CompleteShMem));
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            exit(EXIT_FAILURE);
        }
        if (sem_init(&info_shm->sem_empty, 1, MQ_SIZE))
        {
            munmap(info_shm, sizeof(CompleteShMem));
            sem_destroy(&info_shm->sem_mutex);
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            exit(EXIT_FAILURE);
        }
        if (sem_init(&info_shm->sem_fill, 1, 0))
        {
            munmap(info_shm, sizeof(CompleteShMem));
            sem_destroy(&info_shm->sem_mutex);
            sem_destroy(&info_shm->sem_empty);
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            exit(EXIT_FAILURE);
        }

        /* INICIALIZACIÓN DEL TARGET Y SOLUCIÓN DE LA ESTRUCTURA */
        for(j = 0; j < MQ_SIZE; j++)
        {
            info_shm->data[j].target = 0;
            info_shm->data[j].solution = 0;
        }

        i = 0;
        while(1 /* != BLOQUE FINALIZACION DE LET/ESCR */)
        {
            /* PRODUCTOR */
            sem_wait(&info_shm->sem_empty);
            sem_wait(&info_shm->sem_mutex);
            info_shm->data[i].target = i;
            info_shm->data[i].solution = i+1;
            /* COMPROBAR SOLUCION, BIEN = 1, MAL = 0 */
            i++;
            usleep(lag);
            sem_post(&info_shm->sem_mutex);
            sem_post(&info_shm->sem_fill);
        }

        /* "LIBERACIÓN" DEL MAPEO DE LA MEMORIA */
        munmap(info_shm, sizeof(CompleteShMem));

        /* LIBERACIÓN DE RECURSOS UTILIZADOS 
            (SOLO HACE FALTA HACERLO UNA VEZ SI ESTE PROCESO ACABA ANTES 
            YA QUE ES MEMORIA DUPLICADA ENTRE PROCESOS) */
        shm_unlink( SHM_MON_COMPR );
        sem_close(mutex);
        sem_unlink( SEM_NAME_MUTEX );
        
        exit(EXIT_SUCCESS);
    }
    else
    {
        /* CONTROL DE ERRORES AL ABRIR EL SEGMENTO DE MEMORIA COMPARTIDA */
        sem_close(mutex);
        sem_unlink( SEM_NAME_MUTEX );
        perror("Error creating the shared memory segment\n");
        exit(EXIT_FAILURE);
    }

    /* NUNCA SE DEBERÍA LLEGAR AQUÍ PERO POR SI HAY FALLOS NO RESPALDADOS SE LIBERAN LOS RECURSOS */
    sem_close(mutex);
    sem_unlink( SEM_NAME_MUTEX );
    close(fd_shm_mon_compr);
    shm_unlink( SHM_MON_COMPR );

    exit(EXIT_FAILURE);
}
