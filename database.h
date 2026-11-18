/*
 * database.h
 * Prototipi per la gestione della classifica: connessione MySQL,
 * salvataggio/caricamento giocatori e aggiornamento statistiche.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include "tipi.h"

/* Confronto per qsort: ordina per vinte decrescenti, poi per percentuale vittorie */
int confronta_classifica(const void *a, const void *b);

/* Ordina l'array dei record in ordine decrescente di vittorie */
void ordina_classifica(RecordGiocatore *arr, int n);

/* Calcola e aggiorna il campo percentuale_vittorie di un record */
void aggiorna_percentuale(RecordGiocatore *record);

/* Cerca un record per nome, restituisce l'indice o -1 se non trovato */
int trova_record(RecordGiocatore *arr, int n, const char *nome);

/* Aggiunge un nuovo record se il nome non esiste già */
void aggiungi_o_aggiorna_giocatore(StatoUI *ui, const char *nome);

/* Carica la classifica dal database MySQL */
void carica_giocatori_db(ContestoSDL *ctx, StatoUI *ui);

/* Salva tutti i record nel database MySQL (INSERT ... ON DUPLICATE KEY UPDATE) */
void salva_giocatori_db(ContestoSDL *ctx, StatoUI *ui);

/* Carica la classifica dal file CSV di fallback */
void carica_giocatori(StatoUI *ui);

/* Salva la classifica nel file CSV di fallback */
void salva_giocatori(StatoUI *ui);

/* Aggiorna le statistiche del singolo giocatore umano nel database */
void aggiorna_stats_db(ContestoSDL *ctx, const char *nome, int e_vincitore);

/* Aggiorna in memoria le statistiche di tutti i giocatori umani a fine partita */
void aggiorna_statistiche_fine_partita(StatoUI *ui, const char *nome_vincitore,
                                        int num_giocatori);

#endif /* DATABASE_H */
