/*
 * grafica.h
 * Prototipi per i primitivi SDL2 e il caricamento delle risorse grafiche.
 */

#ifndef GRAFICA_H
#define GRAFICA_H

#include "tipi.h"

/* Riempie un rettangolo con il colore specificato */
void disegna_rettangolo_pieno(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c);

/* Riempie un rettangolo con angoli arrotondati */
void disegna_rettangolo_arrotondato(SDL_Renderer *r, int x, int y, int w, int h,
                                     int raggio, SDL_Color c);

/* Disegna il contorno di un rettangolo arrotondato (riempito, non solo bordo) */
void disegna_bordo_arrotondato(SDL_Renderer *r, int x, int y, int w, int h,
                                int raggio, SDL_Color c, int spessore);

/* Disegna un testo centrato in un punto (cx, cy) */
void disegna_testo_centrato(SDL_Renderer *r, TTF_Font *font,
                             const char *testo, SDL_Color c, int cx, int cy);

/* Disegna un testo con allineamento a sinistra a partire da (x, y) */
void disegna_testo(SDL_Renderer *r, TTF_Font *font,
                   const char *testo, SDL_Color c, int x, int y);

/* Disegna una carta usando l'immagine PNG appropriata (o fallback grigio) */
void disegna_carta_png(ContestoSDL *ctx, Carta c, int x, int y, int scoperta);

/* Disegna solo il retro della carta */
void disegna_retro_carta(ContestoSDL *ctx, int x, int y);

/* Carica tutte le texture delle carte PNG */
void carica_texture_carte(ContestoSDL *ctx);

/* Carica le texture di sfondo e le icone del menu */
void carica_risorse_menu(ContestoSDL *ctx);

/* Apre un font TTF con fallback automatico su font di sistema */
TTF_Font *carica_font(int dimensione);

#endif /* GRAFICA_H */
