#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>


void handler(int sig);
void handler_sigurs2(int sig);
void handler_sigurs1(int sig);
void print_votes(int n_proc, char *str);
void candidato(int n_proc);
void votar();
void votantes(int n_proc);