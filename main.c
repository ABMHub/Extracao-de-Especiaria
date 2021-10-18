#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include "unistd.h"

#define QTD_EXTRATORES 3
#define EQUIPAMENTOS 4
#define QTD_MANUTENTORES 3
#define CAP_RECIPIENTE 5
#define STAMINA 3

sem_t equipamentos_cheios;
sem_t equipamentos_vazios;
int capacidade_atual = CAP_RECIPIENTE;

void * extrair(void *arg);
void * manutenir(void *arg);
void * recipiente(void *arg);

pthread_mutex_t lock_capacidade = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_recipiente = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_viajante = PTHREAD_MUTEX_INITIALIZER;

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

  pthread_join(viajante, NULL);

  return 0;  
}

void * extrair(void *arg) {
  int id = *((int *) arg);
  int qtd_acoes = 0;

  while (1) {
    sem_wait(&equipamentos_vazios);
      printf("Extrator %d esta extraindo especiaria\n", id);
      sleep(5);
    sem_post(&equipamentos_cheios);
    printf("Extrator %d terminou de extrair especiaria\n", id);
    qtd_acoes++;
    if (qtd_acoes == 5) {
      printf("Extrator %d esta descansando\n", id);
      sleep(10);
      qtd_acoes = 0;
    }
  }
}

void * manutenir(void *arg) {
  int id = *((int *) arg);
  int qtd_acoes = 0;

  while (1) {
    sem_wait(&equipamentos_cheios);
      printf("Manutenor %d vai esvaziar um equipamento no recipiente\n", id);
      sleep(5);
      pthread_mutex_lock(&lock_capacidade);
        while (capacidade_atual == 0) {
          printf("O recipiente esta cheio, manutenor %d vai dormir\n", id);
          pthread_cond_signal(&cond_viajante);
          pthread_cond_wait(&cond_recipiente, &lock_capacidade);
        }
        capacidade_atual--;
      printf("Manutenor %d terminou de esvaziar um equipamento. Capacidade atual = %d\n", id, capacidade_atual);
      pthread_mutex_unlock(&lock_capacidade);
    sem_post(&equipamentos_vazios);
    qtd_acoes++;
    if (qtd_acoes == 5) {
      printf("Manutenor %d esta descansando\n", id);
      sleep(10);
      qtd_acoes = 0;
    }
  }
}

void * recipiente(void *arg) {
  int id = *((int *) arg);
  while (1) {
    sleep(20);
    printf("O viajante chegou!\n");
    pthread_mutex_lock(&lock_capacidade);
    while (capacidade_atual != 0) {
      printf("O recipiente nao esta lacrado. O viajante vai dormir\n");
      pthread_cond_wait(&cond_viajante, &lock_capacidade);
    }
    pthread_mutex_unlock(&lock_capacidade);

    printf("O viajante levou o antigo recipiente embora\n");
    sleep(30);

    pthread_mutex_lock(&lock_capacidade);
    capacidade_atual = CAP_RECIPIENTE;    
    pthread_cond_broadcast(&cond_recipiente);
    pthread_mutex_unlock(&lock_capacidade);
    printf("O viajante trouxe um novo recipiente e foi embora\n");
  }  
}