#include "principal.h"

extern int candi;

/**
 * función en la que los procesos votantes escriben su voto en el archivo de votantes
*/
void    votar() {
    sigset_t mask2;
    struct sigaction act2;
    int fd;

    act2.sa_handler = handler_sigurs2;
    act2.sa_flags = 0;
    sigemptyset(&act2.sa_mask);
    if (sigaction(SIGUSR2, &act2, NULL) < 0) { //manejo de sigusr2 para que no acabe el programa cuando la reciba
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    sigfillset(&mask2);
    sigdelset(&mask2, SIGUSR2);
    sigprocmask(SIG_SETMASK, &mask2, NULL);
    sigsuspend(&mask2); // espera de señal sigusr2

    fd = open(".votacion", O_WRONLY | O_APPEND); // abrir el fichero de votación
    if (fd < 0)
        kill(getpid(), SIGINT);
    sem_t *sem = sem_open(".votando", O_CREAT | O_RDWR, S_IRWXU, 1); // semáforo para escribir solo 1 proceso a la vez en el fichero de votación
    if (sem == SEM_FAILED)
        kill(getpid(), SIGINT);
    sem_wait(sem); // se cierra el semaforo
    srand(time(NULL) ^ getpid());
    int aleat = rand() % 2;
    if (aleat == 1)
        write(fd, "Y", 1);
    else                        // se escribe aleatoriamente Y o N
        write(fd, "N", 1);
    close(fd);
    sem_post(sem); // se abre para el siguiente proceso esperando
    sem_close(sem);
}

/**
 * función que va dirigiendo a los hijos dependiendo de si son el candidato o un votante
 * cuando llega sigusr1, se elige el candidato en la función handler_sigusr1.
 * hasta que no llega sigusr1 al principio de cada ronda, el proceso no hace su funcionamiento
*/
void    votantes(int n_proc)
{
    sigset_t mask;
    struct sigaction act;
    int fd;

    act.sa_handler = handler_sigurs1; // en esta función se elige el candidato
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGUSR1, &act, NULL) < 0) { // establece el handler en caso de que llegue sigusr1
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigprocmask(SIG_SETMASK, &mask, NULL);
    sigsuspend(&mask); // espera a que llegue sigusr1

    while (1)
    {
        // en caso de que sea candidato
        if (candi == 1) {
            pid_t pid;
            candidato(n_proc); // función que gestiona todo el funcionamiento del candidato
            usleep(250000); // espera no activa ya que ha terminado y va a empezar otra ronda
            fd = open(".pids", O_RDONLY);
            if (fd < 0)
                kill(getpid(), SIGINT);
            unlink(".candi");
            while (read(fd, &pid, sizeof(pid)) > 0) {
                if (getpid() != pid)
                    kill(pid, SIGUSR1); // va enviando sigusr1 a todos los procesos para determinar el nuevo candidato
                                        // y para avisarles de que ya está todo listo
            }
            close(fd);
            candi = 0;    // deja de ser el candidato
        }
        // en caso de que no sea candidato
        else {
            votar(); // función en la que vota
            sigsuspend(&mask); // vuelve a esperar a sigusr1
        }
    }
    exit(EXIT_SUCCESS);
}