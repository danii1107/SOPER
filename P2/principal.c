/**
 * Autores: Marcos Alonso y Daniel Cruz
 * Práctica 2 de SOPER
*/

#include "principal.h"


/**
 * función que inicia todos los procesos necesarios, y les manda la señal sigusr1 para empezar
 * cuando esté listo
*/
int start_procs(char *argv[])
{
    int i;
    int pid;
    int *pids;
    int fd;

    struct sigaction act, actalarm;

    pids = malloc(sizeof(int) * atoi(argv[1]));
    if (!pids)
        return (EXIT_FAILURE);

    actalarm.sa_handler = handler;
    actalarm.sa_flags = 0;
    sigemptyset(&(act.sa_mask)); 
    if (sigaction(SIGALRM ,&actalarm ,NULL) < 0) // establecemos el handler en caso de que llegue sigalarm
    {
        perror(" alarm ");
        exit( EXIT_FAILURE );
    }

    alarm(atoi(argv[2]));

    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&(act.sa_mask));
    if (sigaction(SIGINT ,&act ,NULL) < 0) // establecemos el handler en caso de que llegue sigint
    {
        perror(" sigaction ");
        exit( EXIT_FAILURE );
    }

    pid = 0;
    i = 0;
    fd = open(".pids", O_CREAT | O_APPEND | O_TRUNC | O_WRONLY, S_IRWXU); // abrimos el archivo donde vamos a guardar los pids
    if (fd < 0)
    {
        free(pids);
        kill(getpid(), SIGINT);
    }

    while (i < atoi(argv[1]))
    {
        pid = fork();
        if (pid < 0)
            return (EXIT_FAILURE);
        else if (pid == 0) {
            votantes(atoi(argv[1])); // si es el hijo, va a la función votantes donde esperará la llegada de sigusr1
            exit(0);
        }
        else
        {
            waitpid(pid, NULL, WNOHANG); // wait con wnohang para no tener que esperar la finalización del hijo
            write(fd, &pid, sizeof(pid)); // escribe el pid del hijo
            pids[i] = pid;
        }
        i++;
    }
    sleep(2); // cuando acaba de crear los hijos, sleepea 2 segundos para asegurarse de que todo está en orden cuando va a enviar sigusr1
    for (i = 0; i < atoi(argv[1]); i++)
    {
        kill(pids[i], SIGUSR1); // envía sigusr1 a todos los hijos
    }
    free(pids);
    while (1); // se queda esperando a sigint o sigalarm
    close(fd);
    exit(EXIT_SUCCESS);
}

/**
 * función para indicar el uso del programa en caso de que el uso
 * sea incorrecto
*/
void    exit_use()
{
    printf("Argumenos inválidos\n");
    printf("Uso: ./voting <N_PROCS> <N_SECS>\n");
    exit(EXIT_FAILURE);
}


/**
 * punto de entrada al programa
*/
int main(int argc, char *argv[])
{
    if (argc != 3 || !argv)
        exit_use();
    start_procs(argv);
}
