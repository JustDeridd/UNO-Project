/*
 * logica.h
 * Prototipi delle funzioni di logica di gioco: gestione mazzo, mano,
 * inizializzazione partita, effetti carte speciali e turno CPU.
 */

#ifndef LOGICA_H
#define LOGICA_H

#include "tipi.h"

/* Aggiunge una carta alla mano */
void mano_aggiungi(Mano *mano, Carta carta);

/* Rimuove la carta all'indice indicato dalla mano */
void mano_rimuovi(Mano *mano, int indice);

/* Popola un mazzo completo con le 108 carte standard UNO */
void mazzo_init(Mazzo *mazzo);

/* Mescola il mazzo con l'algoritmo Fisher-Yates */
void mazzo_mescola(Mazzo *mazzo);

/* Pesca l'ultima carta del mazzo e la restituisce */
Carta mazzo_pesca(Mazzo *mazzo);

/* Ricicla gli scarti nel mazzo quando quest'ultimo è esaurito */
void ricicla_scarti(StatoPartita *partita, StatoUI *ui);

/* Restituisce 1 se la carta può essere giocata sul tavolo corrente */
int carta_giocabile(Carta mano, Carta tavolo);

/* Inizializza una nuova partita a partire dalle impostazioni in StatoUI */
void init_partita(StatoUI *ui);

/* Applica gli effetti della carta appena giocata (stop, inverti, pesca) */
void applica_effetto(StatoUI *ui, Carta giocata);

/* Esegue il turno di un giocatore CPU */
void turno_cpu(StatoUI *ui, ContestoSDL *ctx);

/* Restituisce la stringa di visualizzazione del tipo carta (es. "0", "+2") */
const char *tipo_str(TipoCarta tipo);

/* Restituisce il colore SDL corrispondente a un Colore di gioco */
SDL_Color colore_sdl(Colore colore);

#endif /* LOGICA_H */
