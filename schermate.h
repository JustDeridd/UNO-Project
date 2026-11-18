/*
 * schermate.h
 * Prototipi per le funzioni di rendering di ogni schermata del gioco.
 */

#ifndef SCHERMATE_H
#define SCHERMATE_H

#include "tipi.h"
#include "grafica.h"
#include "logica.h"
#include "database.h"

/* Calcola la posizione X/Y della carta idx nella mano umano (centrata in basso) */
int posizione_carta_mano(int idx, int totale, int *fuori_x, int *fuori_y);

/* Schermata menu principale */
void render_menu(ContestoSDL *ctx, StatoUI *ui);

/* Schermata setup (scelta nome e numero di avversari) */
void render_setup(ContestoSDL *ctx, StatoUI *ui);

/* Schermata di gioco principale */
void render_gioco(ContestoSDL *ctx, StatoUI *ui);

/* Overlay di scelta colore per i jolly */
void render_scelta_colore(ContestoSDL *ctx, StatoUI *ui);

/* Overlay di pausa (ESC) */
void render_pausa(ContestoSDL *ctx, StatoUI *ui);

/* Schermata di fine partita / vittoria */
void render_vittoria(ContestoSDL *ctx, StatoUI *ui);

/* Schermata classifica */
void render_classifica(ContestoSDL *ctx, StatoUI *ui);

/* Dispatcher principale: chiama il render corretto in base alla schermata attiva */
void render_tutto(ContestoSDL *ctx, StatoUI *ui);

#endif /* SCHERMATE_H */
