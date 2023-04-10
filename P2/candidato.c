#include "principal.h"

void    print_votes(int n_proc, char *str)
{
    printf("Candidate %d => [ ", getpid());

    int i=0;
    int cont = 0;

    while (i < n_proc) {
        printf("%c ", str[i]);
        if (str[i] == 'Y')
            cont++;
        i++;
    }

    free(str);

    if (cont >= n_proc/2)
        printf("] => Accepted\n");
    else
        printf("] => Rejected\n");
}

/**
 * recibe los votos de los votantes en el archivo de votación que se crea esta función
 * gestiona el inicio de los votos con sigusr2
*/
void    candidato(int n_proc) {
    int     fd, fd2;
    pid_t   pid;
    char *str = malloc(sizeof(char)*n_proc+1);
    if (!str)
        kill(getpid(), SIGINT);

    fd = open(".votacion", O_CREAT | O_APPEND | O_TRUNC | O_RDWR, S_IRWXU); // se crea el archivo de votación
    if (fd < 0)
    {
        free(str);
        kill(getpid(), SIGINT);
    }
    fd2 = open(".pids", O_RDONLY); // se abre el de los pids
    if (fd < 0)
    {
        free(str);
        kill(getpid(), SIGINT);
    }

    usleep(1000); 

    while (read(fd2, &pid, sizeof(pid)) > 0) {
        if (getpid() != pid)
            kill(pid, SIGUSR2); // se envía sigusr2 para indicar que ya pueden votar
    }

    close(fd2);

    while (read(fd, str, n_proc) != n_proc-1) { // se lee cada 1ms si ya han votado todos
        usleep(1000);
        close(fd);
        fd = open(".votacion", O_RDONLY);
        if (fd < 0)
            kill(getpid(), SIGINT);
    }

    close(fd);

    print_votes(n_proc, str); // se printean los votos
}