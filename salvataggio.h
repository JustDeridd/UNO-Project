/*
 * salvataggio.h
 * Prototipi per il salvataggio e il caricamento della partita in formato JSON.
 */

#ifndef SALVATAGGIO_H
#define SALVATAGGIO_H

#include "tipi.h"

/* Serializza la partita corrente su file JSON */
void salva_partita_json(StatoUI *ui);

/* Carica una partita dal file JSON. Restituisce 1 se riuscito, 0 altrimenti */
int carica_partita_json(StatoUI *ui);

/* Elimina il file di salvataggio dal disco */
void elimina_salvataggio(void);

#endif /* SALVATAGGIO_H */
