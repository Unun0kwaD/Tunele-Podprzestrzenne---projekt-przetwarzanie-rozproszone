#ifndef UTILH
#define UTILH
#include "main.h"

/* typ pakietu */
typedef struct {
    int ts;       /* timestamp (zegar lamporta */
    int src;  

    int rozmiar_grupy;
    int typ_grupy;     /* przykładowe pole z danymi; można zmienić nazwę na bardziej pasującą */
} packet_t;

typedef struct {
    int front;
    int rear;
    int size;
    int perons_inside;
    int *ids;
    int *types;
    int *sizes;
    int *ts;
    int *rank;

} queue_t;

/* packet_t ma trzy pola, więc NITEMS=3. Wykorzystane w inicjuj_typ_pakietu */
#define NITEMS 4

/* Typy wiadomości */
/* TYPY PAKIETÓW */
#define ACK     1
#define REQUEST 2
#define RELEASE 3
#define APP_PKT 4
#define FINISH  5
#define INSECTION 6

#define KURIER 999
#define WYCIECZKA 777
#define EMPTY 555

#define M 100 //maksymalny rozmiar grupy
#define P 1000 //maksymalna liczba osób w podprzestrzeni

// int podprz_typy[P]; //typy osób w podprzestrzeni
// int podprz_rozmiary[P]; //rozmiary grup w podprzestrzeni


extern MPI_Datatype MPI_PAKIET_T;
void inicjuj_typ_pakietu();

/* wysyłanie pakietu, skrót: wskaźnik do pakietu (0 oznacza stwórz pusty pakiet), do kogo, z jakim typem */
void sendPacket(packet_t *pkt, int destination, int tag);

typedef enum {InRun, InMonitor, InWant, InSection, InFinish  } state_t;
//inRun -DEFAULT
//inMonitor - unused ()
//inWant WAIT
//InSection - INSECTION
//
extern state_t stan;
extern pthread_mutex_t stateMut;
extern pthread_mutex_t lampMut;
extern pthread_mutex_t queueMut;
extern pthread_mutex_t waitingMut;
/* zmiana stanu, obwarowana muteksem */

int isFull(queue_t *q);
int isEmpty(queue_t *q);
int enqueue(int id,int typ,int size,int ts, queue_t *q);
int dequeue(queue_t *q);
int check_first(queue_t *q);
int check_type(queue_t *q);
int check_size(queue_t *q);
void display(queue_t *q);


void changeState( state_t );
#endif
