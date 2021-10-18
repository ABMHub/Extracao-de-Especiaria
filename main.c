#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include "unistd.h"

#define QTD_EXTRATORES 2
#define EQUIPAMENTOS 2
#define QTD_MANUTENTORES 2
#define CAP_RECIPIENTE 2
#define STAMINA 2

sem_t equipamentos_cheios;
sem_t equipamentos_vazios;
int capacidade_atual = CAP_RECIPIENTE;

void * extrair(void *arg);
void * manutenir(void *arg);
void * recipiente(void *arg);

pthread_mutex_t lock_capacidade = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_recipiente = PTHREAD_MUTEX_INITIALIZER;

int main () {
  pthread_t extratores[QTD_EXTRATORES];
  pthread_t manutentores[QTD_MANUTENTORES];
  pthread_t viajante;
  int *id;

  sem_init(&equipamentos_cheios, 0, 0);
  sem_init(&equipamentos_vazios, 0, EQUIPAMENTOS);

  for (int i = 0; i < QTD_EXTRATORES; i++) {
    id = (int *) malloc(sizeof(int));
    *id = i;
    pthread_create(&(extratores[i]), NULL, extrair, (void *) (id));
  }
  for (int i = 0; i < QTD_MANUTENTORES; i++) {
    id = (int *) malloc(sizeof(int));
    *id = i;
    pthread_create(&(manutentores[i]), NULL, manutenir, (void *) (id));
  }

  id = (int *) malloc(sizeof(int));
  *id = 0;
  pthread_create(&viajante, NULL, recipiente, (void *) (id));

  for (int i = 0; i < QTD_EXTRATORES; i++) 
    pthread_join(extratores[i], NULL);

  for (int i = 0; i < QTD_EXTRATORES; i++) 
    pthread_join(manutentores[i], NULL);

  pthread_join(recipiente, NULL);

  return 0;  
}

void * extrair(void *arg) {
  int id = *((int *) id);
  int qtd_acoes = 0;

  while (1) {
    sem_wait(&equipamentos_vazios);
      sleep(5);
    sem_post(&equipamentos_cheios);
    qtd_acoes++;
    if (qtd_acoes == 5) {
      sleep(10);
    }
  }
}

void * manutenir(void *arg) {
  int id = *((int *) id);
  int qtd_acoes = 0;

  while (1) {
    sem_wait(&equipamentos_cheios);
      sleep(5);
      pthread_mutex_lock(&lock_capacidade);
        while (capacidade_atual == 0) {
          pthread_cond_wait(&cond_recipiente, &lock_capacidade);
        }
        capacidade_atual--;
      pthread_mutex_unlock(&lock_capacidade);
    sem_post(&equipamentos_cheios);
    qtd_acoes++;
    if (qtd_acoes == 5) {
      sleep(10);
      qtd_acoes = 0;
    }
  }
}

// void * viajante(void *arg) {

// }