/*
 * tipi.h
 * Definizioni globali: include di sistema, costanti, enumerazioni e strutture
 * condivise tra tutti i moduli del gioco UNO.
 */

#ifndef TIPI_H
#define TIPI_H

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "C:/xampp/mysql/include/mysql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ═══════════════════════════════════════════
   COSTANTI DI GIOCO E INTERFACCIA
═══════════════════════════════════════════ */
#define LARG_FINESTRA       1280
#define ALTO_FINESTRA       720
#define MAX_GIOCATORI       6
#define MIN_AVVERSARI       1
#define MAX_AVVERSARI       5
#define TOTALE_CARTE        108
#define CARTE_INIZIALI      7
#define MAX_NOME            32
#define MAX_RECORD_GIOCATORI 256
#define FILE_GIOCATORI      "players.csv"
#define FILE_SALVATAGGIO    "salvataggio.json"

/* Alias di compatibilità usati nei moduli di rendering */
#define WIN_W               LARG_FINESTRA
#define WIN_H               ALTO_FINESTRA

/* Dimensioni carta grafica */
#define LARG_CARTA          80
#define ALTO_CARTA          120
#define RAGGIO_CARTA        8
#define SPAZIATURA_CARTA    18

/* Percorso font principale (con fallback di sistema) */
#define PERCORSO_FONT       "font.ttf"

/* Frame per secondo target */
#define FPS_TARGET          60

/* Alias per compatibilità con codice di rendering */
#define CARD_W              LARG_CARTA
#define CARD_H              ALTO_CARTA
#define CARD_RADIUS         RAGGIO_CARTA
#define CARD_SPACING        SPAZIATURA_CARTA
#define TARGET_FPS          FPS_TARGET
#define FONT_PATH           PERCORSO_FONT

/* ═══════════════════════════════════════════
   COLORI SDL (macro per inizializzatori struct)
═══════════════════════════════════════════ */
#define COL_BG          { 20,  90,  40, 255}
#define COL_BG2         { 15,  70,  30, 255}
#define COL_ROSSO_SDL   {220,  50,  50, 255}
#define COL_GIALLO_SDL  {230, 190,  30, 255}
#define COL_VERDE_SDL   { 50, 180,  80, 255}
#define COL_BLU_SDL     { 50, 100, 210, 255}
#define COL_JOLLY       { 30,  30,  30, 255}
#define COL_BIANCO      {255, 255, 255, 255}
#define COL_NERO        {  0,   0,   0, 255}
#define COL_EVIDENZIA   {255, 220,  60, 200}
#define COL_OVERLAY     {  0,   0,   0, 160}
#define COL_PANNELLO    { 10,  50,  20, 230}
#define COL_PULSANTE    { 40, 140,  60, 255}
#define COL_RETRO_CARTA { 30,  60, 160, 255}

/* ═══════════════════════════════════════════
   ENUMERAZIONI DI GIOCO
═══════════════════════════════════════════ */

/* Colori delle carte */
typedef enum { ROSSO, GIALLO, VERDE, BLU, NESSUNO } Colore;

/* Tipi di carta */
typedef enum {
    ZERO, UNO, DUE, TRE, QUATTRO, CINQUE,
    SEI, SETTE, OTTO, NOVE,
    STOP, INVERTI, DRAW_TWO,
    CHANGE_COLOR, DRAW_4
} TipoCarta;

/* Identificatori delle schermate del gioco */
typedef enum {
    SCHERMATA_MENU,
    SCHERMATA_SETUP,
    SCHERMATA_GIOCO,
    SCHERMATA_SCELTA_COLORE,
    SCHERMATA_PAUSA,
    SCHERMATA_VITTORIA,
    SCHERMATA_CLASSIFICA,
    SCHERMATA_ESCI
} Schermata;

/* ═══════════════════════════════════════════
   STRUTTURE DATI DI GIOCO
═══════════════════════════════════════════ */

/* Singola carta: colore, tipo e colore scelto (per i jolly) */
typedef struct {
    Colore   colore;
    TipoCarta tipo;
    Colore   colore_scelto;
} Carta;

/* Mano di un giocatore o mazzo generico */
typedef struct {
    Carta carte[TOTALE_CARTE];
    int   num_carte;
} Mano;

/* Alias identico a Mano, usato per il mazzo e gli scarti */
typedef Mano Mazzo;

/* Giocatore: nome, mano e flag umano/CPU */
typedef struct {
    char nome[MAX_NOME];
    Mano mano;
    int  e_umano; /* 1 = giocatore umano, 0 = CPU */
} Giocatore;

/* Stato completo di una partita in corso */
typedef struct {
    Giocatore giocatori[MAX_GIOCATORI];
    int       num_giocatori;
    Mazzo     mazzo;
    Mazzo     scarti;
    Carta     carta_tavolo;
    int       turno_corrente;
    int       direzione; /* +1 orario, -1 antiorario */
} StatoPartita;

/* Record classifica di un singolo giocatore */
typedef struct {
    char  nome[MAX_NOME];
    int   vinte;
    int   perse;
    int   totali;
    float percentuale_vittorie;
} RecordGiocatore;

/* Stato animazione carta giocata */
typedef struct {
    int   attiva;
    float x, y;   /* posizione corrente */
    float dx, dy;  /* destinazione */
    float alfa;
    int   fotogramma;
} AnimCarta;

/* ═══════════════════════════════════════════
   CONTESTO SDL (finestra, renderer, font, texture)
═══════════════════════════════════════════ */
typedef struct {
    SDL_Window   *finestra;
    SDL_Renderer *renderer;
    TTF_Font     *font_grande;
    TTF_Font     *font_medio;
    TTF_Font     *font_piccolo;
    SDL_Texture  *tex_carte[5][15];       /* [Colore][Valore] */
    SDL_Texture  *tex_retro;
    SDL_Texture  *tex_sfondo_menu;
    SDL_Texture  *tex_sfondo_tavolo;
    SDL_Texture  *tex_sfondo_fine_partita;
    SDL_Texture  *tex_sfondo_classifica;
    SDL_Texture  *sfondo_setup_rosso;
    SDL_Texture  *sfondo_setup_giallo;
    SDL_Texture  *sfondo_setup_verde;
    SDL_Texture  *sfondo_setup_blu;
    SDL_Texture  *tex_pulsante_uno;
    SDL_Texture  *tex_menu_icone[4];
    MYSQL        *connessione_db;
} ContestoSDL;

/* ═══════════════════════════════════════════
   STATO UI GLOBALE
═══════════════════════════════════════════ */
typedef struct {
    Schermata       schermata;
    StatoPartita    partita;
    int             carta_selezionata;       /* indice carta in mano umano, -1 = nessuna */
    int             hover_carta;             /* indice carta sotto il cursore */
    int             hover_pesca;             /* cursore sopra il mazzo */
    char            nome_input[MAX_NOME];    /* nome digitato nella schermata setup */
    int             lunghezza_nome;
    int             num_avversari_setup;     /* numero di CPU scelti (1-5) */
    AnimCarta       animazione;
    char            messaggio[128];          /* messaggio temporaneo a schermo */
    int             timer_messaggio;
    int             ritardo_cpu;             /* frame di attesa prima del turno CPU */
    Colore          colore_scelto_jolly;     /* colore scelto dall'umano per un jolly */
    int             carta_da_giocare;        /* indice carta jolly in attesa del colore */

    RecordGiocatore record[MAX_RECORD_GIOCATORI];
    int             num_record;

    int             indice_giocatore_attivo;
    int             scroll_classifica;       /* offset scroll nella schermata classifica */

    int             mostra_riciclo_mazzo;    /* 1 = mostra avviso riciclo scarti */
    int             scelta_sfondo_setup;     /* indice sfondo casuale per il setup (0-3) */

    /* Logica pulsante UNO */
    int             mostra_pulsante_uno;     /* 1 = pulsante visibile */
    Uint32          timer_pulsante_uno;      /* timestamp di attivazione (ms) */
    int             ha_cliccato_uno;         /* 1 = giocatore ha premuto UNO in tempo */
} StatoUI;

#endif /* TIPI_H */
