/* w main.h także makra println oraz debug -  z kolorkami! */
#include "main.h"
#include "watek_glowny.h"
#include "watek_komunikacyjny.h"

/*
 * W main.h extern int rank (zapowiedź) w main.c int rank (definicja)
 * Zwróćcie uwagę, że każdy proces ma osobą pamięć, ale w ramach jednego
 * procesu wątki współdzielą zmienne - więc dostęp do nich powinien
 * być obwarowany muteksami. Rank i size akurat są write-once, więc nie trzeba,
 * ale zob util.c oraz util.h - zmienną state_t state i funkcję changeState
 *
 */
int rank, size;
int ackCount = 0;
int lamp_clock = 0;
queue_t podprzestrzen;
int *waiting;
int waitingSize = 0;

/*
 * Każdy proces ma dwa wątki - główny i komunikacyjny
 * w plikach, odpowiednio, watek_glowny.c oraz (siurpryza) watek_komunikacyjny.c
 *
 *
 */

pthread_t threadKom;

void finalizuj()
{
    pthread_mutex_destroy(&stateMut);
    pthread_mutex_destroy(&lampMut);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n");
    pthread_join(threadKom, NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided)
    {
    case MPI_THREAD_SINGLE:
        printf("Brak wsparcia dla wątków, kończę\n");
        /* Nie ma co, trzeba wychodzić */
        fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
        MPI_Finalize();
        exit(-1);
        break;
    case MPI_THREAD_FUNNELED:
        printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
        break;
    case MPI_THREAD_SERIALIZED:
        /* Potrzebne zamki wokół wywołań biblioteki MPI */
        printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
        break;
    case MPI_THREAD_MULTIPLE:
        printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
        break;
    default:
        printf("Nikt nic nie wie\n");
    }
}

int main(int argc, char **argv)
{
    MPI_Status status;
    int provided;

    podprzestrzen.front = podprzestrzen.rear = -1;
    podprzestrzen.size = P;
    podprzestrzen.perons_inside = 0;
    podprzestrzen.ids = malloc(sizeof(int) * P);
    podprzestrzen.types = malloc(sizeof(int) * P);
    podprzestrzen.sizes = malloc(sizeof(int) * size);

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    // debug("Inicjalizacja");
    srand(rank);
    /* zob. util.c oraz util.h */
    inicjuj_typ_pakietu(); // tworzy typ pakietu
    // debug("Inicjalizacja zakonczona");
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // debug("Proces %d z %d", rank, size);

    waiting = malloc(sizeof(int)*size);

    /* startKomWatek w watek_komunikacyjny.c
     * w vi najedź kursorem na nazwę pliku i wciśnij klawisze gf
     * powrót po wciśnięciu ctrl+6
     * */
    // debug("Start wątku komunikacyjnego");
    pthread_create(&threadKom, NULL, startKomWatek, 0);

    /* mainLoop w watek_glowny.c
     * w vi najedź kursorem na nazwę pliku i wciśnij klawisze gf
     * powrót po wciśnięciu ctrl+6
     * */
    mainLoop(); // możesz także wcisnąć ctrl-] na nazwie funkcji
                // działa, bo używamy ctags (zob Makefile)
                // jak nie działa, wpisz set tags=./tags :)

    finalizuj();


    free(podprzestrzen.ids);
    free(podprzestrzen.types);
    free(podprzestrzen.sizes);
    free(waiting);
    return 0;
}
