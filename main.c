/*
 * main.c
 * Punto di ingresso del gioco UNO.
 * Inizializza SDL2, TTF, Image, il database MySQL, carica le risorse,
 * esegue il game loop e libera tutte le risorse alla chiusura.
 *
 * Compilazione (Linux/macOS):
 *   gcc main.c logica.c database.c salvataggio.c grafica.c schermate.c eventi.c \
 *       -o uno $(sdl2-config --cflags --libs) -lSDL2_ttf -lSDL2_image \
 *       $(mysql_config --cflags --libs) -lm
 *
 * Compilazione (Windows MinGW):
 *   gcc main.c logica.c database.c salvataggio.c grafica.c schermate.c eventi.c \
 *       -o uno.exe -Iinclude -Llib -lSDL2 -lSDL2_ttf -lSDL2_image -lmysql -lm
 *
 * Setup database MySQL (XAMPP):
 *   CREATE DATABASE classifica_uno;
 *   USE classifica_uno;
 *   CREATE TABLE classifica (
 *       id              INT AUTO_INCREMENT PRIMARY KEY,
 *       nome            VARCHAR(32) NOT NULL UNIQUE,
 *       partite_vinte   INT DEFAULT 0,
 *       partite_perse   INT DEFAULT 0,
 *       partite_giocate INT DEFAULT 0
 *   );
 */

#include "tipi.h"
#include "logica.h"
#include "database.h"
#include "grafica.h"
#include "schermate.h"
#include "eventi.h"
#include "salvataggio.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    SDL_SetMainReady();

    /* Inizializzazione SDL2 */
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    /* Inizializzazione SDL_image con supporto PNG e JPG */
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(img_flags) & img_flags))
        printf("Attenzione SDL_image: %s\n", IMG_GetError());

    /* Inizializzazione SDL_ttf */
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    /* Creazione finestra */
    ContestoSDL ctx = {0};
    ctx.finestra = SDL_CreateWindow(
        "UNO - Gioco di Carte",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    if (!ctx.finestra) {
        fprintf(stderr, "CreateWindow: %s\n", SDL_GetError());
        TTF_Quit(); SDL_Quit(); return 1;
    }

    /* Creazione renderer con accelerazione hardware; fallback software */
    ctx.renderer = SDL_CreateRenderer(ctx.finestra, -1,
                                      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx.renderer)
        ctx.renderer = SDL_CreateRenderer(ctx.finestra, -1, 0);

    SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);

    /* Caricamento font con fallback automatico */
    ctx.font_grande  = carica_font(64);
    ctx.font_medio   = carica_font(28);
    ctx.font_piccolo = carica_font(18);

    if (!ctx.font_grande || !ctx.font_medio || !ctx.font_piccolo)
        fprintf(stderr,
            "ATTENZIONE: Font non trovato.\n"
            "Copia un file .ttf come font.ttf nella cartella del progetto,\n"
            "oppure installa i font DejaVu: sudo apt install fonts-dejavu\n");

    /* Caricamento texture carte e risorse grafiche */
    carica_texture_carte(&ctx);
    carica_risorse_menu(&ctx);

    /* Connessione al database MySQL (XAMPP) */
    ctx.connessione_db = mysql_init(NULL);
    if (!ctx.connessione_db) {
        printf("[AVVISO DB] Impossibile inizializzare il connettore MySQL\n");
    } else {
        if (mysql_real_connect(ctx.connessione_db, "localhost", "root", "",
                               "classifica_uno", 3306, NULL, 0) == NULL) {
            printf("[AVVISO DB] Connessione a MySQL fallita (XAMPP è spento?): %s\n",
                   mysql_error(ctx.connessione_db));
            mysql_close(ctx.connessione_db);
            ctx.connessione_db = NULL;
        } else {
            printf("[OK] Connessione al database MySQL 'classifica_uno' riuscita!\n");
        }
    }

    /* Inizializzazione stato UI */
    StatoUI ui = {0};
    ui.schermata           = SCHERMATA_MENU;
    ui.carta_selezionata   = -1;
    ui.hover_carta         = -1;
    ui.num_avversari_setup = 1;
    ui.carta_da_giocare    = -1;
    strncpy(ui.nome_input, "Giocatore", MAX_NOME);
    ui.lunghezza_nome = (int)strlen(ui.nome_input);

    /* Caricamento classifica (CSV come base, poi sovrascritta dal DB se disponibile) */
    SDL_StartTextInput();
    carica_giocatori(&ui);
    if (ctx.connessione_db)
        carica_giocatori_db(&ctx, &ui);

    /* ── GAME LOOP ── */
    Uint32 ultimo_tick = SDL_GetTicks();

    while (ui.schermata != SCHERMATA_ESCI) {

        /* Gestione eventi SDL */
        SDL_Event evento;
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) {
                ui.schermata = SCHERMATA_ESCI;
                break;
            }
            gestisci_evento(&evento, &ctx, &ui);
        }

        /* Turno CPU: eseguito dopo 60 frame (~1 secondo) di attesa */
        if (ui.schermata == SCHERMATA_GIOCO && ui.partita.turno_corrente != 0) {
            ui.ritardo_cpu++;
            if (ui.ritardo_cpu >= 60) {
                ui.ritardo_cpu = 0;
                turno_cpu(&ui, &ctx);
            }
        } else {
            ui.ritardo_cpu = 0;
        }

        /* Countdown timer messaggi */
        if (ui.timer_messaggio > 0) ui.timer_messaggio--;

        /* Verifica scadenza timer pulsante UNO (3 secondi) */
        if (ui.schermata == SCHERMATA_GIOCO &&
            ui.mostra_pulsante_uno && !ui.ha_cliccato_uno) {
            if (SDL_GetTicks() - ui.timer_pulsante_uno >= 3000) {
                /* Penalità: pesca 2 carte automaticamente */
                ui.mostra_pulsante_uno = 0;
                ui.ha_cliccato_uno     = 0;
                for (int k = 0; k < 2 && ui.partita.mazzo.num_carte > 0; k++)
                    mano_aggiungi(&ui.partita.giocatori[0].mano,
                                  mazzo_pesca(&ui.partita.mazzo));
                strcpy(ui.messaggio, "Non hai detto UNO! Peschi 2 Carte");
                ui.timer_messaggio = 90;
                ui.partita.turno_corrente =
                    (ui.partita.turno_corrente + ui.partita.direzione +
                     ui.partita.num_giocatori) % ui.partita.num_giocatori;
            }
        }

        /* Rendering del frame corrente */
        render_tutto(&ctx, &ui);

        /* Limitazione FPS */
        Uint32 adesso   = SDL_GetTicks();
        Uint32 trascorso = adesso - ultimo_tick;
        if (trascorso < (Uint32)(1000 / TARGET_FPS))
            SDL_Delay(1000 / TARGET_FPS - trascorso);
        ultimo_tick = SDL_GetTicks();
    }

    /* ── CLEANUP ── */
    SDL_StopTextInput();

    if (ctx.font_grande)  TTF_CloseFont(ctx.font_grande);
    if (ctx.font_medio)   TTF_CloseFont(ctx.font_medio);
    if (ctx.font_piccolo) TTF_CloseFont(ctx.font_piccolo);

    SDL_DestroyRenderer(ctx.renderer);
    SDL_DestroyWindow(ctx.finestra);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    /* Salvataggio classifica su CSV come backup */
    salva_giocatori(&ui);

    /* Salvataggio classifica nel DB e chiusura connessione */
    if (ctx.connessione_db) {
        salva_giocatori_db(&ctx, &ui);
        mysql_close(ctx.connessione_db);
        ctx.connessione_db = NULL;
        printf("[OK] Connessione MySQL chiusa correttamente.\n");
    }

    return 0;
}
