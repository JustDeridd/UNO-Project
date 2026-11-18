/*
 * eventi.h
 * Prototipi per la gestione degli eventi SDL in ogni schermata.
 */

#ifndef EVENTI_H
#define EVENTI_H

#include "tipi.h"

/* Dispatcher: smista l'evento alla funzione corretta in base alla schermata */
void gestisci_evento(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui);

/* Gestione eventi nella schermata menu */
void evento_menu(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui);

/* Gestione eventi nella schermata setup */
void evento_setup(SDL_Event *evento, StatoUI *ui);

/* Gestione eventi durante il gioco */
void evento_gioco(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui);

/* Gestione eventi nella schermata di scelta colore */
void evento_scelta_colore(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui);

/* Gestione eventi nella schermata di pausa */
void evento_pausa(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui);

/* Gestione eventi nella schermata classifica */
void evento_classifica(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui);

/* Gestione eventi nella schermata di vittoria */
void evento_vittoria(SDL_Event *evento, StatoUI *ui);

#endif /* EVENTI_H */
