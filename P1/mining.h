#ifndef MINING_H
#define MINING_H

#define NUM_PROC 3 
#define MAX_THREADS 100

typedef struct _Data{
    long pow_ini; /*!< Número inicial del intervalo */
    long pow_fin; /*!< Número final del intervalo */
    long target; /*!< Objetivo a encontrar */
}Data;

/**
 * @brief Busca la solución del target y la guarda en
 *        la variable global res
 *
 * @param data estructura de datos del programa
 * @return NULL
 */
int minero(int n_threads, long obj);

/**
 * @brief crea los hilos y les adjudica su intervalo de búsqueda
 *
 * @param n_threads números de hilos
 * @param obj objetivo del proceso
 * @return 0 si funciona correctamente o 1 si algo da error
 */
void* solve(void* data);

/**
 * @brief Comprueba si el resultado recibido del minero es correcto
 *        y escribe el target y su solución
 *
 * @param buf target y solución del objetivo
 * @return 0 si la solución es aceptada o 1 si la solución es denegada
 */
int monitor(long buf[2]);

#endif 