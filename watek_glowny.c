#include "main.h"
#include "watek_glowny.h"

void mainLoop()
{
    srandom(rank);
    int tag;
    int perc;
	int traveler;
	int rozmiar;
    while (stan != InFinish) {
	switch (stan) {
	    case InRun: 
		perc = random()%100;
		if ( perc < 25 ) {
			if (random()%100 < 10 ){
				traveler=KURIER;
				rozmiar=1;
			}
			else {
				traveler=WYCIECZKA;
		    	rozmiar=random()%100;
			}
			debug("Perc: %d", perc);
		    println("Ubiegam się o sekcję krytyczną")
		    debug("Zmieniam stan na wysyłanie");
		    packet_t *pkt = malloc(sizeof(packet_t));
		    pkt->typ_grupy=traveler;
			pkt->rozmiar_grupy=rozmiar;
		    ackCount = 0;
		    for (int i=0;i<=size-1;i++)
			if (i!=rank)
			    sendPacket( pkt, i, REQUEST);
		    changeState( InWant ); // w VI naciśnij ctrl-] na nazwie funkcji, ctrl+^ żeby wrócić
					   // :w żeby zapisać, jeżeli narzeka że w pliku są zmiany
					   // ewentualnie wciśnij ctrl+w ] (trzymasz ctrl i potem najpierw w, potem ]
					   // między okienkami skaczesz ctrl+w i strzałki, albo ctrl+ww
					   // okienko zamyka się :q
					   // ZOB. regułę tags: w Makefile (naciśnij gf gdy kursor jest na nazwie pliku)
		    free(pkt);
		} // a skoro już jesteśmy przy komendach vi, najedź kursorem na } i wciśnij %  (niestety głupieje przy komentarzach :( )
		debug("Skończyłem myśleć");
		break;
	    case InWant:
		println("Czekam na wejście do sekcji krytycznej")
		// tutaj zapewne jakiś muteks albo zmienna warunkowa
		// bo aktywne czekanie jest BUE
		if ( ackCount == size - 1) //sprawdź także czy się miesci w tunelu oraz dla kuriera czy tunel jest pusty lub ma samych kurierów
		    changeState(InSection);
		
		break;
	    case InSection:
		// tutaj zapewne jakiś muteks albo zmienna warunkowa

		//odeślij ACK do wszytkich listy kórym nie wysłaliśmy wcześniej
		println("Jestem w sekcji krytycznej")
		    sleep(5);
		//if ( perc < 25 ) {
		    debug("Perc: %d", perc);
		    println("Wychodzę z sekcji krytyczneh")
			//send RELEASE z informacją na temat tego ile rozmiaru zwalnia
		    debug("Zmieniam stan na wysyłanie");
		    packet_t *pkt = malloc(sizeof(packet_t));
		    pkt->data = perc;
		    for (int i=0;i<=size-1;i++)
			if (i!=rank)
			    sendPacket( pkt, (rank+1)%size, RELEASE);
		    changeState( InRun );
		    free(pkt);
		//}
		break;
	    default: 
		break;
            }
        sleep(SEC_IN_STATE);
    }
}
