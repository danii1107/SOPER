#include "minero.h"

int stop = 0;         /* FLAG GLOBAL PARA DETENER LOS HILOS */
long MinerResult = 0; /* VARIABLE GLOBAL PARA GUARDAR EL RESULTADO */

/**
 * @brief Busca la solución del target y la guarda en
 *        la variable global MinerResult
 *
 * @param data estructura de datos del programa
 * @return NULL
 */
void* solve(void* data)
{
    if (!data) return NULL;

    MinData* d = data;  /* EdD ALMACENA EL INTERVALO Y EL TARGET DE LA BÚSQUEDA */
    long i = 0;         /*INDEXACIÓN */

    for(i = d->pow_ini; i <= d->pow_fin && stop == 0; i++)
    {
        if(d->target == pow_hash(i)) /* SI EL TARGET ES IGUAL AL RESULTADO DE LA FUNCIÓN HASH */
        {
            MinerResult = i; /* GUARDAMOS EL RESULTADO EN UNA VARIABLE GLOBAL */
            stop = 1; /* PARAMOS TODOS LOS HILOS */
        }
    }

    return NULL;
}

/**
 * @brief crea los hilos y les adjudica su intervalo de búsqueda
 *
 * @param target objetivo del proceso
 * @return 0 si funciona correctamente o 1 si algo da error
 */
int minero(long target)
{
    int i;                          /* INDEXACIÓN */
    int error;                      /* CONTROL E INFORME DE ERRORES */
    MinData data[NUM_THREADS];      /* EdD PARA ALMACENAR LOS INTERVALOS Y EL TARGET DE LA BÚSQUEDA PARA CADA HILO */
    pthread_t threads[NUM_THREADS]; /* ARRAY DE HILOS */

    stop = 0; /* FLAG GLOBAL A 0 PARA PERMITIR LA NUEVA RONDA */

    if (target < 0 || target > POW_LIMIT) return EXIT_FAILURE;

    /* CREACIÓN DE LOS INTERVALOS DE BÚSQUEDA PARA CADA HILO */
    for (i = 0; i < NUM_THREADS; i++)
    {
        data[i].pow_ini = i * POW_LIMIT/NUM_THREADS;
        data[i].pow_fin = (i+1) * POW_LIMIT/NUM_THREADS;
        data[i].target = target;
        if (data[i].pow_ini > 0) (data[i].pow_ini)++;
    }

    /* LANZAMIENTO DE LOS HILOS */
    for(i = 0; i < NUM_THREADS; i++)
    {
        error = pthread_create(&threads[i], NULL, solve, &data[i]);
        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            return EXIT_FAILURE;
        }
    }
    for(i = 0; i < NUM_THREADS; i++)
    {
        error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    int rounds, lag = 0; /* PARÁMETROS */
    int i = 0;           /* INDEXACIÓN */
    mqd_t mq;            /* COLA DE MENSAJES MINERO-COMPROBADOR */
    ShmData info;        /* EdD QUE ALMACENA EL TARGET Y LA SOLUCIÓN */

    /* CONTROL DE ERRORES DE PARÁMETRO */
    if (argc != 3)
    {
        fprintf(stderr, "Execution format: ./minero <ROUNDS> <LAG>\n");
        exit(EXIT_FAILURE);
    }

    /* GUARDADO DE LOS PARÁMETROS EN VARIABLES LOCALES */
    rounds = atoi(argv[1]);
    if (rounds <= 0)
    {
        fprintf(stderr, "Invalid rounds\n");
        exit(EXIT_FAILURE);
    }
    lag = atoi(argv[2]);
    if (lag <= 0)
    {
        fprintf(stderr, "Invalid lag\n");
        exit(EXIT_FAILURE);
    }

    /* APERTURA DE LA MQ MINERO-COMPROBADOR, SI YA EXISTE SE ABRE SOLO PARA ESCRIBIR */
    mq = mq_open( MQ_NAME , O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attributes);
    if(mq == (mqd_t) - 1)
    {
        fprintf(stderr, "Error creating the message queue\n");
        exit(EXIT_FAILURE);
    }

    printf("[%d] Generating blocks...\n", getpid());

    /* COMIENZO DEL MINADO A PARTIR DEL TARGET INICIAL */
    info.target = INITIAL_TARGET;
    for (i = 0; i < rounds; i++)
    {
        if (minero(info.target)) /* SI MINERO DEVUELVE ALGO != 0 (EXIT_FAILURE) */
        {
            /* ENVIAR BLOQUE DE FINALIZACION, POR ERROR EN EL MINERO */
            info.target = END_BLOCK;
            info.solution = END_BLOCK;
            if (mq_send(mq, (char *) &info, sizeof(info), 1) == -1)
            {
                mq_close(mq);
                mq_unlink( MQ_NAME );
                fprintf(stderr, "Error sending into message queue\n");
                exit(EXIT_FAILURE);
            }
            /* LIBERACIÓN DE RECURSOS */
            mq_close(mq);
            mq_unlink( MQ_NAME );
            exit(EXIT_FAILURE);
        }
        
        info.solution = MinerResult; /* RESULTADO DE LA RONDA */
        
        /* GUARDAR EL TARGET/SOLUCIÓN EN LA MQ PARA QUE COMRPOBADOR LOS RECIBA */
        if (mq_send(mq, (char *) &info, sizeof(ShmData), 1) == -1)
        {
            mq_close(mq);
            mq_unlink( MQ_NAME );
            fprintf(stderr, "Error sending into message queue\n");
            exit(EXIT_FAILURE);
        }
        
        info.target = MinerResult; /* REASIGNACIÓN DEL TARGET A LA SOLUCIÓN ANTERIOR PARA LA PRÓXIMA RONDA */

        /* ESPERA ACTIVA DE <LAG> MILISEGUNDOS */
        usleep(lag*1000);
    }

    /* ENVIAR BLOQUE DE FINALIZACIÓN */
    info.target = END_BLOCK; /* -1 */
    info.solution = END_BLOCK; /* -1 */
    if (mq_send(mq, (char *) &info, sizeof(info), 1) == -1)
    {
        mq_close(mq);
        mq_unlink( MQ_NAME );
        fprintf(stderr, "Error sending into message queue\n");
        exit(EXIT_FAILURE);
    }

    printf("[%d] Finishing\n", getpid());
    
    /* LIBERAR RECURSOS Y TERMINAR */
    mq_close(mq);
    mq_unlink( MQ_NAME );
    exit(EXIT_SUCCESS);
}
