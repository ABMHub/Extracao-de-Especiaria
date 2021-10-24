#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "unistd.h"

// definicao dos parametros
#define QTD_EXTRATORES 4    // qtd de pessoas extraindo a especiaria
#define EQUIPAMENTOS 6      // qtd de equipamentos disponiveis
#define QTD_MANUTENTORES 3  // qtd de pessoas colocando a especiaria no recipiente final
#define CAP_RECIPIENTE 20    // capacidade maxima do recipiente
#define STAMINA 3           // qtd de acoes que uma pessoa toma, antes de descansar

// definicao dos semaforos de equipamento
sem_t equipamentos_cheios;
sem_t equipamentos_vazios;

// definicao do lock e das variaveis de condicao do recipiente
pthread_cond_t cond_viajante = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_recipiente = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_capacidade = PTHREAD_MUTEX_INITIALIZER;

// definicao da capacidade inicial do recipiente
int capacidade_atual = CAP_RECIPIENTE;

void * extrair(void *arg);
void * manutenir(void *arg);
void * recipiente(void *arg);

int main () {
  srand(time(NULL));                          // melhora a randomizacao
  pthread_t extratores[QTD_EXTRATORES];                
  pthread_t manutentores[QTD_MANUTENTORES];            
  pthread_t viajante;                                 
  int *id;                                            

  // comeca em 0, para que nao haja nenhum equipamento cheio antes do trabalho comecar
  sem_init(&equipamentos_cheios, 0, 0);              

  // o numero de equipamentos vazios eh o numero de equipamentos disponiveis, pois ninguem trabalhou ainda
  sem_init(&equipamentos_vazios, 0, EQUIPAMENTOS);    

  // Secao do codigo para criacao de threads
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

  // Secao do codigo para join nas threads
  // Nao eh necessario join em todas, ja que nenhuma acaba
  for (int i = 0; i < QTD_EXTRATORES; i++) 
    pthread_join(extratores[i], NULL);

  for (int i = 0; i < QTD_EXTRATORES; i++) 
    pthread_join(manutentores[i], NULL);

  pthread_join(viajante, NULL);

  return 0;  
}

// Pega equipamentos e extrai a especiaria.
// Nao faz nada enquanto nao houver equipamentos disponiveis
void * extrair(void *arg) {
  int id = *((int *) arg);
  int qtd_acoes = 0;                  // zera qtd de acoes tomadas

  while (1) {
    sem_wait(&equipamentos_vazios);   // pega um equipamento vazio
      printf("Extrator %d esta extraindo especiaria\n", id);
      sleep(rand()%5);                       // simula trabalho
    sem_post(&equipamentos_cheios);   // devolve um equipamento cheio

    printf("Extrator %d terminou de extrair especiaria\n", id);
    qtd_acoes++;                      // aumenta contador de acoes
    if (qtd_acoes == STAMINA) {       // se o funcionario estiver cansado
      printf("Extrator %d esta descansando\n", id);
      sleep((rand()%3)+3);                      // descansa
      qtd_acoes = 0;                  // zera qtd de acoes tomadas
    }
  }
}

// Espera equipamentos cheios de especiaria aparecerem
// Esvazia os equipamentos e devolve eles para os extratores
// Espera o recipiente esvaziar, caso esteja cheio
void * manutenir(void *arg) {
  int id = *((int *) arg);
  int qtd_acoes = 0;                              // zera qtd de acoes tomadas

  while (1) {
    sem_wait(&equipamentos_cheios);               // pega um equipamento cheio
      printf("Manutenor %d vai esvaziar um equipamento no recipiente\n", id);
      pthread_mutex_lock(&lock_capacidade);       // pega lock de exclusao mutua do recipiente
        while (capacidade_atual == 0) {           // se o recipiente estiver cheio :
          printf("O recipiente esta cheio, manutenor %d vai dormir\n", id);
          pthread_cond_signal(&cond_viajante);    // acorda o viajante para que ele esvazie o recipiente
          // dorme 
          pthread_cond_wait(&cond_recipiente, &lock_capacidade);
        }
        sleep(rand()%3);                                 // simula 'esvaziamento'
        capacidade_atual--;                       // caso o recipiente tenha capacidade disponivel, enche ele
      printf("Manutenor %d terminou de esvaziar um equipamento. Capacidade atual = %d\n", id, capacidade_atual);
      pthread_mutex_unlock(&lock_capacidade);     // destrava lock do recipiente
    sem_post(&equipamentos_vazios);               // devolve um equipamento vazio
    qtd_acoes++;                                  // aumenta contador de acoes
    if (qtd_acoes == STAMINA) {                   // se o funcionario estiver cansado
      printf("Manutenor %d esta descansando\n", id);
      sleep((rand()%3)+3);                                  // descansa
      qtd_acoes = 0;                              // zera qtd de acoes tomadas
    }
  }
}

// Chega de tempos em tempos para recolher o recipiente cheio
// Caso ele chegue antes do recipiente encher, ele espera dormindo
// Quando estiver cheio, ele recolhe e devolve um novo, depois de um tempo, e vai embora novamente
void * recipiente(void *arg) {
  int id = *((int *) arg);
  while (1) {
    sleep(20);                                    // o tempo que demora pro viajante chegar
    printf("O viajante chegou!\n");               // 
    pthread_mutex_lock(&lock_capacidade);         // trava o lock do recipiente
    while (capacidade_atual != 0) {               // enquanto o recipiente nao estiver cheio:
      printf("O recipiente nao esta lacrado. O viajante vai dormir\n");
      // dorme enquanto nao estiver cheio
      pthread_cond_wait(&cond_viajante, &lock_capacidade);
    }
    pthread_mutex_unlock(&lock_capacidade);       // destrava lock do recipiente

    printf("O viajante levou o antigo recipiente embora\n");
    sleep((rand()%5) + 5);                                    // simula tempo entre pegar o recipiente cheio e devolver um vazio

    pthread_mutex_lock(&lock_capacidade);         // trava lock do recipiente
    capacidade_atual = CAP_RECIPIENTE;            // capacidade atual vira capacidade total (esta vazio)
    pthread_cond_broadcast(&cond_recipiente);     // acorda todos os manutenores dormindo
    pthread_mutex_unlock(&lock_capacidade);       // destrava lock do recipiente
    printf("O viajante trouxe um novo recipiente e foi embora\n");
  }  
}