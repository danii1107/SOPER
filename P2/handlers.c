#include "principal.h"

int candi = 0; // variable global que indica si el proceso es actualmente votante (0) o candidato (1)

/**
 * función que maneja sigint y sigalarm, que son los casos en los que se acaba el programa.
 * cierra todos los archivos y semáforos.
*/
void handler(int sig)
{
    int fd;
    pid_t pid;

    sig = sig;
    fd = open(".pids", O_RDONLY);
    while (read(fd, &pid, sizeof(pid)) > 0) {
        kill(pid, SIGTERM); // se envía sigterm al resto de procesos
    }
    unlink(".pids");
    sem_unlink(".can");
    sem_unlink(".votando");
    unlink(".votacion");
    unlink(".candi");
    if (sig != 14)
        printf("Finishing by signal\n");
    else
        printf("Finishing by alarm.\n");
    close(fd);
    exit(EXIT_SUCCESS);
}

/**
 * función que maneja sigusr2, no hace nada pero evita que el programa se cierre al recibirla
*/
void handler_sigurs2(int sig) {
    sig = sig;
}

/**
 * función que maneja sigusr1, basicamente elige el candidato, que es el primero en ejecutar
 * esta función manejadora
*/
void handler_sigurs1(int sig) { 
    sem_t *sem;
    int fd;

    sig = sig;
    sem = sem_open(".can", O_CREAT | O_RDWR, S_IRWXU, 1); // semáforo que gestiona la selección del candidato
    if (sem == SEM_FAILED)
        printf("error en semaforo\n");

    sem_wait(sem);

    if ((fd = open(".candi", O_RDONLY)) < 0) // el que consiga llegar hasta aquí y el archivo .candi no exista, se convierte en candidato
    {
        fd = open(".candi", O_CREAT | O_APPEND | O_TRUNC | O_WRONLY, S_IRWXU);
        candi = 1; // indica que este proceso es el candidato
        write(fd, "Se acabó", 8);
    }

    close(fd); // los demas directamente terminan
    sem_post(sem);
    sem_close(sem);
}