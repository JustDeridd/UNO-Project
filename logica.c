/*
 * logica.c
 * Implementazione della logica di gioco: mazzo, mano, partita, effetti
 * delle carte speciali e intelligenza artificiale dei giocatori CPU.
 */

#include "logica.h"
#include "database.h"

/* ═══════════════════════════════════════════
   UTILITY STRINGHE
═══════════════════════════════════════════ */

/* Restituisce la stringa breve del tipo di carta per la visualizzazione */
const char *tipo_str(TipoCarta tipo) {
    switch (tipo) {
        case ZERO:         return "0";
        case UNO:          return "1";
        case DUE:          return "2";
        case TRE:          return "3";
        case QUATTRO:      return "4";
        case CINQUE:       return "5";
        case SEI:          return "6";
        case SETTE:        return "7";
        case OTTO:         return "8";
        case NOVE:         return "9";
        case STOP:         return "STOP";
        case INVERTI:      return "INV";
        case DRAW_TWO:     return "+2";
        case CHANGE_COLOR: return "W";
        case DRAW_4:       return "W+4";
        default:           return "?";
    }
}

/* Restituisce il colore SDL corrispondente al colore di gioco */
SDL_Color colore_sdl(Colore colore) {
    switch (colore) {
        case ROSSO:  { SDL_Color c = {220,  50,  50, 255}; return c; }
        case GIALLO: { SDL_Color c = {230, 190,  30, 255}; return c; }
        case VERDE:  { SDL_Color c = { 50, 180,  80, 255}; return c; }
        case BLU:    { SDL_Color c = { 50, 100, 210, 255}; return c; }
        default:     { SDL_Color c = { 30,  30,  30, 255}; return c; }
    }
}

/* ═══════════════════════════════════════════
   GESTIONE MANO
═══════════════════════════════════════════ */

/* Aggiunge una carta alla mano del giocatore, rispettando il limite massimo */
void mano_aggiungi(Mano *mano, Carta carta) {
    if (mano->num_carte < TOTALE_CARTE)
        mano->carte[mano->num_carte++] = carta;
}

/* Rimuove la carta all'indice specificato scorrendo le successive a sinistra */
void mano_rimuovi(Mano *mano, int indice) {
    if (indice < 0 || indice >= mano->num_carte) return;
    for (int i = indice; i < mano->num_carte - 1; i++)
        mano->carte[i] = mano->carte[i + 1];
    mano->num_carte--;
}

/* ═══════════════════════════════════════════
   GESTIONE MAZZO
═══════════════════════════════════════════ */

/* Crea il mazzo standard UNO con 108 carte (4 colori + 8 jolly) */
void mazzo_init(Mazzo *mazzo) {
    int    i = 0;
    Colore colori[4] = { ROSSO, GIALLO, VERDE, BLU };

    for (int c = 0; c < 4; c++) {
        /* Ogni colore ha: uno ZERO, due copie di 1-9, due STOP, INVERTI e +2 */
        mazzo->carte[i++] = (Carta){ colori[c], ZERO,     NESSUNO };
        for (int n = UNO; n <= NOVE; n++) {
            mazzo->carte[i++] = (Carta){ colori[c], n, NESSUNO };
            mazzo->carte[i++] = (Carta){ colori[c], n, NESSUNO };
        }
        mazzo->carte[i++] = (Carta){ colori[c], STOP,     NESSUNO };
        mazzo->carte[i++] = (Carta){ colori[c], STOP,     NESSUNO };
        mazzo->carte[i++] = (Carta){ colori[c], INVERTI,  NESSUNO };
        mazzo->carte[i++] = (Carta){ colori[c], INVERTI,  NESSUNO };
        mazzo->carte[i++] = (Carta){ colori[c], DRAW_TWO, NESSUNO };
        mazzo->carte[i++] = (Carta){ colori[c], DRAW_TWO, NESSUNO };
    }
    /* 4 Cambia Colore + 4 Pesca Quattro */
    for (int j = 0; j < 4; j++) {
        mazzo->carte[i++] = (Carta){ NESSUNO, CHANGE_COLOR, NESSUNO };
        mazzo->carte[i++] = (Carta){ NESSUNO, DRAW_4,       NESSUNO };
    }
    mazzo->num_carte = i;
}

/* Mescola il mazzo in-place con l'algoritmo Fisher-Yates */
void mazzo_mescola(Mazzo *mazzo) {
    for (int i = mazzo->num_carte - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Carta tmp       = mazzo->carte[i];
        mazzo->carte[i] = mazzo->carte[j];
        mazzo->carte[j] = tmp;
    }
}

/* Preleva l'ultima carta del mazzo e la restituisce */
Carta mazzo_pesca(Mazzo *mazzo) {
    return mazzo->carte[--mazzo->num_carte];
}

/* Sposta gli scarti nel mazzo, mescola e mostra avviso a schermo */
void ricicla_scarti(StatoPartita *partita, StatoUI *ui) {
    if (partita->scarti.num_carte <= 0) return;

    for (int i = 0; i < partita->scarti.num_carte; i++)
        partita->mazzo.carte[i] = partita->scarti.carte[i];

    partita->mazzo.num_carte  = partita->scarti.num_carte;
    partita->scarti.num_carte = 0;
    mazzo_mescola(&partita->mazzo);

    strcpy(ui->messaggio, "MAZZO RICICLATO!");
    ui->timer_messaggio = 50;
}

/* ═══════════════════════════════════════════
   REGOLE DI GIOCABILITÀ
═══════════════════════════════════════════ */

/* Una carta è giocabile se è un jolly oppure se combacia per colore o tipo */
int carta_giocabile(Carta mano, Carta tavolo) {
    if (mano.tipo == CHANGE_COLOR || mano.tipo == DRAW_4) return 1;
    return (mano.colore == tavolo.colore || mano.tipo == tavolo.tipo);
}

/* ═══════════════════════════════════════════
   INIZIALIZZAZIONE PARTITA
═══════════════════════════════════════════ */

/* Prepara una nuova partita: giocatori, mazzo, distribuzione carte iniziali */
void init_partita(StatoUI *ui) {
    StatoPartita *partita = &ui->partita;
    partita->num_giocatori = ui->num_avversari_setup + 1;

    /* Giocatore umano */
    strncpy(partita->giocatori[0].nome, ui->nome_input, MAX_NOME);
    partita->giocatori[0].e_umano       = 1;
    partita->giocatori[0].mano.num_carte = 0;

    /* Giocatori CPU */
    for (int i = 1; i <= ui->num_avversari_setup; i++) {
        snprintf(partita->giocatori[i].nome, MAX_NOME, "BOT %d", i);
        partita->giocatori[i].e_umano       = 0;
        partita->giocatori[i].mano.num_carte = 0;
    }

    srand((unsigned int)time(NULL));
    mazzo_init(&partita->mazzo);
    mazzo_mescola(&partita->mazzo);
    partita->scarti.num_carte = 0;

    /* Distribuisce CARTE_INIZIALI carte a ciascun giocatore */
    for (int pg = 0; pg < partita->num_giocatori; pg++)
        for (int k = 0; k < CARTE_INIZIALI; k++)
            mano_aggiungi(&partita->giocatori[pg].mano, mazzo_pesca(&partita->mazzo));

    /* La prima carta sul tavolo non può essere un jolly */
    do {
        partita->carta_tavolo = mazzo_pesca(&partita->mazzo);
    } while (partita->carta_tavolo.tipo == CHANGE_COLOR ||
             partita->carta_tavolo.tipo == DRAW_4);

    partita->mazzo.num_carte = TOTALE_CARTE;
    partita->turno_corrente  = 0;
    partita->direzione       = 1;

    /* Reset stato UI */
    ui->carta_selezionata   = -1;
    ui->hover_carta         = -1;
    ui->messaggio[0]        = '\0';
    ui->timer_messaggio     = 0;
    ui->ritardo_cpu         = 0;
    ui->carta_da_giocare    = -1;
    ui->animazione.attiva   = 0;
    ui->mostra_pulsante_uno = 0;
    ui->ha_cliccato_uno     = 0;
}

/* ═══════════════════════════════════════════
   EFFETTI CARTE SPECIALI
═══════════════════════════════════════════ */

/* Applica l'effetto della carta appena giocata e avanza il turno */
void applica_effetto(StatoUI *ui, Carta giocata) {
    StatoPartita *partita = &ui->partita;
    int prossimo = (partita->turno_corrente + partita->direzione + partita->num_giocatori)
                   % partita->num_giocatori;

    if (giocata.tipo == INVERTI) {
        partita->direzione *= -1;
        /* Con 2 giocatori INVERTI equivale a STOP */
        if (partita->num_giocatori == 2)
            prossimo = partita->turno_corrente;
    }

    if (giocata.tipo == STOP)
        prossimo = (prossimo + partita->direzione + partita->num_giocatori)
                   % partita->num_giocatori;

    if (giocata.tipo == DRAW_TWO) {
        for (int k = 0; k < 2 && partita->mazzo.num_carte > 0; k++)
            mano_aggiungi(&partita->giocatori[prossimo].mano, mazzo_pesca(&partita->mazzo));
        snprintf(ui->messaggio, 128, "%s pesca 2!", partita->giocatori[prossimo].nome);
        ui->timer_messaggio = 50;
        prossimo = (prossimo + partita->direzione + partita->num_giocatori) % partita->num_giocatori;
    }

    if (giocata.tipo == DRAW_4) {
        for (int k = 0; k < 4 && partita->mazzo.num_carte > 0; k++)
            mano_aggiungi(&partita->giocatori[prossimo].mano, mazzo_pesca(&partita->mazzo));
        snprintf(ui->messaggio, 128, "%s pesca 4!", partita->giocatori[prossimo].nome);
        ui->timer_messaggio = 50;
        prossimo = (prossimo + partita->direzione + partita->num_giocatori) % partita->num_giocatori;
    }

    partita->turno_corrente = prossimo;
}

/* ═══════════════════════════════════════════
   TURNO CPU
═══════════════════════════════════════════ */

/* Gestisce il turno automatico del bot: sceglie la carta migliore o pesca */
void turno_cpu(StatoUI *ui, ContestoSDL *ctx) {
    StatoPartita *partita  = &ui->partita;
    Giocatore    *giocatore = &partita->giocatori[partita->turno_corrente];

    /* Ricicla gli scarti se il mazzo è esaurito */
    if (partita->mazzo.num_carte == 0 && partita->scarti.num_carte > 0) {
        for (int i = 0; i < partita->scarti.num_carte; i++)
            partita->mazzo.carte[i] = partita->scarti.carte[i];
        partita->mazzo.num_carte  = partita->scarti.num_carte;
        partita->scarti.num_carte = 0;
        mazzo_mescola(&partita->mazzo);
        ui->mostra_riciclo_mazzo = 1;
    }

    /* Strategia: preferisce carte speciali, poi numeriche */
    int indice_giocata = -1;
    for (int i = 0; i < giocatore->mano.num_carte; i++) {
        if (carta_giocabile(giocatore->mano.carte[i], partita->carta_tavolo)) {
            if (indice_giocata == -1) indice_giocata = i;
            if (giocatore->mano.carte[i].tipo >= STOP) {
                indice_giocata = i;
                break;
            }
        }
    }

    if (indice_giocata == -1) {
        /* Nessuna carta giocabile: pesca */
        if (partita->mazzo.num_carte > 0)
            mano_aggiungi(&giocatore->mano, mazzo_pesca(&partita->mazzo));
        snprintf(ui->messaggio, 128, "%s pesca una carta", giocatore->nome);
        ui->timer_messaggio = 40;
        partita->turno_corrente = (partita->turno_corrente + partita->direzione + partita->num_giocatori)
                                  % partita->num_giocatori;
        return;
    }

    Carta carta_giocata = giocatore->mano.carte[indice_giocata];
    /* Salva la carta del tavolo negli scarti prima di sostituirla */
    partita->scarti.carte[partita->scarti.num_carte++] = partita->carta_tavolo;

    /* Per i jolly il CPU sceglie il colore più frequente in mano */
    if (carta_giocata.tipo == CHANGE_COLOR || carta_giocata.tipo == DRAW_4) {
        int conteggio[4] = {0};
        for (int i = 0; i < giocatore->mano.num_carte; i++)
            if (giocatore->mano.carte[i].colore != NESSUNO)
                conteggio[giocatore->mano.carte[i].colore]++;
        Colore migliore = ROSSO;
        for (int col = 1; col < 4; col++)
            if (conteggio[col] > conteggio[migliore]) migliore = col;
        carta_giocata.colore_scelto = migliore;
        carta_giocata.colore        = migliore;
    }

    partita->carta_tavolo = carta_giocata;
    mano_rimuovi(&giocatore->mano, indice_giocata);

    if (giocatore->mano.num_carte == 1) {
        snprintf(ui->messaggio, 128, "%s dice UNO!", giocatore->nome);
        ui->timer_messaggio = 50;
    }

    if (giocatore->mano.num_carte == 0) {
        snprintf(ui->messaggio, 128, "%s ha vinto!", giocatore->nome);
        ui->timer_messaggio = 90;
        aggiorna_statistiche_fine_partita(ui, giocatore->nome, partita->num_giocatori);
        /* Aggiorna le statistiche nel database per il giocatore umano */
        for (int k = 0; k < partita->num_giocatori; k++) {
            if (!partita->giocatori[k].e_umano) continue;
            aggiorna_stats_db(ctx, partita->giocatori[k].nome,
                strcmp(partita->giocatori[k].nome, giocatore->nome) == 0);
        }
        ui->schermata = SCHERMATA_VITTORIA;
        return;
    }

    applica_effetto(ui, carta_giocata);
}
