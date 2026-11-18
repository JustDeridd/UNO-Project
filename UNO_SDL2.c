/*
 * UNO - SDL2 Graphical Version (con classifica MySQL)
 *
 * Dipendenze:
 *   sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev default-libmysqlclient-dev
 *   (Linux)
 *   brew install sdl2 sdl2_ttf sdl2_image mysql-connector-c   (macOS)
 *
 * Compilazione:
 *   gcc UNO_SDL2.c -o uno $(sdl2-config --cflags --libs) -lSDL2_ttf -lSDL2_image $(mysql_config --cflags --libs) -lm
 *
 * Setup XAMPP MySQL:
 *   1. Avvia XAMPP e assicurati che MySQL sia in esecuzione sulla porta 3306
 *   2. Crea il database: CREATE DATABASE uno_gioco;
 *   3. Crea la tabella:
 *      CREATE TABLE classifica (
 *          id INT AUTO_INCREMENT PRIMARY KEY,
 *          nome VARCHAR(32) NOT NULL UNIQUE,
 *          partite_vinte INT DEFAULT 0,
 *          partite_perse INT DEFAULT 0,
 *          partite_giocate INT DEFAULT 0
 *      );
 *   4. User: root, Password: (vuota) - configurabile nel codice
 *
 * Su Windows con MinGW:
 *   gcc UNO_SDL2.c -o uno.exe -Iinclude -Llib -lSDL2 -lSDL2_ttf -lSDL2_image -lmysql -lm
 */

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
   COSTANTI
═══════════════════════════════════════════ */
#define WIN_W           1280
#define WIN_H           720
#define MAX_GIOCATORI   6
#define MIN_AVVERSARI   1
#define MAX_AVVERSARI   5
#define TOTALE_CARTE    108
#define CARTE_INIZIALI  7
#define MAX_NOME        32
#define MAX_RECORD_GIOCATORI 256
#define FILE_GIOCATORI "players.csv"
#define FILE_SALVATAGGIO "salvataggio.json"
#define MAX_RIGA_CLASSIFICA 128


/*Dimensioni finestra*/
#define SCREEN_W 1280
#define SCREEN_H 720


/* Dimensioni carta */
#define CARD_W          80
#define CARD_H          120
#define CARD_RADIUS     8
#define CARD_SPACING    18   /* sovrapposizione in mano */

/* Font fallback — SDL_ttf userà il font di sistema se il file non esiste */
#define FONT_PATH       "font.ttf"

/* Frame rate */
#define TARGET_FPS      60

/* ═══════════════════════════════════════════
   COLORI SDL
═══════════════════════════════════════════ */
#define COL_BG          { 20,  90,  40, 255}   /* verde tavolo */
#define COL_BG2         { 15,  70,  30, 255}   /* zona centrale */
#define COL_ROSSO       {220,  50,  50, 255}
#define COL_GIALLO      {230, 190,  30, 255}
#define COL_VERDE       { 50, 180,  80, 255}
#define COL_BLU         { 50, 100, 210, 255}
#define COL_JOLLY       { 30,  30,  30, 255}
#define COL_WHITE       {255, 255, 255, 255}
#define COL_BLACK       {  0,   0,   0, 255}
#define COL_HIGHLIGHT   {255, 220,  60, 200}  /* carta selezionata */
#define COL_OVERLAY     {  0,   0,   0, 160}  /* overlay scuro */
#define COL_PANEL       { 10,  50,  20, 230}  /* pannello UI */
#define COL_BUTTON      { 40, 140,  60, 255}
#define COL_BUTTON_HOV  { 60, 180,  80, 255}
#define COL_CARD_BACK   { 30,  60, 160, 255}  /* retro carta */

/* ═══════════════════════════════════════════
   ENUMERAZIONI GIOCO
═══════════════════════════════════════════ */
typedef enum { ROSSO, GIALLO, VERDE, BLU, NESSUNO } Colore;

typedef enum {
    ZERO, UNO, DUE, TRE, QUATTRO, CINQUE,
    SEI, SETTE, OTTO, NOVE,
    STOP, INVERTI, DRAW_TWO,
    CHANGE_COLOR, DRAW_4
} TipoCarta;

/* ═══════════════════════════════════════════
   STRUTTURE GIOCO
═══════════════════════════════════════════ */

typedef struct {
    char nome[MAX_NOME];
    int vinte;
    int perse;
    int totali;
    float win_rate;
} PlayerRecord;


typedef struct { Colore colore; TipoCarta tipo; Colore colore_scelto; } Carta;
typedef struct { Carta carte[TOTALE_CARTE]; int num_carte; } Mano;
typedef struct { Carta carte[TOTALE_CARTE]; int num_carte; } Mazzo;
typedef struct { char nome[MAX_NOME]; Mano mano; int e_umano; } Giocatore;

typedef struct {
    Giocatore giocatori[MAX_GIOCATORI];
    int       num_giocatori;
    Mazzo     mazzo;
    Mazzo     scarti;
    Carta     carta_tavolo;
    int       turno_corrente;
    int       direzione;
} StatoPartita;

/* ═══════════════════════════════════════════
   STATI SCHERMATA
═══════════════════════════════════════════ */
typedef enum {
    SCHERMATA_MENU,
    SCHERMATA_SETUP,       /* scelta num giocatori + nome */
    SCHERMATA_GIOCO,
    SCHERMATA_SCELTA_COLORE,
    SCHERMATA_PAUSA,       /* pannello pausa ESC */
    SCHERMATA_VITTORIA,
    SCHERMATA_CLASSIFICA,
    SCHERMATA_ESCI
} Schermata;

/* ═══════════════════════════════════════════
   CONTESTO GLOBALE SDL
═══════════════════════════════════════════ */
typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font_large;
    TTF_Font     *font_med;
    TTF_Font     *font_small;
    SDL_Texture* tex_carte[5][15]; // [Colore][Valore]
    SDL_Texture* tex_retro;
    SDL_Texture *tex_sfondo_menu;
    SDL_Texture *tex_icona_plus;
    SDL_Texture *tex_icona_save;
    SDL_Texture *tex_icona_trofeo;
    SDL_Texture *tex_icona_exit;
    SDL_Texture *tex_menu_icone[4];
    SDL_Texture *tex_sfondo_tavolo;
    SDL_Texture *tex_sfondo_fine_partita;
    SDL_Texture *sfondo_setup_rosso;
    SDL_Texture *sfondo_setup_giallo;
    SDL_Texture *sfondo_setup_verde;
    SDL_Texture *sfondo_setup_blu;
    SDL_Texture *tex_pulsante_uno;
    MYSQL *db_conn;

} SDLCtx;

/* Stato animazione carta giocata */
typedef struct {
    int   attiva;
    float x, y;          /* posizione corrente */
    float tx, ty;         /* target */
    float alpha;
    int   frame;
} AnimCarta;

/* Stato UI globale */
typedef struct {
    Schermata     schermata;
    StatoPartita  partita;
    int           carta_selezionata;   /* indice in mano umano, -1 = nessuna */
    int           hover_carta;         /* carta sotto il mouse */
    int           hover_pesca;         /* mouse sopra mazzo */
    char          nome_input[MAX_NOME];
    int           nome_len;
    int           num_avversari_setup; /* 1-5 */
    AnimCarta     anim;
    char          messaggio[128];      /* messaggio temporaneo (UNO!, ecc.) */
    int           messaggio_timer;
    int           cpu_delay;           /* frame di attesa turno CPU */
    Colore        colore_scelto_jolly; /* colore scelto dall'umano per jolly */
    int           carta_da_giocare;    /* indice carta jolly in attesa colore */

    PlayerRecord records[MAX_RECORD_GIOCATORI];
    int          num_records;

    int          indice_giocatore_attivo;
    int          schermata_lista_scroll;

    char         player_input[MAX_NOME];
    int          player_input_len;
    
    int         mostra_riciclo_mazzo;

    int         scelta_sfondo_setup;

    // ── LOGICA SERRATA PULSANTE UNO ──
    int         mostra_pulsante_uno;     // 1 = visibile, 0 = nascosto
    Uint32      timer_pulsante_uno;      // Timestamp di attivazione (in millisecondi)
    int         ha_cliccato_uno;         // Flag: 1 = hai premuto UNO in tempo per QUESTA attivazione
} UIState;

/* ═══════════════════════════════════════════
   PROTOTIPI
═══════════════════════════════════════════ */
static void render_tutto(SDLCtx *ctx, UIState *ui);
static void gestisci_evento(SDL_Event *e, SDLCtx *ctx, UIState *ui);
static void carica_giocatori(UIState *ui);
static void carica_giocatori_db(SDLCtx *ctx, UIState *ui);
static void salva_giocatori_db(SDLCtx *ctx, UIState *ui);
static void salva_giocatori(UIState *ui);
static void carica_texture_carte(SDLCtx *ctx);
static void carica_risorse_menu(SDLCtx *ctx);
static void mazzo_mescola(Mazzo *mazzo);
static void ricicla_scarti(StatoPartita *p, UIState *ui);
static void ordina_classifica(PlayerRecord *arr, int n);
static void aggiungi_o_aggiorna_giocatore(UIState *ui, const char *nome);
static void aggiorna_statistiche_fine_partita(UIState *ui, const char *nome_vincitore, int num_giocatori);
static void aggiorna_stats_db(SDLCtx *ctx, const char *nome, int is_winner);
static void render_classifica(SDLCtx *ctx, UIState *ui);
static void evento_classifica(SDL_Event *e, SDLCtx *ctx, UIState *ui);
static void aggiorna_winrate(PlayerRecord *p);
static int trova_record(PlayerRecord *arr, int n, const char *nome);
static void salva_partita_json(UIState *ui);
static int carica_partita_json(UIState *ui);
static void elimina_salvataggio(void);
static void render_pausa(SDLCtx *ctx, UIState *ui);
static void evento_pausa(SDL_Event *e, SDLCtx *ctx, UIState *ui);


/* ═══════════════════════════════════════════
   UTILITY STRINGHE GIOCO
═══════════════════════════════════════════ */
static const char *tipo_str(TipoCarta t) {
    switch (t) {
        case ZERO: return "0"; case UNO:  return "1"; case DUE:  return "2";
        case TRE:  return "3"; case QUATTRO: return "4"; case CINQUE: return "5";
        case SEI:  return "6"; case SETTE: return "7"; case OTTO:  return "8";
        case NOVE: return "9";
        case STOP:         return "STOP";
        case INVERTI:      return "INV";
        case DRAW_TWO:     return "+2";
        case CHANGE_COLOR: return "W";
        case DRAW_4:       return "W+4";
        default: return "?";
    }
}

static SDL_Color colore_sdl(Colore c) {
    switch (c) {
        case ROSSO:  { SDL_Color r = {220, 50,  50, 255}; return r; }
        case GIALLO: { SDL_Color r = {230,190,  30, 255}; return r; }
        case VERDE:  { SDL_Color r = { 50,180,  80, 255}; return r; }
        case BLU:    { SDL_Color r = { 50,100, 210, 255}; return r; }
        default:     { SDL_Color r = { 30, 30,  30, 255}; return r; }
    }
}

/* ═══════════════════════════════════════════
   LOGICA GIOCO
═══════════════════════════════════════════ */
static void mano_aggiungi(Mano *m, Carta c) {
    if (m->num_carte < TOTALE_CARTE) {
        m->carte[m->num_carte++] = c;
    }
}

static void mano_rimuovi(Mano *m, int idx) {
    if (idx < 0 || idx >= m->num_carte) return;
    // Sposta tutte le carte a sinistra di una posizione
    for (int i = idx; i < m->num_carte - 1; i++) {
        m->carte[i] = m->carte[i + 1];
    }
    m->num_carte--;
}

static void mazzo_init(Mazzo *mazzo) {
    int i = 0;
    Colore colori[4] = { ROSSO, GIALLO, VERDE, BLU };
    for (int c = 0; c < 4; c++) {
        mazzo->carte[i++] = (Carta){ colori[c], ZERO, NESSUNO };
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
    for (int j = 0; j < 4; j++) {
        mazzo->carte[i++] = (Carta){ NESSUNO, CHANGE_COLOR, NESSUNO };
        mazzo->carte[i++] = (Carta){ NESSUNO, DRAW_4,       NESSUNO };
    }
    mazzo->num_carte = i;
}

static void mazzo_mescola(Mazzo *mazzo) {
    for (int i = mazzo->num_carte - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Carta t = mazzo->carte[i]; mazzo->carte[i] = mazzo->carte[j]; mazzo->carte[j] = t;
    }
}

static void ricicla_scarti(StatoPartita *p, UIState *ui) {
    if (p->scarti.num_carte <= 0) return;

    for (int i = 0; i < p->scarti.num_carte; i++) {
        p->mazzo.carte[i] = p->scarti.carte[i];
    }
    
    p->mazzo.num_carte = p->scarti.num_carte;
    p->scarti.num_carte = 0; 
    mazzo_mescola(&p->mazzo); 

    // IMPOSTA IL MESSAGGIO A SCHERMO
    strcpy(ui->messaggio, "MAZZO RICICLATO!");
    ui->messaggio_timer = 50; // Il messaggio resta per meno di 1 secondo
}

static Carta mazzo_pesca(Mazzo *mazzo) { 
    return mazzo->carte[--mazzo->num_carte]; 
}

static int carta_giocabile(Carta mano, Carta tavolo) {
    if (mano.tipo == CHANGE_COLOR || mano.tipo == DRAW_4) return 1;
    return (mano.colore == tavolo.colore || mano.tipo == tavolo.tipo);
}

/* Avvia nuova partita */
static void init_partita(UIState *ui) {
    StatoPartita *p = &ui->partita;
    p->num_giocatori = ui->num_avversari_setup + 1;

    /* Giocatore umano */
    strncpy(p->giocatori[0].nome, ui->nome_input, MAX_NOME);
    p->giocatori[0].e_umano = 1;
    p->giocatori[0].mano.num_carte = 0;

    /* CPU */
    for (int i = 1; i <= ui->num_avversari_setup; i++) {
        snprintf(p->giocatori[i].nome, MAX_NOME, "BOT %d", i);
        p->giocatori[i].e_umano = 0;
        p->giocatori[i].mano.num_carte = 0;
    }

    srand((unsigned int)time(NULL));
    mazzo_init(&p->mazzo);
    mazzo_mescola(&p->mazzo);
    p->scarti.num_carte = 0;

    for (int pg = 0; pg < p->num_giocatori; pg++)
        for (int k = 0; k < CARTE_INIZIALI; k++)
            mano_aggiungi(&p->giocatori[pg].mano, mazzo_pesca(&p->mazzo));

    do { p->carta_tavolo = mazzo_pesca(&p->mazzo); 
    }while (p->carta_tavolo.tipo == CHANGE_COLOR || p->carta_tavolo.tipo == DRAW_4);

    p->mazzo.num_carte = TOTALE_CARTE;

    p->turno_corrente = 0;
    p->direzione      = 1;

    ui->carta_selezionata  = -1;
    ui->hover_carta        = -1;
    ui->messaggio[0]       = '\0';
    ui->messaggio_timer    = 0;
    ui->cpu_delay          = 0;
    ui->carta_da_giocare   = -1;
    ui->anim.attiva        = 0;
    ui->mostra_pulsante_uno = 0;
    ui->ha_cliccato_uno    = 0;
}

/* Applica effetti carta speciale */
static void applica_effetto(UIState *ui, Carta giocata) {
    StatoPartita *p = &ui->partita;
    int next = (p->turno_corrente + p->direzione + p->num_giocatori) % p->num_giocatori;

    if (giocata.tipo == INVERTI) {
        p->direzione *= -1;
        if (p->num_giocatori == 2) {
            /* Con 2 giocatori INVERTI funziona come STOP */
            next = p->turno_corrente;
        }
    }
    if (giocata.tipo == STOP) {
        /* Salta il prossimo */
        next = (next + p->direzione + p->num_giocatori) % p->num_giocatori;
    }
    if (giocata.tipo == DRAW_TWO) {
        for (int k = 0; k < 2 && p->mazzo.num_carte > 0; k++)
            mano_aggiungi(&p->giocatori[next].mano, mazzo_pesca(&p->mazzo));
        snprintf(ui->messaggio, 128, "%s pesca 2!", p->giocatori[next].nome);
        ui->messaggio_timer = 50;
        next = (next + p->direzione + p->num_giocatori) % p->num_giocatori;
    }
    if (giocata.tipo == DRAW_4) {
        for (int k = 0; k < 4 && p->mazzo.num_carte > 0; k++)
            mano_aggiungi(&p->giocatori[next].mano, mazzo_pesca(&p->mazzo));
        snprintf(ui->messaggio, 128, "%s pesca 4!", p->giocatori[next].nome);
        ui->messaggio_timer = 50;
        next = (next + p->direzione + p->num_giocatori) % p->num_giocatori;
    }

    p->turno_corrente = next;
}

/* Turno CPU */
static void turno_cpu(UIState *ui, SDLCtx *ctx) {
    StatoPartita *p = &ui->partita;
    Giocatore    *g = &p->giocatori[p->turno_corrente];

    // CONTROLLO RICICLO PER BOT
    if (p->mazzo.num_carte == 0 && p->scarti.num_carte > 0) {
        for (int i = 0; i < p->scarti.num_carte; i++) 
            p->mazzo.carte[i] = p->scarti.carte[i];
        p->mazzo.num_carte = p->scarti.num_carte;
        p->scarti.num_carte = 0;
        mazzo_mescola(&p->mazzo);
        ui->mostra_riciclo_mazzo = 1;
    }

    int giocata = -1;
    /* Strategia: preferisci carte speciali, poi numeriche */
    for (int i = 0; i < g->mano.num_carte; i++) {
        if (carta_giocabile(g->mano.carte[i], p->carta_tavolo)) {
            if (giocata == -1) giocata = i;
            if (g->mano.carte[i].tipo >= STOP) { giocata = i; break; }
        }
    }

    if (giocata == -1) {
        /* Pesca */
        if (p->mazzo.num_carte > 0)
            mano_aggiungi(&g->mano, mazzo_pesca(&p->mazzo));
        snprintf(ui->messaggio, 128, "%s pesca una carta", g->nome);
        ui->messaggio_timer = 40;
        p->turno_corrente = (p->turno_corrente + p->direzione + p->num_giocatori) % p->num_giocatori;
    } else {
        Carta c = g->mano.carte[giocata];
        /*Aggiungi carta giocata negli scarti*/
        p->scarti.carte[p->scarti.num_carte++] = p->carta_tavolo;
        /* Per i jolly CPU sceglie il colore più frequente in mano */
        if (c.tipo == CHANGE_COLOR || c.tipo == DRAW_4) {
            int cnt[4] = {0};
            for (int i = 0; i < g->mano.num_carte; i++)
                if (g->mano.carte[i].colore != NESSUNO)
                    cnt[g->mano.carte[i].colore]++;
            Colore best = ROSSO;
            for (int col = 1; col < 4; col++)
                if (cnt[col] > cnt[best]) best = col;
            c.colore_scelto = best;
            p->carta_tavolo = c;
            /* Aggiorna il colore "attivo" simulando la scelta */
            p->carta_tavolo.colore = best;
        } else {
            p->carta_tavolo = c;
        }
        mano_rimuovi(&g->mano, giocata);

        if (g->mano.num_carte == 1) {
            snprintf(ui->messaggio, 128, "%s dice UNO!", g->nome);
            ui->messaggio_timer = 50;
        }
        if (g->mano.num_carte == 0) {
            snprintf(ui->messaggio, 128, "%s ha vinto!", g->nome);
            ui->messaggio_timer = 90;
            aggiorna_statistiche_fine_partita(ui, g->nome, p->num_giocatori);
            /* Aggiorna anche il DB MySQL */
            for (int _dbi = 0; _dbi < p->num_giocatori; _dbi++) {
                if (!p->giocatori[_dbi].e_umano) continue;
                aggiorna_stats_db(ctx, p->giocatori[_dbi].nome,
                    strcmp(p->giocatori[_dbi].nome, g->nome) == 0);
            }
            ui->schermata = SCHERMATA_VITTORIA;
            return;
        }
        applica_effetto(ui, c);
    }
}

/* ═══════════════════════════════════════════
   DISEGNO PRIMITIVI SDL2
═══════════════════════════════════════════ */

static int cmp_classifica(const void *a, const void *b) {
    const PlayerRecord *pa = (const PlayerRecord *)a;
    const PlayerRecord *pb = (const PlayerRecord *)b;

    if (pb->vinte != pa->vinte) return pb->vinte - pa->vinte;
    if (pb->win_rate > pa->win_rate) return 1;
    if (pb->win_rate < pa->win_rate) return -1;
    return strcmp(pa->nome, pb->nome);
}

static void ordina_classifica(PlayerRecord *arr, int n) {
    qsort(arr, n, sizeof(PlayerRecord), cmp_classifica);
}

/* ── Carica giocatori dal database MySQL ── */
static void carica_giocatori_db(SDLCtx *ctx, UIState *ui) {
    ui->num_records = 0;

    if (!ctx->db_conn) return;

    const char *query = "SELECT nome, partite_vinte, partite_perse, partite_giocate "
                        "FROM classifica "
                        "ORDER BY partite_vinte DESC, partite_giocate ASC";

    if (mysql_query(ctx->db_conn, query) != 0) {
        fprintf(stderr, "[DB] Errore query carica_giocatori: %s\n", mysql_error(ctx->db_conn));
        return;
    }

    MYSQL_RES *result = mysql_store_result(ctx->db_conn);
    if (!result) {
        fprintf(stderr, "[DB] Errore mysql_store_result: %s\n", mysql_error(ctx->db_conn));
        return;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != NULL) {
        if (ui->num_records >= MAX_RECORD_GIOCATORI) break;

        PlayerRecord p = {0};
        strncpy(p.nome, row[0] ? row[0] : "", MAX_NOME - 1);
        p.vinte  = row[1] ? atoi(row[1]) : 0;
        p.perse  = row[2] ? atoi(row[2]) : 0;
        p.totali = row[3] ? atoi(row[3]) : 0;
        aggiorna_winrate(&p);

        ui->records[ui->num_records++] = p;
    }

    mysql_free_result(result);
}

static void carica_giocatori(UIState *ui) {
    ui->num_records = 0;

    FILE *fp = fopen(FILE_GIOCATORI, "r");
    if (!fp) return;

    char linea[256];
    while (fgets(linea, sizeof(linea), fp)) {
        PlayerRecord p = {0};
        char *nome = strtok(linea, ";");
        char *vinte = strtok(NULL, ";");
        char *perse = strtok(NULL, ";");
        char *totali = strtok(NULL, ";");
        char *wr = strtok(NULL, ";\n");

        if (!nome || !vinte || !perse || !totali) continue;

        strncpy(p.nome, nome, MAX_NOME - 1);
        p.vinte = atoi(vinte);
        p.perse = atoi(perse);
        p.totali = atoi(totali);
        p.win_rate = wr ? (float)atof(wr) : 0.0f;
        aggiorna_winrate(&p);

        if (ui->num_records < MAX_RECORD_GIOCATORI) {
            ui->records[ui->num_records++] = p;
        }
    }

    fclose(fp);
}

/* ── Salva tutti i giocatori nel database MySQL ── */
static void salva_giocatori_db(SDLCtx *ctx, UIState *ui) {
    if (!ctx->db_conn) return;

    for (int i = 0; i < ui->num_records; i++) {
        char query[512];
        char nome_escaped[MAX_NOME * 2 + 1];
        mysql_real_escape_string(ctx->db_conn, nome_escaped,
                                 ui->records[i].nome, (unsigned long)strlen(ui->records[i].nome));

        snprintf(query, sizeof(query),
            "INSERT INTO classifica (nome, partite_vinte, partite_perse, partite_giocate) "
            "VALUES ('%s', %d, %d, %d) "
            "ON DUPLICATE KEY UPDATE "
            "partite_vinte = %d, partite_perse = %d, partite_giocate = %d",
            nome_escaped,
            ui->records[i].vinte, ui->records[i].perse, ui->records[i].totali,
            ui->records[i].vinte, ui->records[i].perse, ui->records[i].totali
        );

        if (mysql_query(ctx->db_conn, query) != 0) {
            fprintf(stderr, "[DB] Errore salvataggio giocatore '%s': %s\n",
                    ui->records[i].nome, mysql_error(ctx->db_conn));
        }
    }
}

/* ── Fallback: salva su CSV se il DB non è disponibile ── */
static void salva_giocatori(UIState *ui) {
    FILE *fp = fopen(FILE_GIOCATORI, "w");
    if (!fp) return;

    for (int i = 0; i < ui->num_records; i++) {
        fprintf(fp, "%s;%d;%d;%d;%.2f\n",
            ui->records[i].nome,
            ui->records[i].vinte,
            ui->records[i].perse,
            ui->records[i].totali,
            ui->records[i].win_rate);
    }

    fclose(fp);
}


/* ──────────────────────────────────────────────────────────────────────
   SALVATAGGIO/CARICAMENTO PARTITA IN FORMATO JSON
   ────────────────────────────────────────────────────────────────────── */
static const char *colore_str(Colore c) {
    switch (c) {
        case ROSSO:   return "ROSSO";
        case GIALLO:  return "GIALLO";
        case VERDE:   return "VERDE";
        case BLU:     return "BLU";
        default:      return "NESSUNO";
    }
}

static Colore colore_from_str(const char *s) {
    if (strcmp(s, "ROSSO")  == 0) return ROSSO;
    if (strcmp(s, "GIALLO") == 0) return GIALLO;
    if (strcmp(s, "VERDE")  == 0) return VERDE;
    if (strcmp(s, "BLU")   == 0) return BLU;
    return NESSUNO;
}

static const char *tipo_str_save(TipoCarta t) {
    switch (t) {
        case ZERO:         return "ZERO";
        case UNO:          return "UNO";
        case DUE:          return "DUE";
        case TRE:          return "TRE";
        case QUATTRO:      return "QUATTRO";
        case CINQUE:       return "CINQUE";
        case SEI:          return "SEI";
        case SETTE:        return "SETTE";
        case OTTO:         return "OTTO";
        case NOVE:         return "NOVE";
        case STOP:         return "STOP";
        case INVERTI:      return "INVERTI";
        case DRAW_TWO:     return "DRAW_TWO";
        case CHANGE_COLOR: return "CHANGE_COLOR";
        case DRAW_4:       return "DRAW_4";
        default:           return "ZERO";
    }
}

static TipoCarta tipo_from_str(const char *s) {
    if (strcmp(s, "ZERO")         == 0) return ZERO;
    if (strcmp(s, "UNO")          == 0) return UNO;
    if (strcmp(s, "DUE")          == 0) return DUE;
    if (strcmp(s, "TRE")          == 0) return TRE;
    if (strcmp(s, "QUATTRO")      == 0) return QUATTRO;
    if (strcmp(s, "CINQUE")       == 0) return CINQUE;
    if (strcmp(s, "SEI")          == 0) return SEI;
    if (strcmp(s, "SETTE")        == 0) return SETTE;
    if (strcmp(s, "OTTO")         == 0) return OTTO;
    if (strcmp(s, "NOVE")         == 0) return NOVE;
    if (strcmp(s, "STOP")         == 0) return STOP;
    if (strcmp(s, "INVERTI")      == 0) return INVERTI;
    if (strcmp(s, "DRAW_TWO")     == 0) return DRAW_TWO;
    if (strcmp(s, "CHANGE_COLOR") == 0) return CHANGE_COLOR;
    if (strcmp(s, "DRAW_4")       == 0) return DRAW_4;
    return ZERO;
}

/* Helper: find closing quote, returns pointer past it */
static const char *read_json_string(const char *p, char *buf, int bufsize) {
    if (!p || *p != '"') return p;
    p++; /* skip opening quote */
    int i = 0;
    while (*p && *p != '"' && i < bufsize - 1) {
        buf[i++] = *p++;
    }
    buf[i] = '\0';
    if (*p == '"') p++; /* skip closing quote */
    return p;
}

/* Helper: parse integer from string */
static int read_json_int(const char *p, const char **end) {
    return (int)strtol(p, (char **)end, 10);
}

static void salva_partita_json(UIState *ui) {
    StatoPartita *p = &ui->partita;
    FILE *fp = fopen(FILE_SALVATAGGIO, "w");
    if (!fp) return;

    fprintf(fp, "{\n");
    fprintf(fp, "  \"num_giocatori\": %d,\n", p->num_giocatori);
    fprintf(fp, "  \"turno_corrente\": %d,\n", p->turno_corrente);
    fprintf(fp, "  \"direzione\": %d,\n", p->direzione);
    fprintf(fp, "  \"carta_tavolo\": {\"colore\": \"%s\", \"tipo\": \"%s\", \"colore_scelto\": \"%s\"},\n",
            colore_str(p->carta_tavolo.colore), tipo_str_save(p->carta_tavolo.tipo), colore_str(p->carta_tavolo.colore_scelto));

    /* Mazzo */
    fprintf(fp, "  \"mazzo_num_carte\": %d,\n", p->mazzo.num_carte);
    fprintf(fp, "  \"mazzo\": [\n");
    for (int i = 0; i < p->mazzo.num_carte; i++) {
        fprintf(fp, "    {\"colore\": \"%s\", \"tipo\": \"%s\", \"colore_scelto\": \"%s\"}%s\n",
                colore_str(p->mazzo.carte[i].colore), tipo_str_save(p->mazzo.carte[i].tipo),
                colore_str(p->mazzo.carte[i].colore_scelto),
                (i < p->mazzo.num_carte - 1) ? "," : "");
    }
    fprintf(fp, "  ],\n");

    /* Scarti */
    fprintf(fp, "  \"scarti_num_carte\": %d,\n", p->scarti.num_carte);
    fprintf(fp, "  \"scarti\": [\n");
    for (int i = 0; i < p->scarti.num_carte; i++) {
        fprintf(fp, "    {\"colore\": \"%s\", \"tipo\": \"%s\", \"colore_scelto\": \"%s\"}%s\n",
                colore_str(p->scarti.carte[i].colore), tipo_str_save(p->scarti.carte[i].tipo),
                colore_str(p->scarti.carte[i].colore_scelto),
                (i < p->scarti.num_carte - 1) ? "," : "");
    }
    fprintf(fp, "  ],\n");

    /* Giocatori */
    fprintf(fp, "  \"giocatori\": [\n");
    for (int g = 0; g < p->num_giocatori; g++) {
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"nome\": \"%s\",\n", p->giocatori[g].nome);
        fprintf(fp, "      \"e_umano\": %d,\n", p->giocatori[g].e_umano);
        fprintf(fp, "      \"mano_num_carte\": %d,\n", p->giocatori[g].mano.num_carte);
        fprintf(fp, "      \"mano\": [\n");
        for (int i = 0; i < p->giocatori[g].mano.num_carte; i++) {
            fprintf(fp, "        {\"colore\": \"%s\", \"tipo\": \"%s\", \"colore_scelto\": \"%s\"}%s\n",
                    colore_str(p->giocatori[g].mano.carte[i].colore),
                    tipo_str_save(p->giocatori[g].mano.carte[i].tipo),
                    colore_str(p->giocatori[g].mano.carte[i].colore_scelto),
                    (i < p->giocatori[g].mano.num_carte - 1) ? "," : "");
        }
        fprintf(fp, "      ]\n");
        fprintf(fp, "    }%s\n", (g < p->num_giocatori - 1) ? "," : "");
    }
    fprintf(fp, "  ]\n");

    /* UI state extra */
    fprintf(fp, "  ,\"nome_input\": \"%s\",\n", ui->nome_input);
    fprintf(fp, "  \"num_avversari_setup\": %d\n", ui->num_avversari_setup);

    fprintf(fp, "}\n");
    fclose(fp);
}

/* Parse a card JSON object like {"colore": "ROSSO", "tipo": "UNO", "colore_scelto": "NESSUNO"} */
static Carta parse_carta_json(const char **pp) {
    Carta c = {NESSUNO, ZERO, NESSUNO};
    const char *p = *pp;
    char buf[64];

    /* Find "colore" */
    while (*p && strncmp(p, "\"colore\"", 8) != 0) p++;
    if (*p) {
        p += 8;
        while (*p && *p != '"') p++;
        p = read_json_string(p, buf, sizeof(buf));
        c.colore = colore_from_str(buf);
    }
    /* Find "tipo" */
    while (*p && strncmp(p, "\"tipo\"", 6) != 0) p++;
    if (*p) {
        p += 6;
        while (*p && *p != '"') p++;
        p = read_json_string(p, buf, sizeof(buf));
        c.tipo = tipo_from_str(buf);
    }
    /* Find "colore_scelto" */
    while (*p && strncmp(p, "\"colore_scelto\"", 15) != 0) p++;
    if (*p) {
        p += 15;
        while (*p && *p != '"') p++;
        p = read_json_string(p, buf, sizeof(buf));
        c.colore_scelto = colore_from_str(buf);
    }
    /* Find closing } */
    while (*p && *p != '}') p++;
    if (*p) p++;
    *pp = p;
    return c;
}

/* Read entire file into malloc'd buffer */
static char *read_file_contents(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf) { fclose(fp); return NULL; }
    fread(buf, 1, sz, fp);
    buf[sz] = '\0';
    fclose(fp);
    return buf;
}

static int carica_partita_json(UIState *ui) {
    char *json = read_file_contents(FILE_SALVATAGGIO);
    if (!json) return 0; /* file non esiste o errore */

    StatoPartita *p = &ui->partita;
    const char *ptr = json;

    /* num_giocatori */
    while (*ptr && strncmp(ptr, "\"num_giocatori\"", 15) != 0) ptr++;
    if (!*ptr) { free(json); return 0; }
    ptr += 15;
    while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
    p->num_giocatori = read_json_int(ptr, &ptr);

    /* turno_corrente */
    while (*ptr && strncmp(ptr, "\"turno_corrente\"", 16) != 0) ptr++;
    if (!*ptr) { free(json); return 0; }
    ptr += 16;
    while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
    p->turno_corrente = read_json_int(ptr, &ptr);

    /* direzione */
    while (*ptr && strncmp(ptr, "\"direzione\"", 11) != 0) ptr++;
    if (!*ptr) { free(json); return 0; }
    ptr += 11;
    while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
    p->direzione = read_json_int(ptr, &ptr);

    /* carta_tavolo */
    while (*ptr && strncmp(ptr, "\"carta_tavolo\"", 14) != 0) ptr++;
    if (!*ptr) { free(json); return 0; }
    ptr += 14;
    while (*ptr && *ptr != '{') ptr++;
    p->carta_tavolo = parse_carta_json(&ptr);

    /* mazzo_num_carte */
    while (*ptr && strncmp(ptr, "\"mazzo_num_carte\"", 17) != 0) ptr++;
    if (!*ptr) { free(json); return 0; }
    ptr += 17;
    while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
    p->mazzo.num_carte = read_json_int(ptr, &ptr);

    /* mazzo array */
    while (*ptr && strncmp(ptr, "\"mazzo\"", 7) != 0) ptr++;
    if (!*ptr) { free(json); return 0; }
    ptr += 7;
    while (*ptr && *ptr != '[') ptr++;
    if (*ptr) ptr++; /* skip [ */
    for (int i = 0; i < p->mazzo.num_carte; i++) {
        while (*ptr && *ptr != '{') ptr++;
        if (*ptr) {
            p->mazzo.carte[i] = parse_carta_json(&ptr);
        }
    }

    /* scarti_num_carte */
    while (*ptr && strncmp(ptr, "\"scarti_num_carte\"", 18) != 0) ptr++;
    if (!*ptr) { free(json); return 0; }
    ptr += 18;
    while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
    p->scarti.num_carte = read_json_int(ptr, &ptr);

    /* scarti array */
    while (*ptr && strncmp(ptr, "\"scarti\"", 8) != 0) ptr++;
    if (!*ptr) { free(json); return 0; }
    ptr += 8;
    while (*ptr && *ptr != '[') ptr++;
    if (*ptr) ptr++;
    for (int i = 0; i < p->scarti.num_carte; i++) {
        while (*ptr && *ptr != '{') ptr++;
        if (*ptr) {
            p->scarti.carte[i] = parse_carta_json(&ptr);
        }
    }

    /* giocatori */
    while (*ptr && strncmp(ptr, "\"giocatori\"", 11) != 0) ptr++;
    if (!*ptr) { free(json); return 0; }
    ptr += 11;
    while (*ptr && *ptr != '[') ptr++;
    if (*ptr) ptr++;

    for (int g = 0; g < p->num_giocatori; g++) {
        /* nome */
        while (*ptr && strncmp(ptr, "\"nome\"", 6) != 0) ptr++;
        if (!*ptr) { free(json); return 0; }
        ptr += 6;
        while (*ptr && *ptr != '"') ptr++;
        ptr = read_json_string(ptr, p->giocatori[g].nome, MAX_NOME);

        /* e_umano */
        while (*ptr && strncmp(ptr, "\"e_umano\"", 9) != 0) ptr++;
        if (!*ptr) { free(json); return 0; }
        ptr += 9;
        while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
        p->giocatori[g].e_umano = read_json_int(ptr, &ptr);

        /* mano_num_carte */
        while (*ptr && strncmp(ptr, "\"mano_num_carte\"", 16) != 0) ptr++;
        if (!*ptr) { free(json); return 0; }
        ptr += 16;
        while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
        p->giocatori[g].mano.num_carte = read_json_int(ptr, &ptr);

        /* mano array */
        while (*ptr && strncmp(ptr, "\"mano\"", 6) != 0) ptr++;
        if (!*ptr) { free(json); return 0; }
        ptr += 6;
        while (*ptr && *ptr != '[') ptr++;
        if (*ptr) ptr++;
        for (int i = 0; i < p->giocatori[g].mano.num_carte; i++) {
            while (*ptr && *ptr != '{') ptr++;
            if (*ptr) {
                p->giocatori[g].mano.carte[i] = parse_carta_json(&ptr);
            }
        }
    }

    /* nome_input */
    while (*ptr && strncmp(ptr, "\"nome_input\"", 12) != 0) ptr++;
    if (*ptr) {
        ptr += 12;
        while (*ptr && *ptr != '"') ptr++;
        ptr = read_json_string(ptr, ui->nome_input, MAX_NOME);
        ui->nome_len = (int)strlen(ui->nome_input);
    }

    /* num_avversari_setup */
    while (*ptr && strncmp(ptr, "\"num_avversari_setup\"", 21) != 0) ptr++;
    if (*ptr) {
        ptr += 21;
        while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
        ui->num_avversari_setup = read_json_int(ptr, &ptr);
    }

    /* Reset UI state */
    ui->carta_selezionata = -1;
    ui->hover_carta = -1;
    ui->hover_pesca = 0;
    ui->carta_da_giocare = -1;
    ui->messaggio[0] = '\0';
    ui->messaggio_timer = 0;
    ui->cpu_delay = 0;
    ui->mostra_pulsante_uno = 0;
    ui->ha_cliccato_uno = 0;
    ui->mostra_riciclo_mazzo = 0;

    free(json);

    /* Elimina il salvataggio dopo averlo caricato */
    elimina_salvataggio();

    return 1;
}

static void elimina_salvataggio(void) {
    remove(FILE_SALVATAGGIO);
}


void carica_risorse_menu(SDLCtx *ctx){
    ctx->tex_sfondo_menu = IMG_LoadTexture(ctx->renderer, "sfondo_menu.jpg");
if (!ctx->tex_sfondo_menu) {
    printf("Errore sfondo: %s\n", IMG_GetError());
}

ctx->tex_sfondo_tavolo = IMG_LoadTexture(ctx->renderer, "sfondo_tavolo.jpg");
    if (!ctx->tex_sfondo_tavolo) {
        printf("Errore caricamento sfondo tavolo (sfondo_tavolo.jpg): %s\n", IMG_GetError());
    }

    ctx->tex_sfondo_fine_partita = IMG_LoadTexture(ctx->renderer, "sfondo_fine_partita.jpg");
    if (!ctx->tex_sfondo_fine_partita) {
        printf("Errore caricamento sfondo fine partita: %s\n", IMG_GetError());
    }

    ctx->tex_pulsante_uno = IMG_LoadTexture(ctx->renderer, "pulsante_uno.png");
if (!ctx->tex_pulsante_uno) {
    printf("ERR texture: %s | SDL: %s\n", IMG_GetError(), SDL_GetError());
} else {
    SDL_SetTextureBlendMode(ctx->tex_pulsante_uno, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(ctx->tex_pulsante_uno, 255);
}

    ctx->sfondo_setup_rosso  = IMG_LoadTexture(ctx->renderer, "sfondo_setup_rosso.jpg");
    ctx->sfondo_setup_giallo = IMG_LoadTexture(ctx->renderer, "sfondo_setup_giallo.jpg");
    ctx->sfondo_setup_verde  = IMG_LoadTexture(ctx->renderer, "sfondo_setup_verde.jpg");
    ctx->sfondo_setup_blu    = IMG_LoadTexture(ctx->renderer, "sfondo_setup_blu.jpg");
}

void carica_texture_carte(SDLCtx *ctx) {
    char path[256];
    const char* nomi_colori[] = {"red", "yellow", "green", "blue", "wild"};
    
    // 1. Carica il dorso
    ctx->tex_retro = IMG_LoadTexture(ctx->renderer, "./individual/card_back/card_back.png");
    if (!ctx->tex_retro) {
        printf("ERRORE: Impossibile caricare il dorso: %s\n", IMG_GetError());
    }

    // 2. Carica le carte colorate
    for (int c = 0; c < 4; c++) {
        for (int v = 0; v <= 12; v++) {
            if (v <= 9) 
                sprintf(path, "individual/%s/%d_%s.png", nomi_colori[c], v, nomi_colori[c]);
            else if (v == 10) // STOP
                sprintf(path, "individual/%s/block_%s.png", nomi_colori[c], nomi_colori[c]);
            else if (v == 11) // INVERTI
                sprintf(path, "individual/%s/inverse_%s.png", nomi_colori[c], nomi_colori[c]);
            else if (v == 12) // +2
                sprintf(path, "individual/%s/2plus_%s.png", nomi_colori[c], nomi_colori[c]);

            ctx->tex_carte[c][v] = IMG_LoadTexture(ctx->renderer, path);
            if (!ctx->tex_carte[c][v]) {
                printf("Immagine mancante: %s\n", path);
            }
        }
    }

    // 3. Carica le Wild
    ctx->tex_carte[4][13] = IMG_LoadTexture(ctx->renderer, "individual/wild/wild_card.png");
    ctx->tex_carte[4][14] = IMG_LoadTexture(ctx->renderer, "individual/wild/4_plus.png");
}

static void aggiungi_o_aggiorna_giocatore(UIState *ui, const char *nome) {
    if (!nome || !nome[0]) return;

    int idx = trova_record(ui->records, ui->num_records, nome);
    if (idx >= 0) return;

    if (ui->num_records >= MAX_RECORD_GIOCATORI) return;

    PlayerRecord p = {0};
    strncpy(p.nome, nome, MAX_NOME - 1);
    p.vinte = 0;
    p.perse = 0;
    p.totali = 0;
    p.win_rate = 0.0f;
    ui->records[ui->num_records++] = p;
}

static void aggiorna_winrate(PlayerRecord *p) {
    p->totali = p->vinte + p->perse;
    p->win_rate = (p->totali > 0) ? ((float)p->vinte * 100.0f / (float)p->totali) : 0.0f;
}

static int trova_record(PlayerRecord *arr, int n, const char *nome) {
    for (int i = 0; i < n; i++) {
        if (strcmp(arr[i].nome, nome) == 0) {
            return i;
        }   
    }
    return -1;
}

/* ── Aggiorna statistiche nel DB per un singolo giocatore ── */
static void aggiorna_stats_db(SDLCtx *ctx, const char *nome, int is_winner) {
    if (!ctx || !ctx->db_conn) return;
    if (!nome || !nome[0]) return;

    char nome_escaped[MAX_NOME * 2 + 1];
    mysql_real_escape_string(ctx->db_conn, nome_escaped,
                             nome, (unsigned long)strlen(nome));

    char query[512];
    snprintf(query, sizeof(query),
        "INSERT INTO classifica (nome, partite_vinte, partite_perse, partite_giocate) "
        "VALUES ('%s', %d, %d, 1) "
        "ON DUPLICATE KEY UPDATE "
        "partite_vinte = partite_vinte + %d, "
        "partite_perse = partite_perse + %d, "
        "partite_giocate = partite_giocate + 1",
        nome_escaped,
        is_winner ? 1 : 0,
        is_winner ? 0 : 1,
        is_winner ? 1 : 0,
        is_winner ? 0 : 1
    );

    if (mysql_query(ctx->db_conn, query) != 0) {
        fprintf(stderr, "[DB] Errore aggiornamento statistiche '%s': %s\n",
                nome, mysql_error(ctx->db_conn));
    }
}

/* ── Aggiorna statistiche in memoria + DB + fallback CSV ── */
static void aggiorna_statistiche_fine_partita(UIState *ui, const char *nome_vincitore, int num_giocatori) {
    for (int i = 0; i < num_giocatori; i++) {
        Giocatore *g = &ui->partita.giocatori[i];
        if (!g->e_umano) continue;

        int idx = trova_record(ui->records, ui->num_records, g->nome);
        if (idx < 0) {
            if (ui->num_records >= MAX_RECORD_GIOCATORI) continue;

            idx = ui->num_records++;
            memset(&ui->records[idx], 0, sizeof(PlayerRecord));
            strncpy(ui->records[idx].nome, g->nome, MAX_NOME - 1);
            ui->records[idx].nome[MAX_NOME - 1] = '\0';
        }

        ui->records[idx].totali += 1;

        if (strcmp(g->nome, nome_vincitore) == 0) {
            ui->records[idx].vinte += 1;
        } else {
            ui->records[idx].perse += 1;
        }

        aggiorna_winrate(&ui->records[idx]);
    }

    /* Salva anche su CSV come backup */
    salva_giocatori(ui);
}

/* Riempie rettangolo con colori */
static void fill_rect(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

/* Rettangolo con bordo arrotondato (approssimato con rettangoli + cerchi) */
static void fill_rounded_rect(SDL_Renderer *r, int x, int y, int w, int h,
                              int radius, SDL_Color c) {
    if (w <= 0 || h <= 0) return;
    if (radius < 0) radius = 0;
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;

    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    SDL_Rect center = { x + radius, y, w - 2 * radius, h };
    SDL_RenderFillRect(r, &center);

    SDL_Rect left = { x, y + radius, radius, h - 2 * radius };
    SDL_Rect right = { x + w - radius, y + radius, radius, h - 2 * radius };
    SDL_RenderFillRect(r, &left);
    SDL_RenderFillRect(r, &right);

    for (int dy = 0; dy < radius; dy++) {
    int dx = (int)(sqrtf((float)(radius * radius - dy * dy)));

    int ytop = y + radius - dy - 1;
    int ybot = y + h - radius + dy;

    int xleft_start  = x + radius - dx;
    int xleft_end    = x + radius - 1;
    int xright_start  = x + w - radius;
    int xright_end   = x + w - radius + dx - 1;

    SDL_RenderDrawLine(r, xleft_start, ytop, xleft_end, ytop);
    SDL_RenderDrawLine(r, xright_start, ytop, xright_end, ytop);

    SDL_RenderDrawLine(r, xleft_start, ybot, xleft_end, ybot);
    SDL_RenderDrawLine(r, xright_start, ybot, xright_end, ybot);
}
}

/* Bordo rettangolo arrotondato */
static void draw_rounded_rect(SDL_Renderer *r, int x, int y, int w, int h,
                               int radius, SDL_Color c, int thickness) {
    if (thickness <= 0) return;
    if (w <= 0 || h <= 0) return;

    if (radius < 0) radius = 0;
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;

    fill_rounded_rect(r, x, y, w, h, radius, c);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
}

/* Testo centrato in un rettangolo */
static void draw_text_center(SDL_Renderer *r, TTF_Font *font,
                              const char *txt, SDL_Color c,
                              int cx, int cy) {
    if (!font || !txt || txt[0] == '\0') return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, txt, c);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    int tw = surf->w, th = surf->h;
    SDL_FreeSurface(surf);
    if (!tex) return;
    SDL_Rect dst = { cx - tw/2, cy - th/2, tw, th };
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

/* Testo allineato a sinistra */
static void draw_text(SDL_Renderer *r, TTF_Font *font,
                      const char *txt, SDL_Color c, int x, int y) {
    if (!font || !txt || txt[0] == '\0') return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, txt, c);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_FreeSurface(surf);
    if (!tex) return;
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}


static void render_classifica(SDLCtx *ctx, UIState *ui) {
    SDL_Renderer *r = ctx->renderer;
    SDL_Color bg      = {15, 70, 30, 255};
    SDL_Color panel   = {10, 50, 20, 230};
    SDL_Color bianco  = {255,255,255,255};
    SDL_Color giallo  = {230,190,30,255};
    SDL_Color verde   = {60,180,80,255};
    SDL_Color nero    = {0, 0, 0, 255};
    SDL_Color riga_alt = {20, 80, 40, 80};   /* colore righe alternate */
    SDL_Color oro     = {255,215,0,255};       /* primo classificato */
    SDL_Color argento = {192,192,192,255};     /* secondo classificato */
    SDL_Color bronzo  = {205,127,50,255};      /* terzo classificato */

    fill_rect(r, 0, 0, WIN_W, WIN_H, bg);
    fill_rounded_rect(r, 60, 40, WIN_W - 120, WIN_H - 80, 20, panel);

    /* Titolo con contorno */
    int sp = 3;
    draw_text_center(r, ctx->font_large, "CLASSIFICA", nero, WIN_W/2 - sp, 85);
    draw_text_center(r, ctx->font_large, "CLASSIFICA", nero, WIN_W/2 + sp, 85);
    draw_text_center(r, ctx->font_large, "CLASSIFICA", nero, WIN_W/2, 85 - sp);
    draw_text_center(r, ctx->font_large, "CLASSIFICA", nero, WIN_W/2, 85 + sp);
    draw_text_center(r, ctx->font_large, "CLASSIFICA", giallo, WIN_W/2, 85);

    /* Ordina per partite vinte (e winrate come tiebreaker) */
    ordina_classifica(ui->records, ui->num_records);

    /* Parametri tabella */
    int tbl_x  = 100;                          /* margine sinistro tabella */
    int tbl_w  = WIN_W - 200;                  /* larghezza tabella */
    int y0     = 135;                           /* y inizio header */
    int row_h  = 34;                            /* altezza riga */
    int header_h = 36;                          /* altezza header */

    /* Colonne: # | Nome | Vinte | Perse | Giocate | WinRate% */
    int col_pos[] = {tbl_x + 30, tbl_x + 100, tbl_x + 340, tbl_x + 460, tbl_x + 590, tbl_x + 770};
    const char *col_names[] = {"#", "Nome", "Vinte", "Perse", "Giocate", "Win Rate"};

    /* Header sfondo */
    fill_rounded_rect(r, tbl_x, y0, tbl_w, header_h, 6, (SDL_Color){30, 100, 50, 200});

    /* Header testi */
    for (int c = 0; c < 6; c++) {
        draw_text_center(r, ctx->font_small, col_names[c], giallo, col_pos[c], y0 + header_h/2);
    }

    /* Linea separazione header */
    SDL_SetRenderDrawColor(r, 255, 220, 60, 120);
    SDL_RenderDrawLine(r, tbl_x, y0 + header_h, tbl_x + tbl_w, y0 + header_h);

    /* Righe dati */
    int start = ui->schermata_lista_scroll;
    int max_rows = 14;
    int end = start + max_rows;
    if (end > ui->num_records) end = ui->num_records;

    for (int i = start; i < end; i++) {
        int row_idx = i - start;
        int y = y0 + header_h + row_idx * row_h;

        /* Riga alternata */
        if (row_idx % 2 == 1) {
            fill_rect(r, tbl_x, y, tbl_w, row_h, riga_alt);
        }

        /* Evidenzia i primi 3 */
        SDL_Color nome_col = bianco;
        if (i == 0) nome_col = oro;
        else if (i == 1) nome_col = argento;
        else if (i == 2) nome_col = bronzo;

        /* Calcola winrate (non salvato nel DB, calcolato al volo) */
        float wr = (ui->records[i].totali > 0)
                   ? ((float)ui->records[i].vinte * 100.0f / (float)ui->records[i].totali)
                   : 0.0f;
        ui->records[i].win_rate = wr;

        char buf[128];

        /* Posizione (#) */
        snprintf(buf, sizeof(buf), "%d", i + 1);
        draw_text_center(r, ctx->font_small, buf, nome_col, col_pos[0], y + row_h/2);

        /* Nome */
        snprintf(buf, sizeof(buf), "%s", ui->records[i].nome);
        draw_text_center(r, ctx->font_small, buf, nome_col, col_pos[1], y + row_h/2);

        /* Vinte */
        snprintf(buf, sizeof(buf), "%d", ui->records[i].vinte);
        draw_text_center(r, ctx->font_small, buf, verde, col_pos[2], y + row_h/2);

        /* Perse */
        snprintf(buf, sizeof(buf), "%d", ui->records[i].perse);
        draw_text_center(r, ctx->font_small, buf, bianco, col_pos[3], y + row_h/2);

        /* Giocate */
        snprintf(buf, sizeof(buf), "%d", ui->records[i].totali);
        draw_text_center(r, ctx->font_small, buf, bianco, col_pos[4], y + row_h/2);

        /* Win Rate (calcolato!) */
        snprintf(buf, sizeof(buf), "%.1f%%", wr);
        /* Colore winrate: verde se >50%, giallo se 30-50%, rosso se <30% */
        SDL_Color wr_col = bianco;
        if (wr >= 50.0f) wr_col = verde;
        else if (wr >= 30.0f) wr_col = giallo;
        else if (wr > 0.0f) wr_col = (SDL_Color){220, 50, 50, 255};
        draw_text_center(r, ctx->font_small, buf, wr_col, col_pos[5], y + row_h/2);

        /* Linea separazione riga */
        SDL_SetRenderDrawColor(r, 255, 255, 255, 25);
        SDL_RenderDrawLine(r, tbl_x + 10, y + row_h, tbl_x + tbl_w - 10, y + row_h);
    }

    /* Se non ci sono record */
    if (ui->num_records == 0) {
        draw_text_center(r, ctx->font_med, "Nessun giocatore registrato", bianco,
                         WIN_W/2, WIN_H/2);
        draw_text_center(r, ctx->font_small, "Gioca una partita per apparire in classifica!", 
                         (SDL_Color){180,180,180,255}, WIN_W/2, WIN_H/2 + 30);
    }

    /* Indicatore scroll */
    if (ui->num_records > max_rows) {
        char scroll_buf[64];
        snprintf(scroll_buf, 64, "Scroll: %d-%d di %d  (rotella mouse)",
                 start + 1, end, ui->num_records);
        draw_text_center(r, ctx->font_small, scroll_buf,
                         (SDL_Color){180,180,180,200}, WIN_W/2, WIN_H - 60);
    }

    /* Messaggio ESC con contorno */
    int spessore_esc = 2;
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) - spessore_esc, WIN_H - 35);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) + spessore_esc, WIN_H - 35);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, WIN_W/2, (WIN_H - 35) - spessore_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, WIN_W/2, (WIN_H - 35) + spessore_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) - spessore_esc, (WIN_H - 35) - spessore_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) + spessore_esc, (WIN_H - 35) - spessore_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) - spessore_esc, (WIN_H - 35) + spessore_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) + spessore_esc, (WIN_H - 35) + spessore_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", bianco, WIN_W/2, WIN_H - 35);
}

/* ═══════════════════════════════════════════
   DISEGNO CARTA
═══════════════════════════════════════════ */
static void disegna_carta_png(SDLCtx *ctx, Carta c, int x, int y, int scoperta) {
    SDL_Rect rect = { x, y, 90, 140 }; // CARD_W e CARD_H
    SDL_Texture* t = NULL;

    if (!scoperta) {
        t = ctx->tex_retro;
    } else {
        int col = (int)c.colore;
        int tipo = (int)c.tipo;
        if (c.tipo == CHANGE_COLOR || c.tipo == DRAW_4) {
            t = ctx->tex_carte[4][tipo];
        } else {
            t = ctx->tex_carte[col][tipo];
        }
    }

    if (t) {
        SDL_RenderCopy(ctx->renderer, t, NULL, &rect);
    } else {
        // Se l'immagine non carica, disegna un quadratino grigio di emergenza
        SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(ctx->renderer, &rect);
    }
}

/* Retro carta */
static void disegna_retro_carta(SDLCtx *ctx, int x, int y) {
    if (!ctx || !ctx->tex_retro) {
        // Fallback di sicurezza: se l'immagine sparisce, disegna un rettangolo blu
        SDL_Rect r_back = { x, y, CARD_W, CARD_H };
        SDL_SetRenderDrawColor(ctx->renderer, 30, 60, 160, 255);
        SDL_RenderFillRect(ctx->renderer, &r_back);
        return;
    }

    // Definiamo dove disegnare il dorso
    SDL_Rect rect = { x, y, CARD_W, CARD_H };

    // Disegniamo un'ombra leggera sotto (opzionale, per bellezza)
    SDL_Rect ombra = { x + 3, y + 3, CARD_W, CARD_H };
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 80);
    SDL_RenderFillRect(ctx->renderer, &ombra);

    // DISEGNA IL PNG DEL DORSO
    SDL_RenderCopy(ctx->renderer, ctx->tex_retro, NULL, &rect);
}

/* ═══════════════════════════════════════════
   DISEGNO SCHERMATA MENU
═══════════════════════════════════════════ */
void render_menu(SDLCtx *ctx, UIState *ui) {
    // 1. Sfondo a tutto schermo
    SDL_RenderCopy(ctx->renderer, ctx->tex_sfondo_menu, NULL, NULL);

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    // --- PARAMETRI DIMENSIONI (Aumentati per icone più grandi) ---
    int btn_w = 180;  // Larghezza carta
    int btn_h = 280;  // Altezza carta
    int spacing = 40; // Spazio tra le carte
    
    // Calcolo per centrare il blocco delle 4 carte nello schermo
    int totale_w = (btn_w * 4) + (spacing * 3);
    int start_x = (WIN_W - totale_w) / 2;
    int start_y = WIN_H - 380; // Alzate un po' per far stare il testo sotto

    const char* testi[] = {"Nuova Partita", "Carica Partita", "Classifica", "Esci"};

    for (int i = 0; i < 4; i++) {
        // Definiamo il rettangolo della carta
        SDL_Rect r = { start_x + i * (btn_w + spacing), start_y, btn_w, btn_h };
        
        // Effetto Hover: se il mouse è sopra, la carta si ingrandisce leggermente e si alza
        if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) {
            r.y -= 25; // Si alza di più
            r.x -= 10; // Si allarga a sinistra
            r.w += 20; // Diventa più larga
            r.h += 30; // Diventa più alta
        }

        // 2. DISEGNA DIRETTAMENTE IL PNG (Niente rettangoli sotto)
        if (ctx->tex_menu_icone[i]) {
            SDL_RenderCopy(ctx->renderer, ctx->tex_menu_icone[i], NULL, &r);
        }

        // 3. SCRITTA SOTTO (posizionata rispetto alla r originale per non muoversi col mouse)
        int testo_x = start_x + i * (btn_w + spacing) + btn_w / 2;
        int testo_y = start_y + btn_h + 40;
        
        draw_text_center(ctx->renderer, ctx->font_med, testi[i], 
                         (SDL_Color){255, 255, 255, 255}, testo_x, testo_y);
    }

    /* Messaggio temporaneo nel menu (es. "Nessuna partita salvata trovata") */
    if (ui->messaggio_timer > 0 && ui->messaggio[0]) {
        SDL_Renderer *r = ctx->renderer;
        SDL_Color msg_bg = {0, 0, 0, 180};
        int mw = 500, mh = 60;
        fill_rounded_rect(r, WIN_W/2 - mw/2, WIN_H/2 - mh/2, mw, mh, 12, msg_bg);
        SDL_Color mc = {255, 220, 60, 255};
        draw_text_center(r, ctx->font_med, ui->messaggio, mc, WIN_W/2, WIN_H/2);
    }
}

/* ═══════════════════════════════════════════
   DISEGNO SCHERMATA SETUP
═══════════════════════════════════════════ */
static void render_setup(SDLCtx *ctx, UIState *ui) {
    SDL_Renderer *r = ctx->renderer;
    
    /* ── SCELTA DELLA TEXTURE IN BASE ALLO SFONDO ESTRATTO ── */
    SDL_Texture **sfondo_ptr = NULL;
    const char *nome_jpg = NULL;
    const char *nome_png = NULL;

    switch (ui->scelta_sfondo_setup) {
        case 0: 
            sfondo_ptr = &ctx->sfondo_setup_rosso; 
            nome_jpg = "sfondo_setup_rosso.jpg"; 
            break;
        case 1: 
            sfondo_ptr = &ctx->sfondo_setup_giallo; 
            nome_jpg = "sfondo_setup_giallo.jpg";  
            break;
        case 2: 
            sfondo_ptr = &ctx->sfondo_setup_verde; 
            nome_jpg = "sfondo_setup_verde.jpg"; 
            break;
        case 3: 
            sfondo_ptr = &ctx->sfondo_setup_blu; 
            nome_jpg = "sfondo_setup_blu.jpg"; 
            break;
        default: 
            sfondo_ptr = &ctx->sfondo_setup_rosso; 
            nome_jpg = "sfondo_setup_rosso.jpg"; 
            break;
    }

    /*CARICAMENTO DINAMICO*/
    if (sfondo_ptr && *sfondo_ptr == NULL) {
        *sfondo_ptr = IMG_LoadTexture(r, nome_jpg);
    }

    /*DISEGNO SULLO SCHERMO*/
    if (sfondo_ptr && *sfondo_ptr != NULL) {
        SDL_RenderCopy(r, *sfondo_ptr, NULL, NULL);
    } else {
        /* Fallback se i file non esistono sul disco */
        SDL_Color bg = {15, 70, 30, 255};
        fill_rect(r, 0, 0, WIN_W, WIN_H, bg);
    }

    SDL_Color panel = {0, 0, 0, 120};
    fill_rounded_rect(r, WIN_W/2-220, 80, 440, 500, 16, panel);

    SDL_Color giallo  = {230,190,30,255};

    /* ── TITOLO "NUOVA PARTITA" BIANCO CON CONTORNO NERO ── */
    SDL_Color nero = {0, 0, 0, 255};
    SDL_Color bianco = {255, 255, 255, 255};
    int spessore = 3; 

    // 1. Attiviamo lo scaling del renderer (riduciamo all'82% circa)
    // Moltiplichiamo le coordinate per compensare il rimpicciolimento del sistema di assi
    SDL_RenderSetScale(r, 0.75f, 0.75f);

    // Coordinate ricalcolate per lo scaling (Dividiamo X e Y originali per 0.82)
    int scalato_x = (WIN_W / 2) / 0.75f;
    int scalato_y = 130 / 0.75f;

    // 2. Disegniamo il contorno nero nelle 8 direzioni usando il FONT_LARGE
    draw_text_center(r, ctx->font_large, "NUOVA PARTITA", nero, scalato_x - spessore, scalato_y);
    draw_text_center(r, ctx->font_large, "NUOVA PARTITA", nero, scalato_x + spessore, scalato_y);
    draw_text_center(r, ctx->font_large, "NUOVA PARTITA", nero, scalato_x, scalato_y - spessore);
    draw_text_center(r, ctx->font_large, "NUOVA PARTITA", nero, scalato_x, scalato_y + spessore);
    
    draw_text_center(r, ctx->font_large, "NUOVA PARTITA", nero, scalato_x - spessore, scalato_y - spessore);
    draw_text_center(r, ctx->font_large, "NUOVA PARTITA", nero, scalato_x + spessore, scalato_y - spessore);
    draw_text_center(r, ctx->font_large, "NUOVA PARTITA", nero, scalato_x - spessore, scalato_y + spessore);
    draw_text_center(r, ctx->font_large, "NUOVA PARTITA", nero, scalato_x + spessore, scalato_y + spessore);

    // 3. Disegniamo il testo bianco principale
    draw_text_center(r, ctx->font_large, "NUOVA PARTITA", bianco, scalato_x, scalato_y);

    // 4. RESETTIAMO lo scaling a 1.0 immediatamente, così il resto dell'interfaccia non si rimpicciolisce
    SDL_RenderSetScale(r, 1.0f, 1.0f);

    /* Nome */
    draw_text_center(r, ctx->font_med, "Il tuo nome:", bianco, WIN_W/2, 210);
    SDL_Color input_bg = {255, 255, 255, 255};
    SDL_Color bordo_input = {255, 255, 255, 255};
    fill_rounded_rect(r, WIN_W/2-150, 230, 300, 44, 8, input_bg);
    draw_rounded_rect(r, WIN_W/2-150, 230, 300, 44, 8, bordo_input, 7);

    char display_nome[MAX_NOME + 2];
    snprintf(display_nome, sizeof(display_nome), "%s_", ui->nome_input);
    draw_text_center(r, ctx->font_med, display_nome, nero, WIN_W/2, 252);

    /* Numero avversari */
    draw_text_center(r, ctx->font_med, "Avversari CPU:", bianco, WIN_W/2, 320);

    for (int i = 1; i <= MAX_AVVERSARI; i++) {
        int bx = WIN_W/2 - (MAX_AVVERSARI * 50)/2 + (i-1)*50;
        int by = 345;
        int sel = (ui->num_avversari_setup == i);
        SDL_Color bc = sel ? (SDL_Color){241,217,120,200} : (SDL_Color){0,0,0,180};
        fill_rounded_rect(r, bx, by, 40, 40, 8, bc);
        SDL_Color tc = sel ? (SDL_Color){0,0,0,255} : bianco;
        char num[4]; snprintf(num, 4, "%d", i);
        draw_text_center(r, ctx->font_med, num, tc, bx+20, by+20);
    }

    /* Pulsante Inizia */
    int mx, my; SDL_GetMouseState(&mx, &my);
    int btn_ok = (ui->nome_input[0] != '\0');
    int bx = WIN_W/2-120, by = 430, bw = 240, bh = 50;
    int hover = btn_ok && (mx>=bx && mx<=bx+bw && my>=by && my<=by+bh);

    // Se c'è l'hover, modifichiamo le dimensioni per ingrandirlo
    if (hover) {
        bw += 8;
        bh += 4;
        bx = WIN_W/2 - (bw / 2);
        by = 430 - (4 / 2); 
    }

    // Aggiunta della sfumatura bianca dietro al pulsante in caso di hover
    if (hover) {
        for (int i = 4; i > 0; i--) {
            SDL_Color glow_color = {255, 255, 255, (Uint8)(40 / i)}; 
            fill_rounded_rect(r, bx - i, by - i, bw + (i * 2), bh + (i * 2), 12, glow_color);
        }
    }

    SDL_Color bc = btn_ok
                   ? (hover ? (SDL_Color){255,232,142,200} : (SDL_Color){255,222,89,200})
                   : (SDL_Color){60,60,60,255};
                   
    fill_rounded_rect(r, bx, by, bw, bh, 10, bc);
    
    SDL_Color tc = btn_ok ? nero : (SDL_Color){120,120,120,255};

    // ── GESTIONE TESTO DINAMICO CON HOVER ──
    if (hover) {
        // Applichiamo un leggero scaling (es. +10% -> 1.10) per ingrandire la scritta
        float scala = 1.10f;
        SDL_RenderSetScale(r, scala, scala);

        // Ricalcoliamo il centro basandoci sul fattore di scala impostato
        int testo_scalato_x = (WIN_W / 2) / scala;
        int testo_scalato_y = (by + bh / 2) / scala;

        draw_text_center(r, ctx->font_med, "INIZIA", tc, testo_scalato_x, testo_scalato_y);

        // Resettiamo subito lo scale a 1.0 per non intaccare gli altri elementi grafici
        SDL_RenderSetScale(r, 1.0f, 1.0f);
    } else {
        // Disegno normale quando il mouse non è sopra il pulsante
        draw_text_center(r, ctx->font_med, "INIZIA", tc, WIN_W/2, by + bh/2);
    }

    /* Torna al menu */
    int spessore_esc = 2; // Spessore del contorno grassetto
    int pos_y_esc = 520;  // Coordinata Y aggiornata per stare subito sotto al tasto

    // 1. Disegna il contorno nero nelle 8 direzioni
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) - spessore_esc, pos_y_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) + spessore_esc, pos_y_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, WIN_W/2, pos_y_esc - spessore_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, WIN_W/2, pos_y_esc + spessore_esc);
    
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) - spessore_esc, pos_y_esc - spessore_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) + spessore_esc, pos_y_esc - spessore_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) - spessore_esc, pos_y_esc + spessore_esc);
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", nero, (WIN_W/2) + spessore_esc, pos_y_esc + spessore_esc);

    // 2. Disegna il testo principale in bianco al centro
    draw_text_center(r, ctx->font_small, "Premi ESC per tornare al menu", bianco, WIN_W/2, pos_y_esc);
}

/* ═══════════════════════════════════════════
   DISEGNO SCHERMATA GIOCO
═══════════════════════════════════════════ */

/* Calcola posizione carta nella mano umano (centrata in basso) */
static int pos_carta_mano(int idx, int totale, int *out_x, int *out_y) {
    int zona_w = WIN_W - 100;
    int passo  = (totale > 1) ? (zona_w - CARD_W) / (totale - 1) : 0;
    if (passo > CARD_W + CARD_SPACING) passo = CARD_W + CARD_SPACING;
    int tot_w = (totale > 1) ? passo * (totale - 1) + CARD_W : CARD_W;
    int start_x = WIN_W/2 - tot_w/2;
    *out_x = start_x + idx * passo;
    *out_y = WIN_H - CARD_H - 30;
    return passo;
}

static void render_gioco(SDLCtx *ctx, UIState *ui) {
    SDL_Renderer  *r = ctx->renderer;
    StatoPartita  *p = &ui->partita;
    Giocatore     *umano = &p->giocatori[0];

    /* ── Sfondo Personalizzato ── */
    if (ctx->tex_sfondo_tavolo != NULL) {
        // Disegna la tua immagine su tutta la finestra
        SDL_RenderCopy(r, ctx->tex_sfondo_tavolo, NULL, NULL);
    } else {
        // Se l'immagine non viene trovata, usa il vecchio colore verde di emergenza
        SDL_Color bg = {20, 90, 40, 255};
        fill_rect(r, 0, 0, WIN_W, WIN_H, bg);
    }

    /* ── Carte CPU in alto ── */
    int num_cpu = p->num_giocatori - 1;
    for (int ci = 0; ci < num_cpu; ci++) {
        Giocatore *cpu = &p->giocatori[ci + 1];
        /* Posiziona CPU distribuiti in alto */
        int cpu_zone_w = WIN_W - 200;
        int cpu_x = 100 + ci * cpu_zone_w / (num_cpu > 1 ? num_cpu : 1);
        int cpu_y = 20;

        /* Retri carte CPU */
        int n = cpu->mano.num_carte;
        int passo = (n > 1) ? (100 / n) + 5 : 0;
        if (passo > 18) passo = 18;
        for (int k = 0; k < n && k < 15; k++)
            disegna_retro_carta(ctx, cpu_x + k * passo, cpu_y);

        /* Nome + contatore */
        char info[64];
        int active = (p->turno_corrente == ci + 1);
        SDL_Color nc = active ? (SDL_Color){255,220,60,255} : (SDL_Color){200,220,200,255};
        snprintf(info, 64, "%s  [%d]", cpu->nome, n);
        draw_text_center(r, ctx->font_small, info, nc,
                         cpu_x + 40 + (n > 1 ? passo*(n-1)/2 : 0), cpu_y + CARD_H + 14);

        /* Indicatore turno */
        if (active) {
            SDL_Color ind = {255,220,60,200};
            fill_rounded_rect(r, cpu_x - 5, cpu_y - 5,
                              (n > 1 ? passo*(n-1) : 0) + CARD_W + 10,
                              CARD_H + 10, 6, (SDL_Color){255,220,60,40});
        }
    }

    /* ── Centro: mazzo + carta tavolo ── */
    int cx = WIN_W/2;
    int cy = WIN_H/2 - 20;

    /* Mazzo */
    disegna_retro_carta(ctx, cx - CARD_W - 30, cy - CARD_H/2);
    /* Numero carte rimanenti */
    char nmazzo[16]; 
    snprintf(nmazzo, 16, "%d CARTE", p->mazzo.num_carte);

    SDL_Color c_contorno_num = {0, 0, 0, 255};          // Nero per il contorno
    SDL_Color c_testo_num = {255, 255, 255, 255};       // Il tuo colore originale verde chiaro
    int spessore_num = 2;                               // Spessore del contorno
    
    // Salviamo le tue coordinate originali in due variabili per non modificarle
    int nx = cx - CARD_W - 30 + CARD_W/2;
    int ny = cy - CARD_H/2 + CARD_H + 12;

    /* 1. Disegna il contorno nero nelle 8 direzioni attorno alla posizione originale */
    draw_text_center(r, ctx->font_small, nmazzo, c_contorno_num, nx - spessore_num, ny);
    draw_text_center(r, ctx->font_small, nmazzo, c_contorno_num, nx + spessore_num, ny);
    draw_text_center(r, ctx->font_small, nmazzo, c_contorno_num, nx, ny - spessore_num);
    draw_text_center(r, ctx->font_small, nmazzo, c_contorno_num, nx, ny + spessore_num);
    draw_text_center(r, ctx->font_small, nmazzo, c_contorno_num, nx - spessore_num, ny - spessore_num);
    draw_text_center(r, ctx->font_small, nmazzo, c_contorno_num, nx + spessore_num, ny - spessore_num);
    draw_text_center(r, ctx->font_small, nmazzo, c_contorno_num, nx - spessore_num, ny + spessore_num);
    draw_text_center(r, ctx->font_small, nmazzo, c_contorno_num, nx + spessore_num, ny + spessore_num);

    /* 2. Disegna il numero centrale sopra il contorno */
    draw_text_center(r, ctx->font_small, nmazzo, c_testo_num, nx, ny);


    SDL_Color c_contorno_mazzo = {0, 0, 0, 255};
    SDL_Color c_testo_mazzo = {255, 255, 255, 255};
    int spessore_mazzo = 2;
    int mx_centro = cx - CARD_W - 30 + CARD_W/2;
    int my_pos = cy - CARD_H/2 + CARD_H + 34; // Posizione originale mantenuta sotto il numero carte

    draw_text_center(r, ctx->font_small, "MAZZO", c_contorno_mazzo, mx_centro - spessore_mazzo, my_pos);
    draw_text_center(r, ctx->font_small, "MAZZO", c_contorno_mazzo, mx_centro + spessore_mazzo, my_pos);
    draw_text_center(r, ctx->font_small, "MAZZO", c_contorno_mazzo, mx_centro, my_pos - spessore_mazzo);
    draw_text_center(r, ctx->font_small, "MAZZO", c_contorno_mazzo, mx_centro, my_pos + spessore_mazzo);
    draw_text_center(r, ctx->font_small, "MAZZO", c_contorno_mazzo, mx_centro - spessore_mazzo, my_pos - spessore_mazzo);
    draw_text_center(r, ctx->font_small, "MAZZO", c_contorno_mazzo, mx_centro + spessore_mazzo, my_pos - spessore_mazzo);
    draw_text_center(r, ctx->font_small, "MAZZO", c_contorno_mazzo, mx_centro - spessore_mazzo, my_pos + spessore_mazzo);
    draw_text_center(r, ctx->font_small, "MAZZO", c_contorno_mazzo, mx_centro + spessore_mazzo, my_pos + spessore_mazzo);
    draw_text_center(r, ctx->font_small, "MAZZO", c_testo_mazzo, mx_centro, my_pos);

    /* Evidenzia mazzo se hover */
    if (ui->hover_pesca) {
        SDL_Color gold = {255,220,60,100};
        fill_rounded_rect(r, cx-CARD_W-34, cy-CARD_H/2-4,
                          CARD_W+8, CARD_H+8, CARD_RADIUS+2, gold);
    }

    /* Carta tavolo */
    disegna_carta_png(ctx, p->carta_tavolo, cx + 30, cy - CARD_H/2, 1);

    /* Direzione */
    SDL_Color c_contorno = {0, 0, 0, 255};          // Nero per il contorno
    SDL_Color c_testo_interno = {255,220,60,255}; // Bianco per la scritta
    int spessore = 2;                               // Spessore del contorno

    const char *testo_direzione = (p->direzione == 1) ? "-> Senso orario" : "<- Antiorario";
    int center_x = cx -42 + CARD_W / 2; // Calcola il centro esatto rispetto alla carta tavolo o alle coordinate che usi
    int center_y = cy - CARD_H / 2 + CARD_H + 60; // Posizionalo sotto la scritta "TAVOLO"

    /* 1. Disegna il contorno nero nelle 8 direzioni */
    draw_text_center(r, ctx->font_small, testo_direzione, c_contorno, center_x - spessore, center_y);
    draw_text_center(r, ctx->font_small, testo_direzione, c_contorno, center_x + spessore, center_y);
    draw_text_center(r, ctx->font_small, testo_direzione, c_contorno, center_x, center_y - spessore);
    draw_text_center(r, ctx->font_small, testo_direzione, c_contorno, center_x, center_y + spessore);
    draw_text_center(r, ctx->font_small, testo_direzione, c_contorno, center_x - spessore, center_y - spessore);
    draw_text_center(r, ctx->font_small, testo_direzione, c_contorno, center_x + spessore, center_y - spessore);
    draw_text_center(r, ctx->font_small, testo_direzione, c_contorno, center_x - spessore, center_y + spessore);
    draw_text_center(r, ctx->font_small, testo_direzione, c_contorno, center_x + spessore, center_y + spessore);

    /* 2. Disegna la scritta bianca sopra al contorno */
    draw_text_center(r, ctx->font_small, testo_direzione, c_testo_interno, center_x, center_y);

    /* ── Mano umano ── */
    Mano *mano = &umano->mano;
    int  n = mano->num_carte;

    /* Pannello mano */
    SDL_Color panel = {0, 28, 84, 120};
    fill_rect(r, 0, WIN_H - CARD_H - 60, WIN_W, CARD_H + 60, panel);

    /* Nome giocatore umano */
    int active_umano = (p->turno_corrente == 0);
    SDL_Color nc_umano = active_umano
                       ? (SDL_Color){255,220,60,255}
                       : (SDL_Color){200,220,200,255};
    char info_umano[64];
    snprintf(info_umano, 64, "%s  [%d carte]", umano->nome, n);
    draw_text(r, ctx->font_small, info_umano, nc_umano, 10, WIN_H - CARD_H - 50);

    /* Indicatore turno umano */
    if (active_umano) {
        SDL_Color freccia = {255,220,60,255};
        draw_text(r, ctx->font_small, "▲ È Il tuo turno!", freccia, 10, WIN_H - CARD_H - 30);
    }

    /* Disegna carte mano */
    for (int i = 0; i < n; i++) {
        int cx_c, cy_c;
        pos_carta_mano(i, n, &cx_c, &cy_c);

        int giocabile = active_umano && carta_giocabile(mano->carte[i], p->carta_tavolo);
        int hover     = (ui->hover_carta == i);
        int sel       = (ui->carta_selezionata == i);
        int ev        = (hover || sel) && giocabile;

        /* Solleva carta al hover/selezione */
        if (ev) {
            cy_c -= 20;
        }
        disegna_carta_png(ctx, mano->carte[i], cx_c, cy_c, 1);


        /* Indicatore giocabile */
        if (active_umano && giocabile && !hover && !sel) {
            int w = 16;        // Larghezza della base (in alto)
            int h = 12;        // Altezza del triangolo
            int dist_carta = 11; // Distanza della punta dalla cima della carta

            int centro_x = cx_c + (CARD_W / 2)+9;
            SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 0, 255); // Verde puro

            for (int j = 0; j < h; j++) {
            
            // Calcolo della larghezza della riga (decrescente)    
            int riga_w = (w * (h - j)) / h; 
            // sottraiamo metà della larghezza della riga attuale dal centro
            int x1 = centro_x - (riga_w / 2);
            int x2 = centro_x + (riga_w / 2);
            int y_pos = cy_c - dist_carta - h + j;
        
            SDL_RenderDrawLine(ctx->renderer, x1, y_pos, x2, y_pos);
        }
    }

    /* ── DISEGNO PULSANTE UNO (solo rendering, NON modifica lo stato) ── */
    if (ui->mostra_pulsante_uno && !ui->ha_cliccato_uno) {
        Uint32 tempo_trascorso = SDL_GetTicks() - ui->timer_pulsante_uno;
        
        if (tempo_trascorso <= 3000) {
            int mx_btn, my_btn;
            SDL_GetMouseState(&mx_btn, &my_btn);
            
            SDL_Rect rect_btn_uno = { 720, 550, 100, 100 }; 
            
            int is_hover = (mx_btn >= rect_btn_uno.x && mx_btn <= rect_btn_uno.x + rect_btn_uno.w && 
                            my_btn >= rect_btn_uno.y && my_btn <= rect_btn_uno.y + rect_btn_uno.h);
            
            if (is_hover) {
                rect_btn_uno.w += 10;
                rect_btn_uno.h += 6;
                rect_btn_uno.x -= 5;
                rect_btn_uno.y -= 3;
            }

            if (ctx->tex_pulsante_uno) {
                SDL_RenderCopy(ctx->renderer, ctx->tex_pulsante_uno, NULL, &rect_btn_uno);
            } else {
                // Fallback: disegna un rettangolo rosso con il testo UNO!
                SDL_Color rosso_allarme = is_hover ? (SDL_Color){240, 80, 80, 255} : (SDL_Color){220, 50, 50, 255};
                SDL_Color testo_bianco = {255, 255, 255, 255};
                fill_rounded_rect(r, rect_btn_uno.x, rect_btn_uno.y, rect_btn_uno.w, rect_btn_uno.h, 12, rosso_allarme);
                draw_text_center(r, ctx->font_med, "UNO!", testo_bianco, rect_btn_uno.x + (rect_btn_uno.w / 2), rect_btn_uno.y + (rect_btn_uno.h / 2));
            }
        }
    }

    /* ── Messaggi ── */
    if (ui->messaggio_timer > 0 && ui->messaggio[0]) {
        SDL_Color msg_bg = {0, 0, 0, 160};
        int mw = 400, mh = 50;
        fill_rounded_rect(r, WIN_W/2 - mw/2, WIN_H/2 - 80, mw, mh, 10, msg_bg);
        SDL_Color mc = {255, 220, 60, 255};
        draw_text_center(r, ctx->font_med, ui->messaggio, mc,
                         WIN_W/2, WIN_H/2 - 55);
    }

    /* ── Info turno (angolo in alto a destra) ── */
    SDL_Color panel2 = {1, 23, 67, 200};
    fill_rounded_rect(r, WIN_W - 170, 10, 160, 90, 10, panel2);
    char turno_str[64];
    snprintf(turno_str, 64, "Turno: %s",
             p->giocatori[p->turno_corrente].nome);
    draw_text_center(r, ctx->font_small, turno_str,
                     (SDL_Color){255,220,60,255}, WIN_W - 90, 35);
    snprintf(turno_str, 64, "Giocatori: %d", p->num_giocatori);
    draw_text_center(r, ctx->font_small, turno_str,
                     (SDL_Color){255,255,255,200}, WIN_W - 90, 58);

    /* ESC hint */
    SDL_Color c_contorno_esc = {0, 0, 0, 255};          // Nero per il contorno
    SDL_Color c_testo_esc = {255, 255, 255, 255};       // Il tuo colore originale semitrasparente
    int spessore_esc = 2;                               // Spessore del contorno
    int ex = 10;                                        // Coordinata X originale
    int ey = 10;                                        // Coordinata Y originale

    /* 1. Disegna il contorno nero nelle 8 direzioni attorno alla posizione (10, 10) */
    draw_text(r, ctx->font_small, "ESC = Pausa", c_contorno_esc, ex - spessore_esc, ey);
    draw_text(r, ctx->font_small, "ESC = Pausa", c_contorno_esc, ex + spessore_esc, ey);
    draw_text(r, ctx->font_small, "ESC = Pausa", c_contorno_esc, ex, ey - spessore_esc);
    draw_text(r, ctx->font_small, "ESC = Pausa", c_contorno_esc, ex, ey + spessore_esc);
    draw_text(r, ctx->font_small, "ESC = Pausa", c_contorno_esc, ex - spessore_esc, ey - spessore_esc);
    draw_text(r, ctx->font_small, "ESC = Pausa", c_contorno_esc, ex + spessore_esc, ey - spessore_esc);
    draw_text(r, ctx->font_small, "ESC = Pausa", c_contorno_esc, ex - spessore_esc, ey + spessore_esc);
    draw_text(r, ctx->font_small, "ESC = Pausa", c_contorno_esc, ex + spessore_esc, ey + spessore_esc);

    /* 2. Disegna la scritta principale sopra il contorno */
    draw_text(r, ctx->font_small, "ESC = Pausa", c_testo_esc, ex, ey);

    /* ── Overlay Riciclo Mazzo ── */
    if (ui->mostra_riciclo_mazzo == 1) {
        /* Sfondo oscurato */
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        fill_rect(r, 0, 0, WIN_W, WIN_H, (SDL_Color){0, 0, 0, 180});

        /* Box centrale */
        int bw = 450, bh = 200;
        int bx = WIN_W/2 - bw/2, by = WIN_H/2 - bh/2;
        fill_rounded_rect(r, bx, by, bw, bh, 20, (SDL_Color){30, 70, 40, 255});
        
        /* Bordo box */
        SDL_SetRenderDrawColor(r, 255, 220, 60, 255);
        SDL_Rect border = {bx, by, bw, bh};
        SDL_RenderDrawRect(r, &border);

        /* Testo */
        draw_text_center(r, ctx->font_med, "MAZZO ESAURITO", 
                         (SDL_Color){255, 255, 255, 255}, WIN_W/2, WIN_H/2 - 40);
        draw_text_center(r, ctx->font_small, "Gli scarti sono stati rimescolati", 
                         (SDL_Color){200, 220, 200, 255}, WIN_W/2, WIN_H/2);

        /* Bottone OK */
        SDL_Color btn_c = {255, 220, 60, 255};
        fill_rounded_rect(r, WIN_W/2 - 50, by + 140, 100, 40, 8, btn_c);
        draw_text_center(r, ctx->font_small, "OK", 
                         (SDL_Color){10, 60, 25, 255}, WIN_W/2, by + 160);
    }
}
}

/* ═══════════════════════════════════════════
   DISEGNO SCHERMATA SCELTA COLORE (jolly)
═══════════════════════════════════════════ */
static void render_scelta_colore(SDLCtx *ctx, UIState *ui) {
    SDL_Renderer *r = ctx->renderer;

    /* Overlay */
    SDL_Color ov = {0, 0, 0, 180};
    fill_rect(r, 0, 0, WIN_W, WIN_H, ov);

    SDL_Color panel = {1, 23, 67, 200};
    fill_rounded_rect(r, WIN_W/2-200, WIN_H/2-160, 400, 320, 20, panel);

    SDL_Color bianco = {255,255,255,255};
    draw_text_center(r, ctx->font_med, "Scegli un colore:", bianco,
                     WIN_W/2, WIN_H/2 - 120);

    SDL_Color colori_sdl[4] = {
        {220,50,50,255}, {230,190,30,255},
        {50,180,80,255}, {50,100,210,255}
    };
    const char *nomi[4] = {"ROSSO","GIALLO","VERDE","BLU"};
    int mx, my; SDL_GetMouseState(&mx, &my);

    for (int i = 0; i < 4; i++) {
        int bx = WIN_W/2 - 170 + i * 90;
        int by = WIN_H/2 - 70;
        int hover = (mx >= bx && mx <= bx+70 && my >= by && my <= by+70);
        SDL_Color bc = colori_sdl[i];
        if (hover) { bc.r = SDL_min(bc.r + 30, 255); bc.g = SDL_min(bc.g + 30, 255); }
        fill_rounded_rect(r, bx, by, 70, 70, 10, bc);
        if (hover) {
            SDL_Color gold = {255,220,60,255};
            draw_rounded_rect(r, bx, by, 70, 70, 10, gold, 2);
        }
        draw_text_center(r, ctx->font_small, nomi[i], bianco, bx+35, by+80);
    }
}

/* ───────────────────────────────────────────────────────────────
   DISEGNO SCHERMATA PAUSA (ESC)
   ─────────────────────────────────────────────────────────────── */
static void render_pausa(SDLCtx *ctx, UIState *ui) {
    SDL_Renderer *r = ctx->renderer;

    /* Disegna il gioco sotto */
    render_gioco(ctx, ui);

    /* Overlay scuro */
    SDL_Color ov = {0, 0, 0, 180};
    fill_rect(r, 0, 0, WIN_W, WIN_H, ov);

    /* Pannello centrale */
    int pw = 500, ph = 300;
    int px = WIN_W/2 - pw/2, py = WIN_H/2 - ph/2;
    SDL_Color panel = {20, 20, 60, 230};
    fill_rounded_rect(r, px, py, pw, ph, 20, panel);

    /* Bordo giallo */
    SDL_SetRenderDrawColor(r, 255, 220, 60, 255);
    SDL_Rect border = {px, py, pw, ph};
    SDL_RenderDrawRect(r, &border);

    /* Titolo PAUSA */
    SDL_Color giallo = {230, 190, 30, 255};
    SDL_Color bianco = {255, 255, 255, 255};
    SDL_Color nero   = {0, 0, 0, 255};
    int spessore = 3;

    draw_text_center(r, ctx->font_large, "PAUSA", nero, WIN_W/2 - spessore, py + 50);
    draw_text_center(r, ctx->font_large, "PAUSA", nero, WIN_W/2 + spessore, py + 50);
    draw_text_center(r, ctx->font_large, "PAUSA", nero, WIN_W/2, py + 50 - spessore);
    draw_text_center(r, ctx->font_large, "PAUSA", nero, WIN_W/2, py + 50 + spessore);
    draw_text_center(r, ctx->font_large, "PAUSA", giallo, WIN_W/2, py + 50);

    /* Domanda */
    draw_text_center(r, ctx->font_med, "Salvare la partita?", bianco, WIN_W/2, py + 110);

    /* Pulsanti */
    int mx, my; SDL_GetMouseState(&mx, &my);
    int bw = 180, bh = 50;
    int by = py + 170;

    /* Pulsante SALVA */
    int bx1 = WIN_W/2 - bw - 20;
    int h1 = (mx >= bx1 && mx <= bx1+bw && my >= by && my <= by+bh);
    SDL_Color bc1 = h1 ? (SDL_Color){255, 248, 154, 255} : (SDL_Color){255, 237, 0, 255};
    fill_rounded_rect(r, bx1, by, bw, bh, 10, bc1);
    draw_text_center(r, ctx->font_med, "SALVA", nero, bx1+bw/2, by+bh/2);

    /* Pulsante NON SALVARE */
    int bx2 = WIN_W/2 + 20;
    int h2 = (mx >= bx2 && mx <= bx2+bw && my >= by && my <= by+bh);
    SDL_Color bc2 = h2 ? (SDL_Color){200, 80, 80, 255} : (SDL_Color){160, 50, 50, 255};
    fill_rounded_rect(r, bx2, by, bw, bh, 10, bc2);
    draw_text_center(r, ctx->font_med, "NON SALVARE", bianco, bx2+bw/2, by+bh/2);

    /* Pulsante ANNULLA (torna al gioco) */
    int bx3 = WIN_W/2 - 80, by3 = py + 240;
    int bw3 = 160, bh3 = 40;
    int h3 = (mx >= bx3 && mx <= bx3+bw3 && my >= by3 && my <= by3+bh3);
    SDL_Color bc3 = h3 ? (SDL_Color){80, 160, 100, 255} : (SDL_Color){40, 120, 60, 255};
    fill_rounded_rect(r, bx3, by3, bw3, bh3, 8, bc3);
    draw_text_center(r, ctx->font_small, "ANNULLA", bianco, bx3+bw3/2, by3+bh3/2);
}

/* ───────────────────────────────────────────────────────────────
   GESTIONE EVENTI - PAUSA
   ─────────────────────────────────────────────────────────────── */
static void evento_pausa(SDL_Event *e, SDLCtx *ctx, UIState *ui) {
    if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE) {
        /* ESC dalla pausa = torna al gioco */
        ui->schermata = SCHERMATA_GIOCO;
        return;
    }

    if (e->type != SDL_MOUSEBUTTONDOWN || e->button.button != SDL_BUTTON_LEFT)
        return;

    int mx = e->button.x, my = e->button.y;
    int pw = 500, ph = 300;
    int px = WIN_W/2 - pw/2, py = WIN_H/2 - ph/2;
    int bw = 180, bh = 50;
    int by = py + 170;

    /* SALVA */
    int bx1 = WIN_W/2 - bw - 20;
    if (mx >= bx1 && mx <= bx1+bw && my >= by && my <= by+bh) {
        salva_partita_json(ui);
        snprintf(ui->messaggio, 128, "Partita salvata!");
        ui->messaggio_timer = 60;
        ui->schermata = SCHERMATA_MENU;
        return;
    }

    /* NON SALVARE */
    int bx2 = WIN_W/2 + 20;
    if (mx >= bx2 && mx <= bx2+bw && my >= by && my <= by+bh) {
        elimina_salvataggio();
        ui->schermata = SCHERMATA_MENU;
        return;
    }

    /* ANNULLA */
    int bx3 = WIN_W/2 - 80, by3 = py + 240;
    int bw3 = 160, bh3 = 40;
    if (mx >= bx3 && mx <= bx3+bw3 && my >= by3 && my <= by3+bh3) {
        ui->schermata = SCHERMATA_GIOCO;
        return;
    }
}


/* ═══════════════════════════════════════════
   DISEGNO SCHERMATA VITTORIA
═══════════════════════════════════════════ */
static void render_vittoria(SDLCtx *ctx, UIState *ui) {
    SDL_Renderer *r = ctx->renderer;

    /* Sfondo personalizzato fine partita */
    if (ctx->tex_sfondo_fine_partita != NULL) {
        SDL_RenderCopy(r, ctx->tex_sfondo_fine_partita, NULL, NULL);
    } else {
        SDL_Color bg = {15, 70, 30, 255};
        fill_rect(r, 0, 0, WIN_W, WIN_H, bg);
    }

    SDL_Color panel = {20, 20, 20, 170};
    fill_rounded_rect(r, WIN_W/2-250, WIN_H/2-200, 500, 400, 20, panel);

    SDL_Color giallo = {230,190,30,255};
    SDL_Color bianco = {255,255,255,255};
    SDL_Color nero = {0,0,0,255};
    /* ── SCRITTA FINE PARTITA IN GRASSETTO CON CONTORNO NERO ── */
    int spessore_titolo = 3; // Spessore del contorno per il font grande
    int pos_x_fine = WIN_W / 2;
    int pos_y_fine = WIN_H / 2 - 140;

    // 1. Disegna il contorno usando la variabile 'nero' già esistente nelle 8 direzioni
    draw_text_center(r, ctx->font_large, "FINE PARTITA!", nero, pos_x_fine - spessore_titolo, pos_y_fine);
    draw_text_center(r, ctx->font_large, "FINE PARTITA!", nero, pos_x_fine + spessore_titolo, pos_y_fine);
    draw_text_center(r, ctx->font_large, "FINE PARTITA!", nero, pos_x_fine, pos_y_fine - spessore_titolo);
    draw_text_center(r, ctx->font_large, "FINE PARTITA!", nero, pos_x_fine, pos_y_fine + spessore_titolo);
    
    draw_text_center(r, ctx->font_large, "FINE PARTITA!", nero, pos_x_fine - spessore_titolo, pos_y_fine - spessore_titolo);
    draw_text_center(r, ctx->font_large, "FINE PARTITA!", nero, pos_x_fine + spessore_titolo, pos_y_fine - spessore_titolo);
    draw_text_center(r, ctx->font_large, "FINE PARTITA!", nero, pos_x_fine - spessore_titolo, pos_y_fine + spessore_titolo);
    draw_text_center(r, ctx->font_large, "FINE PARTITA!", nero, pos_x_fine + spessore_titolo, pos_y_fine + spessore_titolo);

    // 2. Disegna il testo principale in giallo sopra al contorno
    draw_text_center(r, ctx->font_large, "FINE PARTITA!", giallo, pos_x_fine, pos_y_fine);
    draw_text_center(r, ctx->font_med, ui->messaggio, bianco, WIN_W/2, WIN_H/2-70);

    /* Punteggi */
    StatoPartita *p = &ui->partita;
    draw_text_center(r, ctx->font_small, "Carte rimaste:", bianco, WIN_W/2, WIN_H/2-20);
    for (int i = 0; i < p->num_giocatori; i++) {
        char buf[64];
        snprintf(buf, 64, "%s:  %d carte",
                 p->giocatori[i].nome, p->giocatori[i].mano.num_carte);
        SDL_Color tc = (p->giocatori[i].mano.num_carte == 0) ? giallo : bianco;
        draw_text_center(r, ctx->font_small, buf, tc,
                         WIN_W/2, WIN_H/2 + 10 + i * 24);
    }

    /* Pulsanti */
    int mx, my; SDL_GetMouseState(&mx, &my);
    int bw = 200, bh = 44;

    /* Nuova partita */
    int bx1 = WIN_W/2 - bw - 15, by = WIN_H/2 + 140;
    int h1 = (mx>=bx1 && mx<=bx1+bw && my>=by && my<=by+bh);
    fill_rounded_rect(r, bx1, by, bw, bh, 8,
                      h1 ? (SDL_Color){255,248,154,255} : (SDL_Color){255,237,0,255});
    draw_text_center(r, ctx->font_med, "RIVINCITA", nero, bx1+bw/2, by+bh/2);

    /* Menu */
    int bx2 = WIN_W/2 + 15;
    int h2 = (mx>=bx2 && mx<=bx2+bw && my>=by && my<=by+bh);
    fill_rounded_rect(r, bx2, by, bw, bh, 8,
                      h2 ? (SDL_Color){255,248,154,255} : (SDL_Color){255,237,0,255});
    draw_text_center(r, ctx->font_med, "MENU", nero, bx2+bw/2, by+bh/2);
}

/* ═══════════════════════════════════════════
   RENDER PRINCIPALE
═══════════════════════════════════════════ */
static void render_tutto(SDLCtx *ctx, UIState *ui) {
    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx->renderer);

    switch (ui->schermata) {
        case SCHERMATA_MENU:          render_menu(ctx, ui);         break;
        case SCHERMATA_SETUP:         render_setup(ctx, ui);        break;
        case SCHERMATA_GIOCO:         render_gioco(ctx, ui);        break;
        case SCHERMATA_SCELTA_COLORE:
            render_gioco(ctx, ui);
            render_scelta_colore(ctx, ui);
            break;
        case SCHERMATA_PAUSA:         render_pausa(ctx, ui);        break;
        case SCHERMATA_VITTORIA:      render_vittoria(ctx, ui);     break;
        case SCHERMATA_CLASSIFICA:
            render_classifica(ctx, ui);
            break;
        default: break;
    }

    SDL_RenderPresent(ctx->renderer);
}

/* ═══════════════════════════════════════════
   GESTIONE EVENTI — MENU
═══════════════════════════════════════════ */
static void evento_menu(SDL_Event *e, SDLCtx *ctx, UIState *ui) {
    if (ui->schermata == SCHERMATA_MENU) {
        if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
            int mx = e->button.x;
            int my = e->button.y;

            // Parametri identici a quelli del rendering
            int btn_w = 200;
            int btn_h = 320;
            int spacing = 10;
            int totale_w = (btn_w * 4) + (spacing * 3);
            int start_x = (WIN_W - totale_w) / 2;
            int start_y = WIN_H - 420;

            for (int i = 0; i < 4; i++) {
                SDL_Rect r = { start_x + i * (btn_w + spacing), start_y, btn_w, btn_h };

                // Controllo se il click è dentro la carta i-esima
                if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) {
                    
                    switch(i) {
                        case 0: // NUOVA PARTITA
                            ui->scelta_sfondo_setup = rand() % 4;
                            ui->schermata = SCHERMATA_SETUP; 
                            break;
                            
                        case 1: // CARICA PARTITA
                            if (carica_partita_json(ui)) {
                                ui->schermata = SCHERMATA_GIOCO;
                            } else {
                                snprintf(ui->messaggio, 128, "Nessuna partita salvata trovata");
                                ui->messaggio_timer = 80;
                            }
                            break;
                            
                        case 2: // CLASSIFICA
                            carica_giocatori_db(ctx, ui);
                            ui->schermata = SCHERMATA_CLASSIFICA;
                            ui->schermata_lista_scroll = 0;
                            break;
                            
                        case 3: // ESCI
                            ui->schermata = SCHERMATA_ESCI;
                            break;
                    }
                }
            }
        }
    }
}

/* ═══════════════════════════════════════════
   GESTIONE EVENTI — SETUP
═══════════════════════════════════════════ */
static void evento_setup(SDL_Event *e, UIState *ui) {
    if (e->type == SDL_KEYDOWN) {
        if (e->key.keysym.sym == SDLK_ESCAPE) {
            ui->schermata = SCHERMATA_MENU;
            return;
        }
        if (e->key.keysym.sym == SDLK_BACKSPACE && ui->nome_len > 0) {
            ui->nome_input[--ui->nome_len] = '\0';
        }
    }
    if (e->type == SDL_TEXTINPUT) {
        if (ui->nome_len < MAX_NOME - 1) {
            ui->nome_input[ui->nome_len++] = e->text.text[0];
            ui->nome_input[ui->nome_len]   = '\0';
        }
    }
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        int mx = e->button.x, my = e->button.y;

        /* Selezione numero avversari */
        for (int i = 1; i <= MAX_AVVERSARI; i++) {
            int bx = WIN_W/2 - (MAX_AVVERSARI * 50)/2 + (i-1)*50;
            int by = 345;
            if (mx >= bx && mx <= bx+40 && my >= by && my <= by+40)
                ui->num_avversari_setup = i;
        }

        /* Pulsante INIZIA */
        int bx = WIN_W/2-120, by = 430, bw = 240, bh = 50;
        if (mx >= bx && mx <= bx+bw && my >= by && my <= by+bh &&
            ui->nome_input[0] != '\0') {
            init_partita(ui);
            ui->schermata = SCHERMATA_GIOCO;
        }
    }
}

static void evento_classifica(SDL_Event *e, SDLCtx *ctx, UIState *ui) {
    if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE) {
        ui->schermata = SCHERMATA_MENU;
    }
    if (e->type == SDL_MOUSEWHEEL) {
        ui->schermata_lista_scroll -= e->wheel.y;
        if (ui->schermata_lista_scroll < 0) ui->schermata_lista_scroll = 0;
        if (ui->schermata_lista_scroll > ui->num_records - 1) ui->schermata_lista_scroll = ui->num_records - 1;
    }
}


/* ═══════════════════════════════════════════
   GESTIONE EVENTI — GIOCO
═══════════════════════════════════════════ */
static void evento_gioco(SDL_Event *e, SDLCtx *ctx, UIState *ui) {
    StatoPartita *p = &ui->partita;

    if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE) {
        ui->schermata = SCHERMATA_PAUSA;
        return;
    }

    /* Aggiorna hover */
    if (e->type == SDL_MOUSEMOTION) {
        int mx = e->motion.x, my = e->motion.y;
        ui->hover_carta = -1;
        ui->hover_pesca = 0;

        if (p->turno_corrente == 0) {
            Mano *mano = &p->giocatori[0].mano;
            int n = mano->num_carte;
            for (int i = n-1; i >= 0; i--) {
                int cx, cy; pos_carta_mano(i, n, &cx, &cy);
                if (mx >= cx && mx <= cx+CARD_W && my >= cy && my <= cy+CARD_H) {
                    ui->hover_carta = i;
                    break;
                }
            }

            

            /* Non riattivare il pulsante UNO al movimento del mouse:
               viene attivato solo quando si gioca una carta (num_carte diventa 1) */

            /* Hover mazzo */
            int mz_x = WIN_W/2 - CARD_W - 30;
            int mz_y = WIN_H/2 - 20 - CARD_H/2;
            ui->hover_pesca = (mx >= mz_x && mx <= mz_x+CARD_W &&
                               my >= mz_y && my <= mz_y+CARD_H);
            
            if (mx >= mz_x && mx <= mz_x + CARD_W && my >= mz_y && my <= mz_y + CARD_H && p->turno_corrente == 0 && ui->messaggio_timer == 0){
                ui->hover_pesca = 1;
            }else{
                ui->hover_pesca = 0;
            }
        }
    }

    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        int mx = e->button.x, my = e->button.y;

        if (p->turno_corrente != 0) return; /* non è il turno umano */

        Mano *mano = &p->giocatori[0].mano;
        int   n    = mano->num_carte;

       /* ── INTERCETTAZIONE CLICK SUL PULSANTE UNO ── */
        if (ui->mostra_pulsante_uno && !ui->ha_cliccato_uno) {
            if (mx >= 720 && mx <= 720 + 100 && my >= 550 && my <= 550 + 100) {
                ui->ha_cliccato_uno = 1;
                ui->mostra_pulsante_uno = 0;  // Scompare immediatamente
                
                snprintf(ui->messaggio, 128, "UNO!");
                ui->messaggio_timer = 50;
                return;
            }
        }

        /* Click sul mazzo = pesca */
        int mz_x = WIN_W/2 - CARD_W - 30;
        int mz_y = WIN_H/2 - 20 - CARD_H/2;

    if (mx >= mz_x && mx <= mz_x + CARD_W && my >= mz_y && my <= mz_y + CARD_H) {
        if (p->turno_corrente == 0 && ui->mostra_riciclo_mazzo == 0 && ui->carta_selezionata != (mano->num_carte - 1)) {
        /* SE IL MAZZO È VUOTO, RICICLA GLI SCARTI */
        if (p->mazzo.num_carte == 0 && p->scarti.num_carte > 0) {
    
        // 1. Reset totale del contatore mazzo prima del travaso
        p->mazzo.num_carte = 0; 

        // 2. Travaso: sposta TUTTE le carte accumulate negli scarti nel mazzo
        for (int i = 0; i < p->scarti.num_carte; i++) {
        p->mazzo.carte[i] = p->scarti.carte[i];
        }

        // 3. Il numero di carte nel mazzo ora è pari a quelle che erano negli scarti
        p->mazzo.num_carte = p->scarti.num_carte;

        // 4. Svuota gli scarti
        p->scarti.num_carte = 0;

        // 5. Mescola il nuovo mazzo
        mazzo_mescola(&p->mazzo);

        // 6. Mostra l'avviso
        ui->mostra_riciclo_mazzo = 1;
    }

        /* 2. PROCEDI CON LA PESCA (solo se ora il mazzo ha carte) */
        if (p->mazzo.num_carte > 0) {
        Carta pescata = mazzo_pesca(&p->mazzo);
        mano_aggiungi(mano, pescata);
        
        snprintf(ui->messaggio, 128, "Hai pescato una carta");
        ui->messaggio_timer = 30;
    
        
        // Se la carta non è giocabile, passa il turno, altrimenti l'umano può ancora giocare
        if (carta_giocabile(pescata, p->carta_tavolo)) {
                /* La carta è giocabile: l'umano può decidere di usarla */
                ui->carta_selezionata = mano->num_carte - 1;
                ui->hover_pesca= 0;
            } 
            else {
                /* La carta non è giocabile: PASSA IL TURNO */
                p->turno_corrente = (p->turno_corrente + p->direzione + p->num_giocatori) % p->num_giocatori;
                ui->carta_selezionata = -1;
            }
        }

        

        
    } 
}

        /* Click su una carta in mano */
        for (int i = n-1; i >= 0; i--) {
            int cx, cy; pos_carta_mano(i, n, &cx, &cy);
            if (mx >= cx && mx <= cx+CARD_W && my >= cy && my <= cy+CARD_H) {
                if (!carta_giocabile(mano->carte[i], p->carta_tavolo)) {
                    snprintf(ui->messaggio, 128, "Carta non giocabile!");
                    ui->messaggio_timer = 30;
                    return;
                }


                /* Il pulsante UNO viene attivato quando num_carte == 2 (vedi sotto),
                   non qui quando num_carte è già 1 */



                /* Carta jolly: chiede colore */
                if (mano->carte[i].tipo == CHANGE_COLOR ||
                    mano->carte[i].tipo == DRAW_4) {
                    ui->carta_da_giocare = i;
                    ui->schermata = SCHERMATA_SCELTA_COLORE;
                    return;
                }
                /* Gioca carta normale */
                Carta giocata = mano->carte[i];

                /* PRIMA di cambiare la carta sul tavolo, salva quella vecchia negli scarti */
                p->scarti.carte[p->scarti.num_carte++] = p->carta_tavolo;


                // Stai per scendere a 1 carta: attiva il pulsante UNO!
                if (mano->num_carte == 2) {
                    ui->mostra_pulsante_uno = 1;
                    ui->timer_pulsante_uno = SDL_GetTicks();
                    ui->ha_cliccato_uno = 0;  // Reset: pronto per il nuovo clic
                }


                mano_rimuovi(mano, i);
                p->carta_tavolo = giocata;
                ui->carta_selezionata = -1;

                if (mano->num_carte == 0) {
                    snprintf(ui->messaggio, 128, "%s ha vinto!", p->giocatori[0].nome);
                    ui->messaggio_timer = 100;
                    aggiorna_statistiche_fine_partita(ui, ui->partita.giocatori[0].nome, ui->partita.num_giocatori);
                    /* Aggiorna anche il DB MySQL */
                    aggiorna_stats_db(ctx, ui->partita.giocatori[0].nome, 1);
                    ui->schermata = SCHERMATA_VITTORIA;
                    return;
                }

                applica_effetto(ui, giocata);
                return;
            }
        }
    }
}

/* ═══════════════════════════════════════════
   GESTIONE EVENTI — SCELTA COLORE
═══════════════════════════════════════════ */
static void evento_scelta_colore(SDL_Event *e, SDLCtx *ctx, UIState *ui) {
    if (e->type != SDL_MOUSEBUTTONDOWN || e->button.button != SDL_BUTTON_LEFT)
        return;

    int mx = e->button.x, my = e->button.y;
    Colore mappa[4] = { ROSSO, GIALLO, VERDE, BLU };

    for (int i = 0; i < 4; i++) {
        int bx = WIN_W/2 - 170 + i * 90;
        int by = WIN_H/2 - 70;
        if (mx >= bx && mx <= bx+70 && my >= by && my <= by+70) {
            StatoPartita *p = &ui->partita;
            Mano *mano = &p->giocatori[0].mano;
            int idx = ui->carta_da_giocare;

            Carta giocata = mano->carte[idx];
            giocata.colore_scelto = mappa[i];
            giocata.colore        = mappa[i]; /* aggiorna colore attivo */
            mano_rimuovi(mano, idx);
            p->carta_tavolo = giocata;

            if (mano->num_carte == 0) {
                snprintf(ui->messaggio, 128, "%s ha vinto!", p->giocatori[0].nome);
                ui->messaggio_timer = 100;
                aggiorna_statistiche_fine_partita(ui, p->giocatori[0].nome, p->num_giocatori);
                /* Aggiorna anche il DB MySQL */
                aggiorna_stats_db(ctx, p->giocatori[0].nome, 1);
                ui->schermata = SCHERMATA_VITTORIA;
                return;
            }
            if (mano->num_carte == 1) {
                snprintf(ui->messaggio, 128, "UNO!");
                ui->messaggio_timer = 60;
            }
            applica_effetto(ui, giocata);
            ui->carta_da_giocare = -1;
            ui->carta_selezionata = -1;
            ui->schermata = SCHERMATA_GIOCO;
            return;
        }
    }
}

/* ═══════════════════════════════════════════
   GESTIONE EVENTI — VITTORIA
═══════════════════════════════════════════ */
static void evento_vittoria(SDL_Event *e, UIState *ui) {
    if (e->type != SDL_MOUSEBUTTONDOWN || e->button.button != SDL_BUTTON_LEFT)
        return;
    int mx = e->button.x, my = e->button.y;
    int bw = 200, bh = 44, by = WIN_H/2 + 140;

    /* Rivincita */
    int bx1 = WIN_W/2 - bw - 15;
    if (mx >= bx1 && mx <= bx1+bw && my >= by && my <= by+bh) {
        init_partita(ui);
        ui->schermata = SCHERMATA_GIOCO;
    }
    /* Menu */
    int bx2 = WIN_W/2 + 15;
    if (mx >= bx2 && mx <= bx2+bw && my >= by && my <= by+bh) {
        elimina_salvataggio();  /* Partita finita: rimuovi salvataggio */
        ui->schermata = SCHERMATA_MENU;
    }
}

/* ═══════════════════════════════════════════
   DISPATCHER EVENTI
═══════════════════════════════════════════ */
static void gestisci_evento(SDL_Event *e, SDLCtx *ctx, UIState *ui) {

    if (ui->mostra_riciclo_mazzo) {
        if (e->type == SDL_MOUSEBUTTONDOWN) {
            ui->mostra_riciclo_mazzo = 0;
        }
    }

    switch (ui->schermata) {
        case SCHERMATA_MENU:          evento_menu(e, ctx, ui);         break;
        case SCHERMATA_SETUP:         evento_setup(e, ui);        break;
        case SCHERMATA_GIOCO:         evento_gioco(e, ctx, ui);   break;
        case SCHERMATA_SCELTA_COLORE: evento_scelta_colore(e, ctx, ui);break;
        case SCHERMATA_PAUSA:         evento_pausa(e, ctx, ui);   break;
        case SCHERMATA_CLASSIFICA:
            evento_classifica(e, ctx, ui);
            break;
        case SCHERMATA_VITTORIA:      evento_vittoria(e, ui);     break;
        default: break;
    }
}

/* ═══════════════════════════════════════════
   CARICAMENTO FONT (con fallback)
═══════════════════════════════════════════ */
static TTF_Font *carica_font(int size) {
    /* Prova prima il file locale, poi font di sistema */
    TTF_Font *f = TTF_OpenFont(FONT_PATH, size);
    if (!f) {
        /* Fallback Linux */
        f = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", size);
    }
    if (!f) {
        /* Fallback macOS */
        f = TTF_OpenFont("/Library/Fonts/Arial Bold.ttf", size);
    }
    if (!f) {
        /* Fallback Windows */
        f = TTF_OpenFont("C:/Windows/Fonts/arialbd.ttf", size);
    }
    return f;
}

/* ═══════════════════════════════════════════
   MAIN
═══════════════════════════════════════════ */
int main(int argc, char* argv[]) {

    SDL_SetMainReady();
    /* ── Init SDL ── */
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    printf("Errore SDL_image: %s\n", IMG_GetError());
}
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    

    SDLCtx ctx = {0};
    ctx.window = SDL_CreateWindow(
        "UNO - Gioco di Carte",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    if (!ctx.window) {
        fprintf(stderr, "CreateWindow: %s\n", SDL_GetError());
        TTF_Quit(); SDL_Quit(); return 1;
    }

    ctx.renderer = SDL_CreateRenderer(ctx.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
if (!ctx.renderer) {
    ctx.renderer = SDL_CreateRenderer(ctx.window, -1, 0);
}
carica_texture_carte(&ctx);


int imgFlags = IMG_INIT_PNG;
if (!(IMG_Init(imgFlags) & imgFlags)) {
    printf("Errore SDL_image: %s\n", IMG_GetError());
}
    SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);

    /* ── Font ── */
    ctx.font_large = carica_font(64);
    ctx.font_med   = carica_font(28);
    ctx.font_small = carica_font(18);

    if (!ctx.font_large || !ctx.font_med || !ctx.font_small) {
        fprintf(stderr,
            "ATTENZIONE: Font non trovato.\n"
            "Copia un file .ttf nella cartella del progetto e rinominalo font.ttf\n"
            "Oppure installa i font DejaVu: sudo apt install fonts-dejavu\n");
        /* Non terminiamo — il gioco funziona anche senza testo */
    }


    // Tenta connessione al DB MySQL (XAMPP)
    ctx.db_conn = mysql_init(NULL);
    if (ctx.db_conn == NULL) {
        printf("[AVVISO DB] Impossibile inizializzare il connettore MySQL (memoria insufficiente?)\n");
    } else {
        // Tenta la connessione a XAMPP (Porta standard: 3306)
        if (mysql_real_connect(ctx.db_conn, "localhost", "root", "", "classifica_uno", 3306, NULL, 0) == NULL) {
            printf("[AVVISO DB] Impossibile connettersi a MySQL (XAMPP e' spento?): %s\n", mysql_error(ctx.db_conn));
            mysql_close(ctx.db_conn);
            ctx.db_conn = NULL;
        } else {
            printf("[OK] Connessione al database MySQL 'uno_gioco' riuscita con successo!\n");
        }
    }

    ctx.tex_sfondo_menu = IMG_LoadTexture(ctx.renderer, "sfondo_menu.jpg");
if (!ctx.tex_sfondo_menu) {
    printf("Errore: sfondo_menu.jpg non trovato!\n");
}

    ctx.tex_sfondo_tavolo = IMG_LoadTexture(ctx.renderer, "sfondo_tavolo.jpg");
    if (!ctx.tex_sfondo_tavolo) {
    printf("Errore: sfondo_tavolo.jpg non trovato!\n");
}

ctx.tex_pulsante_uno = IMG_LoadTexture(ctx.renderer, "pulsante_uno.png");
    if (!ctx.tex_pulsante_uno) {
    printf("Errore: pulsante_uno.png non trovato!\n");
}

ctx.tex_sfondo_fine_partita = IMG_LoadTexture(ctx.renderer, "sfondo_fine_partita.jpg");
    if (!ctx.tex_sfondo_fine_partita) {
    printf("Errore: sfondo_fine_partita.jpg non trovato!\n");
}

const char* file_icone[] = {
    "individual/menu/nuova_partita.png", 
    "individual/menu/carica_partita.png", 
    "individual/menu/classifica.png", 
    "individual/menu/esci.png"
};

for (int i = 0; i < 4; i++) {
    ctx.tex_menu_icone[i] = IMG_LoadTexture(ctx.renderer, file_icone[i]);
    if (!ctx.tex_menu_icone[i]) {
        printf("Errore caricamento icona %d: %s\n", i, IMG_GetError());
    }
}

    /* ── Stato UI ── */
    UIState ui = {0};
    ui.schermata           = SCHERMATA_MENU;
    ui.carta_selezionata   = -1;
    ui.hover_carta         = -1;
    ui.num_avversari_setup = 1;
    ui.carta_da_giocare    = -1;
    strncpy(ui.nome_input, "Giocatore", MAX_NOME);
    ui.nome_len = (int)strlen(ui.nome_input);

    SDL_StartTextInput();
    carica_giocatori(&ui);
    /* Carica dal DB se la connessione è attiva */
    if (ctx.db_conn) {
        carica_giocatori_db(&ctx, &ui);
    }

    /* ── Game loop ── */
    Uint32 last = SDL_GetTicks();
    while (ui.schermata != SCHERMATA_ESCI) {

        /* Gestione eventi */
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { ui.schermata = SCHERMATA_ESCI; break; }
            gestisci_evento(&e, &ctx, &ui);
        }

        /* Logica turno CPU */
        if (ui.schermata == SCHERMATA_GIOCO &&
            ui.partita.turno_corrente != 0) {
            ui.cpu_delay++;
            if (ui.cpu_delay >= 60) { /* aspetta 1 secondo (60 frame) */
                ui.cpu_delay = 0;
                turno_cpu(&ui, &ctx);
            }
        } else {
            ui.cpu_delay = 0;
        }

        /* Timer messaggi */
        if (ui.messaggio_timer > 0) ui.messaggio_timer--;

        /* ── VERIFICA SCADENZA TIMER "UNO" (3 SECONDI) ── */
        if (ui.schermata == SCHERMATA_GIOCO && ui.mostra_pulsante_uno && !ui.ha_cliccato_uno) {
            if (SDL_GetTicks() - ui.timer_pulsante_uno >= 3000) {
                // Tempo scaduto! Nascondi il pulsante e applica penalità
                ui.mostra_pulsante_uno = 0;  // Il pulsante scompare
                ui.ha_cliccato_uno = 0;      // Reset: pronto per la prossima volta
                
                // Pesca automaticamente 2 carte
                for (int k = 0; k < 2 && ui.partita.mazzo.num_carte > 0; k++) {
                    mano_aggiungi(&ui.partita.giocatori[0].mano, mazzo_pesca(&ui.partita.mazzo));
                }
                
                // Notifica a schermo l'infrazione
                strcpy(ui.messaggio, "Non hai detto UNO! Peschi 2 Carte");
                ui.messaggio_timer = 90; // Mostra l'avviso per circa 1.5 secondi
                
                // Passa il turno forzatamente per aver fallito o continua il flusso
                ui.partita.turno_corrente = (ui.partita.turno_corrente + ui.partita.direzione + ui.partita.num_giocatori) % ui.partita.num_giocatori;
            }
        }

        /* Rendering */
        render_tutto(&ctx, &ui);

        /* Cap FPS */
        Uint32 now = SDL_GetTicks();
        Uint32 elapsed = now - last;
        if (elapsed < 1000 / TARGET_FPS)
            SDL_Delay(1000 / TARGET_FPS - elapsed);
        last = SDL_GetTicks();
    }

    /* ── Cleanup ── */
    SDL_StopTextInput();
    if (ctx.font_large) TTF_CloseFont(ctx.font_large);
    if (ctx.font_med)   TTF_CloseFont(ctx.font_med);
    if (ctx.font_small) TTF_CloseFont(ctx.font_small);
    SDL_DestroyRenderer(ctx.renderer);
    SDL_DestroyWindow(ctx.window);
    TTF_Quit();
    SDL_Quit();
    /* Salva su CSV come backup */
    salva_giocatori(&ui);
    /* Salva anche sul DB se la connessione è attiva */
    if (ctx.db_conn) {
        salva_giocatori_db(&ctx, &ui);
        /* Chiudi la connessione MySQL correttamente */
        mysql_close(ctx.db_conn);
        ctx.db_conn = NULL;
        printf("[OK] Connessione MySQL chiusa correttamente.\\n");
    }
    return 0;
}