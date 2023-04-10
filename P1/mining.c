#include "pow.h"
#include "mining.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


int stop = 0;
long res = 0;

/**
 * @brief Busca la solución del target y la guarda en
 *        la variable global res
 *
 * @param data estructura de datos del programa
 * @return NULL
 */
void* solve(void* data)
{
    if (!data) return NULL;

    Data* d = data;
    long i = 0;

    i = d->pow_ini;
    while(i < d->pow_fin && stop == 0)
    {
        if(d->target == pow_hash(i)) 
        {
            res = i;
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
 * @param obj objetivo del proceso
 * @return 0 si funciona correctamente o 1 si algo da error
 */
int minero(int n_threads, long obj)
{
    Data data[n_threads];
    int i;
    int error;
    pthread_t* threads;

    stop = 0;

    if (n_threads <= 0 || obj < 0 || obj > POW_LIMIT || n_threads > MAX_THREADS) return -1;

    threads = (pthread_t*) malloc(sizeof(pthread_t) * n_threads);
    if(!threads){
        return -1;
    }

    i = 0;
    while(i < n_threads)
    {
        data[i].target = obj;
        data[i].pow_ini = i* POW_LIMIT/n_threads;
        data[i].pow_fin = (i+1)* POW_LIMIT/n_threads;

        error = pthread_create(&threads[i], NULL, solve, &data[i]);
        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            free(threads);
            return -1;
        }
        i++;
    }
    i = 0;
    while(i < n_threads)
    {
        error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            free(threads);
            return -1;
        }
        i++;
    }

    free(threads);
    return 0;
}

int main(int argc, char **argv)
{
    long tres, uno, dos = 0;
    long obj;
    long buf[2];
    int fd[2][2]; 
    int i, k;
    int status, pipe_status;
    int mon_ret = 0;
    pid_t pid;

    if(argc != 4)
        exit(EXIT_FAILURE);

    uno = atol(argv[1]);
    dos = atol(argv[2]);
    tres = atol(argv[3]);

    if(uno<0 || uno > POW_LIMIT || dos<0 || tres<0 || tres > MAX_THREADS)
        exit(EXIT_FAILURE);

    obj = uno;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        pipe_status = pipe(fd[0]);
        if (pipe_status < 0)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pipe_status = pipe(fd[1]);
        if (pipe_status < 0)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            close(fd[1][0]);
            close(fd[0][1]);
            k = 0;
            while (k < dos && mon_ret == 0)
            {   
                if(read(fd[0][0], &buf, sizeof(buf)) == -1)
                {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                mon_ret = monitor(buf);
                if (write(fd[1][1], &mon_ret, sizeof(int)) == -1)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
                k++;
            }
            close(fd[0][0]);
            close(fd[1][1]);
        } else {
            close(fd[0][0]);
            close(fd[1][1]);
            i = 0;
            while(i < dos && mon_ret == 0)
            {
                if(minero(tres, obj) != 0)
                {
                    exit(EXIT_FAILURE);
                }
                buf[0] = obj;
                buf[1] = res;
                obj = res;
                if (write(fd[0][1], &buf, sizeof(buf)) == -1)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
                if(read(fd[1][0], &mon_ret, sizeof(int)) == -1)
                {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                if (mon_ret){
                    printf("The solution has been invalidated\n");
                    wait(&status);
                    if(WIFEXITED(status))
                    {
                        fprintf(stdout, "Monitor exited with status: %d\n", WEXITSTATUS(status));
                    }
                    else
                    {
                        fprintf(stdout, "Monitor exited unexpectedly\n");
                        exit(EXIT_FAILURE);
                    }
                    exit(EXIT_FAILURE);
                } 
                i++;
            }
            
            close(fd[1][0]);
            close(fd[0][1]);
            wait(&status);
            if(WIFEXITED(status))
            {
                fprintf(stdout, "Monitor exited with status: %d\n", WEXITSTATUS(status));
                exit(EXIT_SUCCESS);
            }
            else
            {
                fprintf(stdout, "Monitor exited unexpectedly\n");
                exit(EXIT_FAILURE);
            }
            
        }
    } else {
        wait(&status);
        if(WIFEXITED(status))
        {
            fprintf(stdout, "Miner exited with status: %d\n", WEXITSTATUS(status));
            exit(EXIT_SUCCESS);
        }
        else
        {
            fprintf(stdout, "Miner exited unexpectedly\n");
            exit(EXIT_FAILURE);
        }
    }
        
    exit(EXIT_SUCCESS);
}

/**
 * @brief Comprueba si el resultado recibido del minero es correcto
 *        y escribe el target y su solución
 *
 * @param buf target y solución del objetivo
 * @return 0 si la solución es aceptada o 1 si la solución es denegada
 */
int monitor(long buf[2])
{
    if(!buf) return 1;

    if (pow_hash(buf[1]) == buf[0] && buf[0] != buf[1])
    {
        printf("Solution accepted: %08ld --> %08ld\n", buf[0], buf[1]);
        return 0;
    } else {
        printf("Solution rejected: %08ld !-> %08ld\n", buf[0], buf[1]);
        if (buf[0] == buf[1]) printf("Too many threads\n");
        return 1;
    }

    return 1;
}

