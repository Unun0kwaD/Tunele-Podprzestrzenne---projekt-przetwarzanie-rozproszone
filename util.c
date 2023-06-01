#include "main.h"
#include "util.h"
MPI_Datatype MPI_PAKIET_T;

/*
 * w util.h extern state_t stan (czyli zapowiedź, że gdzieś tam jest definicja
 * tutaj w util.c state_t stan (czyli faktyczna definicja)
 */
state_t stan = InRun;

/* zamek wokół zmiennej współdzielonej między wątkami.
 * Zwróćcie uwagę, że każdy proces ma osobą pamięć, ale w ramach jednego
 * procesu wątki współdzielą zmienne - więc dostęp do nich powinien
 * być obwarowany muteksami
 */
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lampMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queueMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t waitingMut = PTHREAD_MUTEX_INITIALIZER;

struct tagNames_t
{
    const char *name;
    int tag;
} tagNames[] = {{"pakiet aplikacyjny", APP_PKT}, {"finish", FINISH}, {"potwierdzenie", ACK}, {"prośbę o sekcję krytyczną", REQUEST}, {"zwolnienie sekcji krytycznej", RELEASE}};

const char *const tag2string(int tag)
{
    for (int i = 0; i < sizeof(tagNames) / sizeof(struct tagNames_t); i++)
    {
        if (tagNames[i].tag == tag)
            return tagNames[i].name;
    }
    return "<unknown>";
}
/* tworzy typ MPI_PAKIET_T
 */
void inicjuj_typ_pakietu()
{
    /* Stworzenie typu */
    /* Poniższe (aż do MPI_Type_commit) potrzebne tylko, jeżeli
       brzydzimy się czymś w rodzaju MPI_Send(&typ, sizeof(pakiet_t), MPI_BYTE....
    */
    /* sklejone z stackoverflow */
    int blocklengths[NITEMS] = {1, 1, 1,1};
    MPI_Datatype typy[NITEMS] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offsets[NITEMS];
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, rozmiar_grupy);
    offsets[3] = offsetof(packet_t, typ_grupy);

    MPI_Type_create_struct(NITEMS, blocklengths, offsets, typy, &MPI_PAKIET_T);

    MPI_Type_commit(&MPI_PAKIET_T);
}

/* opis patrz util.h */
void sendPacket(packet_t *pkt, int destination, int tag)
{
    int freepkt = 0;
    if (pkt == 0)
    {
        pkt = malloc(sizeof(packet_t));
        freepkt = 1;
    }
    pkt->src = rank;
    pthread_mutex_lock(&lampMut);
    pkt->ts = ++lamp_clock;
    pthread_mutex_unlock(&lampMut);
    MPI_Send(pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    debug("Wysyłam %s do %d\n", tag2string(tag), destination);
    if (freepkt)
        free(pkt);
}

void changeState(state_t newState)
{
    pthread_mutex_lock(&stateMut);
    if (stan == InFinish)
    {
        pthread_mutex_unlock(&stateMut);
        return;
    }
    stan = newState;
    pthread_mutex_unlock(&stateMut);
}

int isFull(queue_t *q)
{
    if ((q->front == q->rear + 1) || (q->front == 0 && q->rear == size - 1))
        return 1;
    return 0;
}
int isEmpty(queue_t *q)
{
    if (q->front == -1)
        return 1;
    else
        return 0;
}

//zwraca -1 jeżeli kolejka jest pełna
int enqueue(int id, int typ, int size, queue_t *q)
{
    pthread_mutex_lock(&queueMut);
    if (isFull(q))
    {
        return -1;
        debug("Kolejka jest pełna");
    }
    else
    {
        //debug("Dodaję %d do kolejki", id);
        //debug("Przód: %d, tył: %d", q->front, q->rear );
        if (q->front == -1)
            q->front = 0;
        q->rear = (q->rear + 1) % size;
        //debug("przód: %d, tył: %d", q->front, q->rear );
        q->ids[q->rear] = id;
        q->types[q->rear] = typ;
        q->sizes[q->rear] = size;
        q->perons_inside += size;
        //debug("W kolejce jest %d osób", q->perons_inside);
    }
    pthread_mutex_unlock(&queueMut);
    return q->rear;
}
//zwraca id usunietego elementu
//zwraca -1 jeżeli kolejka jest pusta
int dequeue(queue_t *q)
{
    pthread_mutex_lock(&queueMut);
    int item;
    if (isEmpty(q))
    {
        pthread_mutex_unlock(&queueMut);
        return -1;
    }
    else
    {
        item = q->ids[q->front];
        q->perons_inside -= q->sizes[q->front];
        if (q->front == q->rear)
        {
            q->front = -1;
            q->rear = -1;
        }
        else
        {
            q->front = (q->front + 1) % size;
        }
    }
    pthread_mutex_unlock(&queueMut);
    return item;
}
//zwraca id pierwszego elementu w kolejce
int check_first(queue_t *q)
{
    pthread_mutex_lock(&queueMut);
    if (q->front == -1)
    {
        return -1;
    }
    return q->ids[q->front];
    pthread_mutex_unlock(&queueMut);
}

//funkcja zwraca KURIER jeżeli może wejść kurier
//funkcja zwraca WYCIECZKA jeżeli może wejść wycieczka
//funkcja zwraca EMPTY jeżeli mogą wejść obie grupy
int check_type(queue_t *q)
{
    pthread_mutex_lock(&queueMut);
    if (isEmpty(q))
    {
        return EMPTY;
    }
    else
    {
        for (int i = q->front; i != q->rear; i = (i + 1) % q->size)
        {
            if (q->types[i] == WYCIECZKA)
            {
                pthread_mutex_unlock(&queueMut);
                return WYCIECZKA;
            }
        }
    }
    return KURIER;
    pthread_mutex_unlock(&queueMut);
}

void display(queue_t *q)
{
    pthread_mutex_lock(&queueMut);
    int i;
    if (isEmpty(q))
    {
        printf("\nKolejka jest pusta\n");
    }
    else
    {
        printf("\nKolejka zawiera: \n");
        for (i = q->front; i != q->rear; i = (i + 1) % q->size)
        {
            printf("%d %d %d\n", q->ids[i], q->types[i], q->sizes[i]);
        }
        printf("%d %d %d\n", q->ids[i], q->types[i], q->sizes[i]);
    }
    pthread_mutex_unlock(&queueMut);
}

int check_size(queue_t *q)
{
    pthread_mutex_lock(&queueMut);
    if (isEmpty(q))
    {
        q->perons_inside = 0;
        return 0;
    }
    else
    {
        return q->perons_inside;
    }
    pthread_mutex_unlock(&queueMut);
}