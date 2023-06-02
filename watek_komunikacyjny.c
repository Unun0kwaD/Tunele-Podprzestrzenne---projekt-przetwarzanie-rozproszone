#include "main.h"
#include "watek_komunikacyjny.h"

#define max(a,b) ((a)>(b)?(a):(b))

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
extern int lamp_clock;
extern pthread_mutex_t lampMut;


void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;
    
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while ( stan!=InFinish ) {
	debug("czekam na recv");
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock(&lampMut);
        lamp_clock=max( lamp_clock, pakiet.ts )+1;
        pthread_mutex_unlock(&lampMut);
        
        switch ( status.MPI_TAG ) {
	    case REQUEST: 
            pthread_mutex_lock(&lampMut);
            if ((pakiet.ts < lamp_clock) || (pakiet.ts==lamp_clock && pakiet.src<rank) || stan!=InWant ){  // OR jeśli nie ubiega się o dostęp (stan != InWant)
                pthread_mutex_unlock(&lampMut);
                debug("Ktoś coś prosi. A niech ma!")
		        sendPacket( 0, status.MPI_SOURCE, ACK );
                 }
            else {
                //zapamiętaj komu nie odpowiedzieliśmy ACK --> dodaj do kolejki proc. oczekujacych
                //if (waiting->length)
                pthread_mutex_lock(&waitingMut);
                if(waitingSize < size) {
                    waiting[waitingSize++] = status.MPI_SOURCE;
                }
                pthread_mutex_unlock(&waitingMut);
            }
	    break;


	    case ACK: 
            ackCount++;
            debug("Dostałem ACK od %d, mam już %d", status.MPI_SOURCE, ackCount);
	         /* czy potrzeba tutaj muteksa? Będzie wyścig, czy nie będzie? Zastanówcie się. */
	    break;


        case RELEASE:
            debug("Dostałem RELEASE od %d", status.MPI_SOURCE);
            //usun proces z lokalnej kolejki procesów w tunelu
            dequeue(&podprzestrzen);
        break;
        

        case INSECTION:
            debug("Dostałem INSECTION od %d", status.MPI_SOURCE);
            //usun proces z lokalnej kolejki procesów w tunelu
            // dodaj proces do lokalnej kolejki procesów w tunelu
            int t=enqueue(pakiet.src, pakiet.typ_grupy, pakiet.rozmiar_grupy,pakiet.ts, &podprzestrzen);
            debug("Dodaję proces %d do kolejki procesów w tunelu, t=%d", pakiet.src,t);

        break;
            
	    default:
	    break;
        }
    }
}
