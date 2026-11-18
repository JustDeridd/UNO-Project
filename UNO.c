#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* ─────────────────────────────────────────
   COSTANTI
───────────────────────────────────────── */
#define MAX_GIOCATORI  6   /* 1 umano + max 5 CPU */
#define MIN_AVVERSARI  1
#define MAX_AVVERSARI  5
#define TOTALE_CARTE   108
#define CARTE_INIZIALI 7
#define MAX_NOME       32

/* ─────────────────────────────────────────
   ENUMERAZIONI
───────────────────────────────────────── */
typedef enum {
    ROSSO, GIALLO, VERDE, BLU,
    NESSUNO   /* usato per le carte jolly */
} Colore;

typedef enum {
    ZERO, UNO, DUE, TRE, QUATTRO, CINQUE,
    SEI, SETTE, OTTO, NOVE,
    STOP,        /* Skip        */
    INVERTI,     /* Reverse     */
    DRAW_TWO,    /* +2          */
    CHANGE_COLOR,/* Wild        */
    DRAW_4       /* Wild +4     */
} TipoCarta;

/* ─────────────────────────────────────────
   STRUTTURE
───────────────────────────────────────── */
typedef struct {
    Colore    colore;
    TipoCarta tipo;
    Colore    colore_scelto; /* rilevante solo per CHANGE_COLOR / DRAW_4 */
} Carta;

/* Mano: array statico (no malloc) — dimensione massima sicura */
typedef struct {
    Carta carte[TOTALE_CARTE]; /* nel caso estremo un giocatore pesca tutto */
    int   num_carte;
} Mano;

/* Mazzo */
typedef struct {
    Carta carte[TOTALE_CARTE];
    int   num_carte;
} Mazzo;

/* Giocatore */
typedef struct {
    char nome[MAX_NOME];
    Mano mano;
    int  e_umano; /* 1 = giocatore reale, 0 = CPU */
} Giocatore;

/* Stato completo della partita */
typedef struct {
    Giocatore giocatori[MAX_GIOCATORI];
    int       num_giocatori;
    Mazzo     mazzo;
    Mazzo     scarti;          /* pila degli scarti (riusata come struct Mazzo) */
    Carta     carta_tavolo;
    int       turno_corrente;  /* indice in giocatori[] */
    int       direzione;       /* +1 senso orario, -1 antiorario */
} StatoPartita;

/* ─────────────────────────────────────────
   UTILITY: nomi leggibili
───────────────────────────────────────── */
const char *colore_str(Colore c) {
    switch (c) {
        case ROSSO:   return "Rosso";
        case GIALLO:  return "Giallo";
        case VERDE:   return "Verde";
        case BLU:     return "Blu";
        case NESSUNO: return "---";
        default:      return "?";
    }
}

const char *tipo_str(TipoCarta t) {
    switch (t) {
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
        case STOP:         return "Stop";
        case INVERTI:      return "Inverti";
        case DRAW_TWO:     return "+2";
        case CHANGE_COLOR: return "Jolly";
        case DRAW_4:       return "Jolly +4";
        default:           return "?";
    }
}

void stampa_carta(Carta c) {
    if (c.tipo == CHANGE_COLOR || c.tipo == DRAW_4)
        printf("[%s]", tipo_str(c.tipo));
    else
        printf("[%s %s]", colore_str(c.colore), tipo_str(c.tipo));
}

/* ─────────────────────────────────────────
   MANO  (array statico)
───────────────────────────────────────── */
void mano_init(Mano *m) {
    m->num_carte = 0;
}

void mano_aggiungi(Mano *m, Carta c) {
    if (m->num_carte < TOTALE_CARTE)
        m->carte[m->num_carte++] = c;
}

void mano_rimuovi(Mano *m, int indice) {
    for (int i = indice; i < m->num_carte - 1; i++)
        m->carte[i] = m->carte[i + 1];
    m->num_carte--;
}

/* ─────────────────────────────────────────
   MAZZO
───────────────────────────────────────── */
void mazzo_init(Mazzo *mazzo) {
    int i = 0;
    Colore colori[4] = { ROSSO, GIALLO, VERDE, BLU };

    for (int c = 0; c < 4; c++) {
        /* 1 zero per colore */
        mazzo->carte[i++] = (Carta){ colori[c], ZERO, NESSUNO };

        /* 2 copie di 1-9 per colore */
        for (int n = UNO; n <= NOVE; n++) {
            mazzo->carte[i++] = (Carta){ colori[c], n, NESSUNO };
            mazzo->carte[i++] = (Carta){ colori[c], n, NESSUNO };
        }

        /* 2 copie di ogni speciale per colore */
        mazzo->carte[i++] = (Carta){ colori[c], STOP,     NESSUNO };
        mazzo->carte[i++] = (Carta){ colori[c], STOP,     NESSUNO };
        mazzo->carte[i++] = (Carta){ colori[c], INVERTI,  NESSUNO };
        mazzo->carte[i++] = (Carta){ colori[c], INVERTI,  NESSUNO };
        mazzo->carte[i++] = (Carta){ colori[c], DRAW_TWO, NESSUNO };
        mazzo->carte[i++] = (Carta){ colori[c], DRAW_TWO, NESSUNO };
    }

    /* 4 Jolly + 4 Jolly +4 */
    for (int j = 0; j < 4; j++) {
        mazzo->carte[i++] = (Carta){ NESSUNO, CHANGE_COLOR, NESSUNO };
        mazzo->carte[i++] = (Carta){ NESSUNO, DRAW_4,       NESSUNO };
    }

    mazzo->num_carte = i; /* deve essere 108 */
}

/* Fisher-Yates shuffle */
void mazzo_mescola(Mazzo *mazzo) {
    for (int i = mazzo->num_carte - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Carta temp        = mazzo->carte[i];
        mazzo->carte[i]   = mazzo->carte[j];
        mazzo->carte[j]   = temp;
    }
}

Carta mazzo_pesca(Mazzo *mazzo) {
    return mazzo->carte[--mazzo->num_carte];
}

/* ─────────────────────────────────────────
   MENU
───────────────────────────────────────── */
int menu(void) {
    int scelta = 0;
    do {
        printf("\n========= MENU' =========\n");
        printf("[1] Inizia nuova partita\n");
        printf("[2] Carica partita\n");
        printf("[3] Registra giocatore\n");
        printf("[4] Cambia giocatore\n");
        printf("[5] Mostra classifica\n");
        printf("[6] Esci dal gioco\n");
        printf("Scelta: ");
        if (scanf("%d", &scelta) != 1) {
            /* svuota il buffer in caso di input non numerico */
            while (getchar() != '\n');
            scelta = 0;
        }
        if (scelta < 1 || scelta > 6)
            printf("Scelta non valida, riprova.\n");
    } while (scelta < 1 || scelta > 6);

    return scelta;
}

/* ─────────────────────────────────────────
   PARTITA
───────────────────────────────────────── */

/* Controlla se una carta è giocabile sul tavolo */
int carta_giocabile(Carta mano, Carta tavolo) {
    /* I jolly si giocano sempre */
    if (mano.tipo == CHANGE_COLOR || mano.tipo == DRAW_4)
        return 1;
    /* Stesso colore o stesso tipo */
    return (mano.colore == tavolo.colore || mano.tipo == tavolo.tipo);
}

void stampa_mano(Mano *m) {
    printf("Le tue carte:\n");
    for (int i = 0; i < m->num_carte; i++) {
        printf("  [%d] ", i + 1);
        stampa_carta(m->carte[i]);
        printf("\n");
    }
}

void nuova_partita(void) {
    StatoPartita partita;
    int num_avversari = 0;

    /* --- Numero avversari --- */
    do {
        printf("Quanti avversari CPU vuoi? [%d-%d]: ",
               MIN_AVVERSARI, MAX_AVVERSARI);
        if (scanf("%d", &num_avversari) != 1) {
            while (getchar() != '\n');
            num_avversari = 0;
        }
        if (num_avversari < MIN_AVVERSARI || num_avversari > MAX_AVVERSARI)
            printf("Numero non valido.\n");
    } while (num_avversari < MIN_AVVERSARI || num_avversari > MAX_AVVERSARI);

    partita.num_giocatori = num_avversari + 1; /* +1 per l'umano */

    /* --- Nomi giocatori --- */
    printf("Il tuo nome: ");
    scanf("%31s", partita.giocatori[0].nome);
    partita.giocatori[0].e_umano = 1;
    mano_init(&partita.giocatori[0].mano);

    for (int i = 1; i <= num_avversari; i++) {
        snprintf(partita.giocatori[i].nome, MAX_NOME, "CPU_%d", i);
        partita.giocatori[i].e_umano = 0;
        mano_init(&partita.giocatori[i].mano);
    }

    /* --- Mazzo --- */
    srand((unsigned int)time(NULL));
    mazzo_init(&partita.mazzo);
    mazzo_mescola(&partita.mazzo);
    partita.scarti.num_carte = 0;

    /* --- Distribuzione 7 carte --- */
    for (int p = 0; p < partita.num_giocatori; p++)
        for (int k = 0; k < CARTE_INIZIALI; k++)
            mano_aggiungi(&partita.giocatori[p].mano,
                          mazzo_pesca(&partita.mazzo));

    /* --- Prima carta sul tavolo (non jolly) --- */
    do {
        partita.carta_tavolo = mazzo_pesca(&partita.mazzo);
    } while (partita.carta_tavolo.tipo == CHANGE_COLOR ||
             partita.carta_tavolo.tipo == DRAW_4);

    partita.turno_corrente = 0;
    partita.direzione      = 1;

    printf("\n--- Partita iniziata! ---\n");
    printf("Carta iniziale: ");
    stampa_carta(partita.carta_tavolo);
    printf("\n");

    /* ─── LOOP DI GIOCO ─── */
    int partita_finita = 0;
    while (!partita_finita) {
        Giocatore *g = &partita.giocatori[partita.turno_corrente];

        printf("\n--- Turno di %s ---\n", g->nome);
        printf("Carta sul tavolo: ");
        stampa_carta(partita.carta_tavolo);
        printf("\n");

        if (g->e_umano) {
            /* ── Turno umano ── */
            stampa_mano(&g->mano);

            /* Cerca se ha carte giocabili */
            int ha_carta = 0;
            for (int i = 0; i < g->mano.num_carte; i++)
                if (carta_giocabile(g->mano.carte[i], partita.carta_tavolo)) {
                    ha_carta = 1;
                    break;
                }

            if (!ha_carta) {
                printf("Nessuna carta giocabile, pesca una carta.\n");
                if (partita.mazzo.num_carte > 0)
                    mano_aggiungi(&g->mano, mazzo_pesca(&partita.mazzo));
            } else {
                int scelta = 0;
                do {
                    printf("Scegli una carta da giocare (1-%d), 0 per pescare: ",
                           g->mano.num_carte);
                    if (scanf("%d", &scelta) != 1) {
                        while (getchar() != '\n');
                        scelta = -1;
                    }
                    if (scelta < 0 || scelta > g->mano.num_carte)
                        printf("Scelta non valida.\n");
                    else if (scelta > 0 &&
                             !carta_giocabile(g->mano.carte[scelta - 1],
                                              partita.carta_tavolo))
                        printf("Carta non giocabile!\n"), scelta = -1;
                } while (scelta < 0 || scelta > g->mano.num_carte);

                if (scelta == 0) {
                    if (partita.mazzo.num_carte > 0)
                        mano_aggiungi(&g->mano, mazzo_pesca(&partita.mazzo));
                } else {
                    Carta giocata = g->mano.carte[scelta - 1];
                    mano_rimuovi(&g->mano, scelta - 1);
                    partita.carta_tavolo = giocata;
                    printf("Hai giocato: ");
                    stampa_carta(giocata);
                    printf("\n");

                    if (g->mano.num_carte == 0) {
                        printf("\n🎉 %s ha vinto!\n", g->nome);
                        partita_finita = 1;
                        break;
                    }
                    if (g->mano.num_carte == 1)
                        printf("UNO!\n");
                }
            }
        } else {
            /* ── Turno CPU (logica semplice) ── */
            int giocata = -1;
            for (int i = 0; i < g->mano.num_carte; i++) {
                if (carta_giocabile(g->mano.carte[i], partita.carta_tavolo)) {
                    giocata = i;
                    break;
                }
            }
            if (giocata == -1) {
                printf("%s pesca una carta.\n", g->nome);
                if (partita.mazzo.num_carte > 0)
                    mano_aggiungi(&g->mano, mazzo_pesca(&partita.mazzo));
            } else {
                Carta c = g->mano.carte[giocata];
                mano_rimuovi(&g->mano, giocata);
                partita.carta_tavolo = c;
                printf("%s gioca: ", g->nome);
                stampa_carta(c);
                printf("\n");

                if (g->mano.num_carte == 0) {
                    printf("\n%s ha vinto!\n", g->nome);
                    partita_finita = 1;
                    break;
                }
                if (g->mano.num_carte == 1)
                    printf("%s dice UNO!\n", g->nome);
            }
        }

        /* Avanza il turno */
        partita.turno_corrente =
            (partita.turno_corrente + partita.direzione +
             partita.num_giocatori) % partita.num_giocatori;
    }
}

/* ─────────────────────────────────────────
   STUB per le voci di menu non ancora
   implementate (placeholder)
───────────────────────────────────────── */
void carica_partita(void) {
    printf("[TODO] Caricamento partita da file — da implementare con SQLite.\n");
}

void registra_giocatore(void) {
    printf("[TODO] Registrazione giocatore — da implementare con SQLite.\n");
}

void cambia_giocatore(void) {
    printf("[TODO] Cambio giocatore — da implementare con SQLite.\n");
}

void mostra_classifica(void) {
    printf("[TODO] Classifica — da implementare con SQLite.\n");
}

/* ─────────────────────────────────────────
   MAIN
───────────────────────────────────────── */
int main(void) {  /* void main() non è standard — usare int main(void) */
    int scelta = 0;
    int esci   = 0;

    while (!esci) {
        scelta = menu();
        switch (scelta) {
            case 1: nuova_partita();      break;
            case 2: carica_partita();     break;
            case 3: registra_giocatore(); break;
            case 4: cambia_giocatore();   break;
            case 5: mostra_classifica();  break;
            case 6:
                printf("Arrivederci!\n");
                esci = 1;
                break;
        }
    }

    return 0;
}