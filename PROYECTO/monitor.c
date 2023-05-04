#include "monitor.h"

static volatile sig_atomic_t got_sigint = 0;

void handler_sigint(int sig)
{
    got_sigint = 1;
}

int main()
{
    int i = 0, j = 0;                     /* INDEXACIÓN */
    int fd_shm_mon_compr;                 /* DESCRIPTOR PARA COMPROBADOR-MONITOR */
    char solutionInfo[10] = "rejected\0"; /* */
    struct sigaction act;
    CompleteShMem *info_shm = NULL; /* EdD ALMACENA SEMÁFOROS Y COLA CIRCULAR COMPROBADOR-MONITOR */
    mqd_t mq;                       /* COLA DE MENSAJES MINERO-COMPROBADOR */
    InfoBlock recieved_info;        /* EdD PARA RECIBIR LA INFORMACIÓN ENVIADA POR MINERO A TRAVÉS DE MQ */
    pid_t pid;

    fd_shm_mon_compr = shm_open(SHM_MON_COMPR, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd_shm_mon_compr == -1)
    {
        /* CONTROL DE ERRORES AL ABRIR EL SEGMENTO DE MEMORIA COMPARTIDA */
        perror("Error creating the shared memory segment\n");
        exit(EXIT_FAILURE);
    }
    /* COMPROBADOR */

    /* REISZE DEL SEGMENTO DE MEMORIA COMPARTIDA */
    if (ftruncate(fd_shm_mon_compr, sizeof(CompleteShMem)) == -1)
    {
        perror("ftruncate");
        close(fd_shm_mon_compr);
        shm_unlink(SHM_MON_COMPR);
        exit(EXIT_FAILURE);
    }

    /* MAPEO DE LA MEMORIA COMPARTIDA DESDE COMPROBADOR */
    info_shm = mmap(NULL, sizeof(CompleteShMem), PROT_WRITE | PROT_READ, MAP_SHARED, fd_shm_mon_compr, 0);
    close(fd_shm_mon_compr); /* SE CIERRA EL DESCRIPTOR COMPROBADOR-MONITOR PORQUE YA NO NOS HACE FALTA */
    if (info_shm == MAP_FAILED)
    {
        perror("mmap");
        shm_unlink(SHM_MON_COMPR);
        exit(EXIT_FAILURE);
    }

    /* CREACIÓN E INICIALIZACIÓN DE LOS SEMÁFOROS ANÓNIMOS DE LA MEMORIA COMPARTIDA */
    if (sem_init(&info_shm->sem_mutex, 1, 1))
    {
        munmap(info_shm, sizeof(CompleteShMem));
        shm_unlink(SHM_MON_COMPR);
        exit(EXIT_FAILURE);
    }
    if (sem_init(&info_shm->sem_empty, 1, BUFF_SIZE))
    {
        sem_destroy(&info_shm->sem_mutex);
        munmap(info_shm, sizeof(CompleteShMem));
        shm_unlink(SHM_MON_COMPR);
        exit(EXIT_FAILURE);
    }
    if (sem_init(&info_shm->sem_fill, 1, 0))
    {
        sem_destroy(&info_shm->sem_mutex);
        sem_destroy(&info_shm->sem_empty);
        munmap(info_shm, sizeof(CompleteShMem));
        shm_unlink(SHM_MON_COMPR);
        exit(EXIT_FAILURE);
    }

    /* INICIALIZACIÓN DEL BLOQUE */
    for (j = 0; j < BUFF_SIZE; j++)
    {
        info_shm->data[j].ID = 0;
        info_shm->data[j].target = INITIAL_TARGET;
        info_shm->data[j].solution = 0;
        for (i = 0; i < MAX_MINER; i++)
        {
            info_shm->data[j].wallets[i].n_coins = 0;
            info_shm->data[j].wallets[i].MinersWalletPID = (pid_t)0;
        }
        info_shm->data[j].total_n_votes = 0;
        info_shm->data[j].positive_n_votes = 0;
    }

    /* INICIALIZACIÓN DE REAR Y FRONT Y FLAG DE CONTROL DE SOLUCIÓN */
    info_shm->front = info_shm->rear = 0;
    info_shm->solStatus = 1;

    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        sem_destroy(&info_shm->sem_mutex);
        sem_destroy(&info_shm->sem_empty);
        sem_destroy(&info_shm->sem_fill);
        munmap(info_shm, sizeof(CompleteShMem));
        shm_unlink(SHM_MON_COMPR);
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        /* MONITOR */
        while (info_shm->data[info_shm->rear - 1].target != END_BLOCK && info_shm->data[info_shm->rear - 1].solution != END_BLOCK /* BLOQUE DE FINALIZACIÓN */)
        {
            /* CONSUMIDOR */
            sem_wait(&info_shm->sem_fill);
            sem_wait(&info_shm->sem_mutex);

            /* MUESTRA POR PANTALLA EL ESTADO DE LA SOLUCIÓN OBTENIDA PARA SU CORRESPONDIENTE TARGET */
            if (info_shm->data[info_shm->rear - 1].target != END_BLOCK && info_shm->data[info_shm->rear - 1].solution != END_BLOCK)
            {
                if (info_shm->solStatus == 1)
                    strcpy(solutionInfo, "validated"); /* SOLUCIÓN CORRECTA */
                else
                    strcpy(solutionInfo, "rejected"); /* SOLUCIÓN INCORRECTA */

                printf("ID: %d\n", info_shm->data[info_shm->front].ID);
                printf("Winner: %ld\n", (long)info_shm->data[info_shm->front].FstMiner);
                printf("Target: %8ld\n", info_shm->data[info_shm->front].target);
                printf("Solution: %8ld (%s)\n", info_shm->data[info_shm->front].solution, solutionInfo);
                printf("Votes: %d/%d\n", info_shm->data[info_shm->front].positive_n_votes, info_shm->data[info_shm->front].total_n_votes);
                i = 0;
                while (info_shm->data[info_shm->front].wallets[i].MinersWalletPID != (pid_t)0)
                {
                    printf("Wallets: %ld:%ld\n", (long)info_shm->data[info_shm->front].wallets[i].MinersWalletPID, info_shm->data[info_shm->front].wallets[i].n_coins);
                    i++;
                }
            }

            /* "POP" COLA CIRCULAR MONITOR-COMPROBADOR */
            info_shm->front = (info_shm->front + 1) % BUFF_SIZE;

            sem_post(&info_shm->sem_mutex);
            sem_post(&info_shm->sem_empty);
        }

        munmap(info_shm, sizeof(CompleteShMem));
        shm_unlink(SHM_MON_COMPR);

        exit(EXIT_SUCCESS);
    }
    else
    {
        /* COMPROBADOR */

        act.sa_handler = handler_sigint;
        sigemptyset(&(act.sa_mask));
        act.sa_flags = 0;

        if (sigaction(SIGINT, &act, NULL) < 0)
        {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        /* APERTURA DE LA MQ MINERO-COMPROBADOR, SI YA EXISTE SE ABRE SOLO PARA LEER */
        mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attributes);
        if (mq == (mqd_t)-1)
        {
            sem_destroy(&info_shm->sem_mutex);
            sem_destroy(&info_shm->sem_empty);
            sem_destroy(&info_shm->sem_fill);
            munmap(info_shm, sizeof(CompleteShMem));
            shm_unlink(SHM_MON_COMPR);
            fprintf(stderr, "Error creating the message queue\n");
            exit(EXIT_FAILURE);
        }

        recieved_info.target = 0;
        recieved_info.solution = 0;

        while ((recieved_info.target != END_BLOCK && recieved_info.solution != END_BLOCK) && got_sigint == 0 /* BLOQUE FINALIZACION DE LECT/ESCR O RECIBE SIGINT */)
        {
            /* PRODUCTOR */
            sem_wait(&info_shm->sem_empty);
            sem_wait(&info_shm->sem_mutex);

            /* EXTRAER DE LA COLA DE MENSAJES MINERO */
            if (mq_receive(mq, (char *)&recieved_info, sizeof(InfoBlock), NULL) == -1)
            {
                sem_destroy(&info_shm->sem_mutex);
                sem_destroy(&info_shm->sem_empty);
                sem_destroy(&info_shm->sem_fill);
                munmap(info_shm, sizeof(CompleteShMem));
                shm_unlink(SHM_MON_COMPR);
                mq_close(mq);
                mq_unlink(MQ_NAME);
                fprintf(stderr, "Error recieving from mq\n");
                exit(EXIT_FAILURE);
            }

            /* METER EN MEMEORIA COMPARTIDA LA INFORMACIÓN RECIBIDA DE MINERO */
            info_shm->data[info_shm->rear].target = recieved_info.target;
            info_shm->data[info_shm->rear].solution = recieved_info.solution;

            /* COMPROBAR SOLUCIÓN; CORRECTA = 1, ERRÓNEA = 0 */
            if (info_shm->data[info_shm->rear].target == pow_hash(info_shm->data[info_shm->rear].solution))
                info_shm->solStatus = 1;
            else
                info_shm->solStatus = 0;

            if (got_sigint) /* HA RECIBIDO SIGINT */
            {
                /* ENVIAR END_BLOCK A MONITOR */
                info_shm->data[info_shm->rear].target = END_BLOCK;
                info_shm->data[info_shm->rear].solution = END_BLOCK;
            }

            /* "PUSH" COLA CIRCULAR MONITOR-COMPROBADOR */
            info_shm->rear = (info_shm->rear + 1) % BUFF_SIZE;

            sem_post(&info_shm->sem_mutex);
            sem_post(&info_shm->sem_fill);
        }

        /* LIBERAR LOS SEMÁFOROS ANÓNIMOS DE LA MEMORIA COMPARTIDA */
        sem_destroy(&info_shm->sem_mutex);
        sem_destroy(&info_shm->sem_empty);
        sem_destroy(&info_shm->sem_fill);

        /* "LIBERACIÓN" DEL MAPEO DE LA MEMORIA DESDE COMPROBADOR */
        munmap(info_shm, sizeof(CompleteShMem));

        /* LIBERACIÓN DE RECURSOS UTILIZADOS POR COMPROBADOR */
        shm_unlink(SHM_MON_COMPR);
        mq_close(mq);
        mq_unlink(MQ_NAME);

        exit(EXIT_SUCCESS);
    }
}
