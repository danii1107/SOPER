#include "monitor.h"

int main(int argc, char **argv)
{
    int lag = 0;                        /* RETARDO PASADO POR PARÁMETRO */
    int j = 0;                          /* INDEXACIÓN */    
    int fd_shm_mon_compr;               /* DESCRIPTOR PARA COMPROBADOR-MONITOR */
    int fd_mon;                         /* DESCRIPTOR PARA EL MONITOR */
    CompleteShMem *info_shm = NULL;     /* EdD ALMACENA SEMÁFOROS Y COLA CIRCULAR COMPROBADOR-MONITOR */
    ShmData recieved_info;              /* EdD PARA RECIBIR LA INFORMACIÓN ENVIADA POR MINERO A TRAVÉS DE MQ */
    mqd_t mq;                           /* COLA DE MENSAJES MINERO-COMPROBADOR */
    sem_t *mutex = NULL;                /* SEMÁFORO EXCLUSIÓN MUTUA */

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
    if (fd_shm_mon_compr == -1 && errno == EEXIST) /* SHM YA EXISTE */
    {
        /* MONITOR */
        sem_post(mutex);
        fflush(stdout);
        printf("[%d] Printing blocks...\n", getpid());

        /* AÈRTURA DEL SEGMENTO DE MEMORIA COMPARTIDA DESDE MONITOR */
        if ((fd_mon = shm_open(SHM_MON_COMPR, O_RDWR , S_IRUSR | S_IWUSR)) == -1)
        {
            perror("shm_open");
            close(fd_shm_mon_compr);
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            exit(EXIT_FAILURE);
        }

        /* MAPEO DE LA MEMORIA COMPARTIDA DESDE MONITOR */
        info_shm = mmap(NULL, sizeof(CompleteShMem), PROT_WRITE | PROT_READ, MAP_SHARED, fd_mon, 0);
        close(fd_mon); /* SE CIERRA EL DESCRIPTOR MONITOR PORQUE YA NO NOS HACE FALTA */
        if (info_shm == MAP_FAILED)
        {
            perror("mmap");
            close(fd_shm_mon_compr);
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            exit(EXIT_FAILURE);
        }

        while (info_shm->data[info_shm->rear-1].target != -1 && info_shm->data[info_shm->rear-1].solution != -1 /* BLOQUE DE FINALIZACIÓN */)
        {
            /* CONSUMIDOR */
            sem_wait(&info_shm->sem_fill);
            sem_wait(&info_shm->sem_mutex);

            /* MUESTRA POR PANTALLA EL ESTADO DE LA SOLUCIÓN OBTENIDA PARA SU CORRESPONDIENTE TARGET */
            if (info_shm->data[info_shm->rear-1].target != -1 && info_shm->data[info_shm->rear-1].solution != -1)
            {
                if (info_shm->solutionStatus == 1) printf("Solution accepted: %08ld --> %08ld\n", 
                    info_shm->data[info_shm->front].target, info_shm->data[info_shm->front].solution); /* SOLUCIÓN CORRECTA */
                else printf("Solution rejected: %08ld !-> %08ld\n", info_shm->data[info_shm->front].target, 
                    info_shm->data[info_shm->front].solution); /* SOLUCIÓN INCORRECTA */
            }
            
            /* "POP" COLA CIRCULAR MONITOR-COMPROBADOR */
            info_shm->front = (info_shm->front + 1) % BUFF_SIZE;
            
            sem_post(&info_shm->sem_mutex);
            sem_post(&info_shm->sem_empty);
            
            /* ESPERA ACTIVA DE <LAG> MILISEGUNDOS */
            usleep(lag*1000);
        }

        /* "LIBERACIÓN" DEL MAPEO DE LA MEMORIA DESDE MONITOR */
        munmap(info_shm, sizeof(CompleteShMem));

        fflush(stdout);
        printf("[%d] Finishing\n", getpid());
        
        close(fd_shm_mon_compr);
        shm_unlink( SHM_MON_COMPR );
        sem_close(mutex);
        sem_unlink( SEM_NAME_MUTEX );

        exit(EXIT_SUCCESS);
    }
    else if (fd_shm_mon_compr != -1)
    {
        /* COMPROBADOR */
        fflush(stdout);
        printf("[%d] Checking blocks...\n", getpid());
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

        /* MAPEO DE LA MEMORIA COMPARTIDA DESDE COMPROBADOR */
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
        if (sem_init(&info_shm->sem_empty, 1, BUFF_SIZE))
        {
            sem_destroy(&info_shm->sem_mutex);
            munmap(info_shm, sizeof(CompleteShMem));
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            exit(EXIT_FAILURE);
        }
        if (sem_init(&info_shm->sem_fill, 1, 0))
        {
            sem_destroy(&info_shm->sem_mutex);
            sem_destroy(&info_shm->sem_empty);
            munmap(info_shm, sizeof(CompleteShMem));
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            exit(EXIT_FAILURE);
        }

        /* INICIALIZACIÓN DEL TARGET Y SOLUCIÓN DE LA ESTRUCTURA */
        for(j = 0; j < BUFF_SIZE; j++)
        {
            info_shm->data[j].target = 0;
            info_shm->data[j].solution = 0;
        }
        /* INICIALIZACIÓN DE LA FLAG DE CONTROL DE SOLUCIÓN,
            REAR Y FRONT */
        info_shm->solutionStatus = 1;
        info_shm->front = info_shm->rear = 0;

        /* APERTURA DE LA MQ MINERO-COMPROBADOR, SI YA EXISTE SE ABRE SOLO PARA LEER */
        mq = mq_open( MQ_NAME , O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attributes);
        if (mq == (mqd_t) - 1)
        {
            sem_destroy(&info_shm->sem_mutex);
            sem_destroy(&info_shm->sem_empty);
            sem_destroy(&info_shm->sem_fill);
            munmap(info_shm, sizeof(CompleteShMem));
            shm_unlink( SHM_MON_COMPR );
            sem_close(mutex);
            sem_unlink( SEM_NAME_MUTEX );
            fprintf(stderr, "Error creating the message queue\n");
            exit(EXIT_FAILURE);
        }
        
        /* INICIALIZACIÓN DE LA EdD ShmData ANTES DEL SALTO CONDICIONAL */
        recieved_info.target = 0;
        recieved_info.solution = 0;
        
        while(recieved_info.target != -1 && recieved_info.solution != -1 /* BLOQUE FINALIZACION DE LECT/ESCR */)
        {
            /* PRODUCTOR */
            sem_wait(&info_shm->sem_empty);
            sem_wait(&info_shm->sem_mutex);
            
            /* EXTRAER DE LA COLA DE MENSAJES MINERO */
            if (mq_receive(mq, (char *) &recieved_info, sizeof(ShmData), NULL) == -1)
            {
                sem_destroy(&info_shm->sem_mutex);
                sem_destroy(&info_shm->sem_empty);
                sem_destroy(&info_shm->sem_fill);
                munmap(info_shm, sizeof(CompleteShMem));
                shm_unlink( SHM_MON_COMPR );
                sem_close(mutex);
                sem_unlink( SEM_NAME_MUTEX );
                mq_close(mq);
                mq_unlink( MQ_NAME );
                fprintf(stderr, "Error recieving from mq\n");
                exit(EXIT_FAILURE);
            }

            /* METER EN MEMEORIA COMPARTIDA LA INFORMACIÓN RECIBIDA DE MINERO */
            info_shm->data[info_shm->rear].target = recieved_info.target;
            info_shm->data[info_shm->rear].solution = recieved_info.solution;
            
            /* COMPROBAR SOLUCIÓN; CORRECTA = 1, ERRÓNEA = 0 */
            if (info_shm->data[info_shm->rear].target == pow_hash(info_shm->data[info_shm->rear].solution)) info_shm->solutionStatus = 1;
            else info_shm->solutionStatus = 0;
            
            /* "PUSH" COLA CIRCULAR MONITOR-COMPROBADOR */
            info_shm->rear = (info_shm->rear + 1) % BUFF_SIZE;

            sem_post(&info_shm->sem_mutex);
            sem_post(&info_shm->sem_fill);
            
            /* ESPERA ACTIVA DE <LAG> MILISEGUNDOS */
            usleep(lag*1000);
        }

        /* LIBERAR LOS SEMÁFOROS ANÓNIMOS DE LA MEMORIA COMPARTIDA */
        sem_destroy(&info_shm->sem_mutex);
        sem_destroy(&info_shm->sem_empty);
        sem_destroy(&info_shm->sem_fill);

        /* "LIBERACIÓN" DEL MAPEO DE LA MEMORIA DESDE COMPROBADOR */
        munmap(info_shm, sizeof(CompleteShMem));

        /* LIBERACIÓN DE RECURSOS UTILIZADOS POR COMPROBADOR */
        shm_unlink( SHM_MON_COMPR );
        sem_close(mutex);
        sem_unlink( SEM_NAME_MUTEX );
        mq_close(mq);
        mq_unlink( MQ_NAME );

        fflush(stdout);
        printf("[%d] Finishing\n", getpid());
        
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
    sem_destroy(&info_shm->sem_mutex);
    sem_destroy(&info_shm->sem_empty);
    sem_destroy(&info_shm->sem_fill);
    munmap(info_shm, sizeof(CompleteShMem));
    sem_close(mutex);
    sem_unlink( SEM_NAME_MUTEX );
    close(fd_shm_mon_compr);
    shm_unlink( SHM_MON_COMPR );
    mq_close(mq);
    mq_unlink( MQ_NAME );

    exit(EXIT_FAILURE);
}
