/*
 * schermate.c
 * Rendering di tutte le schermate: menu, setup, gioco, scelta colore,
 * pausa, vittoria e classifica.
 */

#include "schermate.h"

/* ═══════════════════════════════════════════
   UTILITY POSIZIONAMENTO CARTE IN MANO
═══════════════════════════════════════════ */

/* Calcola la posizione orizzontale della carta idx distribuita in basso */
int posizione_carta_mano(int idx, int totale, int *fuori_x, int *fuori_y) {
    int zona_w = WIN_W - 100;
    int passo  = (totale > 1) ? (zona_w - CARD_W) / (totale - 1) : 0;
    if (passo > CARD_W + CARD_SPACING) passo = CARD_W + CARD_SPACING;
    int tot_w   = (totale > 1) ? passo * (totale - 1) + CARD_W : CARD_W;
    int inizio_x = WIN_W / 2 - tot_w / 2;
    *fuori_x = inizio_x + idx * passo;
    *fuori_y = WIN_H - CARD_H - 30;
    return passo;
}

/* ═══════════════════════════════════════════
   HELPER: TESTO CON CONTORNO NERO (8 DIREZIONI)
═══════════════════════════════════════════ */

/* Disegna un testo con ombra in 8 direzioni + colore principale sopra */
static void disegna_testo_contorno(SDL_Renderer *r, TTF_Font *font,
                                    const char *testo, SDL_Color colore,
                                    int cx, int cy, int sp, int centrato) {
    SDL_Color nero = {0, 0, 0, 255};
    int offset[8][2] = {
        {-sp, 0}, {sp, 0}, {0, -sp}, {0, sp},
        {-sp, -sp}, {sp, -sp}, {-sp, sp}, {sp, sp}
    };
    for (int k = 0; k < 8; k++) {
        if (centrato)
            disegna_testo_centrato(r, font, testo, nero, cx + offset[k][0], cy + offset[k][1]);
        else
            disegna_testo(r, font, testo, nero, cx + offset[k][0], cy + offset[k][1]);
    }
    if (centrato)
        disegna_testo_centrato(r, font, testo, colore, cx, cy);
    else
        disegna_testo(r, font, testo, colore, cx, cy);
}

/* ═══════════════════════════════════════════
   SCHERMATA MENU
═══════════════════════════════════════════ */

void render_menu(ContestoSDL *ctx, StatoUI *ui) {
    SDL_RenderCopy(ctx->renderer, ctx->tex_sfondo_menu, NULL, NULL);

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    /* Le 4 "carte" del menu */
    int larg_carta = 180, alto_carta = 280, spaziatura = 40;
    int totale_w = (larg_carta * 4) + (spaziatura * 3);
    int inizio_x = (WIN_W - totale_w) / 2;
    int inizio_y = WIN_H - 380;

    const char *etichette[] = {"Nuova Partita", "Carica Partita", "Classifica", "Esci"};

    for (int i = 0; i < 4; i++) {
        SDL_Rect r = { inizio_x + i * (larg_carta + spaziatura), inizio_y, larg_carta, alto_carta };

        /* Effetto hover: la carta si alza e si ingrandisce */
        if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) {
            r.y -= 25;
            r.x -= 10;
            r.w += 20;
            r.h += 30;
        }

        if (ctx->tex_menu_icone[i])
            SDL_RenderCopy(ctx->renderer, ctx->tex_menu_icone[i], NULL, &r);

        /* Etichetta fissa sotto (non si muove con l'hover) */
        int tx = inizio_x + i * (larg_carta + spaziatura) + larg_carta / 2;
        int ty = inizio_y + alto_carta + 40;
        disegna_testo_centrato(ctx->renderer, ctx->font_medio, etichette[i],
                               (SDL_Color){255, 255, 255, 255}, tx, ty);
    }

    /* Messaggio temporaneo (es. "Nessuna partita salvata") */
    if (ui->timer_messaggio > 0 && ui->messaggio[0]) {
        SDL_Color sfondo_msg = {0, 0, 0, 180};
        int mw = 500, mh = 60;
        disegna_rettangolo_arrotondato(ctx->renderer,
                                       WIN_W / 2 - mw / 2, WIN_H / 2 - mh / 2,
                                       mw, mh, 12, sfondo_msg);
        disegna_testo_centrato(ctx->renderer, ctx->font_medio, ui->messaggio,
                               (SDL_Color){255, 220, 60, 255}, WIN_W / 2, WIN_H / 2);
    }
}

/* ═══════════════════════════════════════════
   SCHERMATA SETUP
═══════════════════════════════════════════ */

void render_setup(ContestoSDL *ctx, StatoUI *ui) {
    SDL_Renderer *r = ctx->renderer;

    /* Sceglie il puntatore alla texture dello sfondo casuale */
    SDL_Texture **sfondo_ptr = NULL;
    const char  *nome_jpg   = NULL;
    switch (ui->scelta_sfondo_setup) {
        case 0: sfondo_ptr = &ctx->sfondo_setup_rosso;  nome_jpg = "sfondo_setup_rosso.jpg";  break;
        case 1: sfondo_ptr = &ctx->sfondo_setup_giallo; nome_jpg = "sfondo_setup_giallo.jpg"; break;
        case 2: sfondo_ptr = &ctx->sfondo_setup_verde;  nome_jpg = "sfondo_setup_verde.jpg";  break;
        case 3: sfondo_ptr = &ctx->sfondo_setup_blu;    nome_jpg = "sfondo_setup_blu.jpg";    break;
        default: sfondo_ptr = &ctx->sfondo_setup_rosso; nome_jpg = "sfondo_setup_rosso.jpg";  break;
    }

    /* Caricamento dinamico se la texture non è ancora in memoria */
    if (sfondo_ptr && *sfondo_ptr == NULL)
        *sfondo_ptr = IMG_LoadTexture(r, nome_jpg);

    if (sfondo_ptr && *sfondo_ptr)
        SDL_RenderCopy(r, *sfondo_ptr, NULL, NULL);
    else
        disegna_rettangolo_pieno(r, 0, 0, WIN_W, WIN_H, (SDL_Color){15, 70, 30, 255});

    /* Pannello semitrasparente centrale */
    disegna_rettangolo_arrotondato(r, WIN_W / 2 - 220, 80, 440, 500, 16,
                                   (SDL_Color){0, 0, 0, 120});

    /* Titolo "NUOVA PARTITA" ridotto al 75% con contorno */
    SDL_RenderSetScale(r, 0.75f, 0.75f);
    int scalato_x = (int)((WIN_W / 2) / 0.75f);
    int scalato_y = (int)(130 / 0.75f);
    disegna_testo_contorno(r, ctx->font_grande, "NUOVA PARTITA",
                            (SDL_Color){255, 255, 255, 255}, scalato_x, scalato_y, 3, 1);
    SDL_RenderSetScale(r, 1.0f, 1.0f);

    /* Campo nome */
    SDL_Color bianco = {255, 255, 255, 255};
    SDL_Color nero   = {0,   0,   0,   255};
    disegna_testo_centrato(r, ctx->font_medio, "Il tuo nome:", bianco, WIN_W / 2, 210);
    disegna_rettangolo_arrotondato(r, WIN_W / 2 - 150, 230, 300, 44, 8, bianco);
    disegna_bordo_arrotondato(r, WIN_W / 2 - 150, 230, 300, 44, 8, bianco, 7);

    char nome_display[MAX_NOME + 2];
    snprintf(nome_display, sizeof(nome_display), "%s_", ui->nome_input);
    disegna_testo_centrato(r, ctx->font_medio, nome_display, nero, WIN_W / 2, 252);

    /* Selezione numero di avversari */
    disegna_testo_centrato(r, ctx->font_medio, "Avversari CPU:", bianco, WIN_W / 2, 320);
    for (int i = 1; i <= MAX_AVVERSARI; i++) {
        int bx  = WIN_W / 2 - (MAX_AVVERSARI * 50) / 2 + (i - 1) * 50;
        int by  = 345;
        int sel = (ui->num_avversari_setup == i);
        SDL_Color sfondo_btn = sel ? (SDL_Color){241, 217, 120, 200} : (SDL_Color){0, 0, 0, 180};
        disegna_rettangolo_arrotondato(r, bx, by, 40, 40, 8, sfondo_btn);
        SDL_Color tc = sel ? nero : bianco;
        char num[4]; snprintf(num, 4, "%d", i);
        disegna_testo_centrato(r, ctx->font_medio, num, tc, bx + 20, by + 20);
    }

    /* Pulsante INIZIA */
    int mx, my; SDL_GetMouseState(&mx, &my);
    int abilitato = (ui->nome_input[0] != '\0');
    int bx = WIN_W / 2 - 120, by = 430, bw = 240, bh = 50;
    int hover = abilitato && (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh);

    if (hover) {
        bw += 8; bh += 4;
        bx = WIN_W / 2 - bw / 2;
        by = 430 - 2;
        /* Alone bianco al hover */
        for (int k = 4; k > 0; k--) {
            SDL_Color alone = {255, 255, 255, (Uint8)(40 / k)};
            disegna_rettangolo_arrotondato(r, bx - k, by - k, bw + k * 2, bh + k * 2, 12, alone);
        }
    }

    SDL_Color colore_btn = abilitato
        ? (hover ? (SDL_Color){255, 232, 142, 200} : (SDL_Color){255, 222,  89, 200})
        : (SDL_Color){60, 60, 60, 255};
    disegna_rettangolo_arrotondato(r, bx, by, bw, bh, 10, colore_btn);

    SDL_Color colore_txt = abilitato ? nero : (SDL_Color){120, 120, 120, 255};
    if (hover) {
        SDL_RenderSetScale(r, 1.10f, 1.10f);
        disegna_testo_centrato(r, ctx->font_medio, "INIZIA", colore_txt,
                               (int)((WIN_W / 2) / 1.10f), (int)((by + bh / 2) / 1.10f));
        SDL_RenderSetScale(r, 1.0f, 1.0f);
    } else {
        disegna_testo_centrato(r, ctx->font_medio, "INIZIA", colore_txt,
                               WIN_W / 2, by + bh / 2);
    }

    /* Hint ESC con contorno */
    disegna_testo_contorno(r, ctx->font_piccolo, "Premi ESC per tornare al menu",
                            bianco, WIN_W / 2, 520, 2, 1);
}

/* ═══════════════════════════════════════════
   SCHERMATA GIOCO
═══════════════════════════════════════════ */

void render_gioco(ContestoSDL *ctx, StatoUI *ui) {
    SDL_Renderer  *r       = ctx->renderer;
    StatoPartita  *partita = &ui->partita;
    Giocatore     *umano   = &partita->giocatori[0];

    /* Sfondo tavolo */
    if (ctx->tex_sfondo_tavolo)
        SDL_RenderCopy(r, ctx->tex_sfondo_tavolo, NULL, NULL);
    else
        disegna_rettangolo_pieno(r, 0, 0, WIN_W, WIN_H, (SDL_Color){20, 90, 40, 255});

    /* Carte dei giocatori CPU in alto */
    int num_cpu = partita->num_giocatori - 1;
    for (int ci = 0; ci < num_cpu; ci++) {
        Giocatore *cpu     = &partita->giocatori[ci + 1];
        int zona_w         = WIN_W - 200;
        int cpu_x          = 100 + ci * zona_w / (num_cpu > 1 ? num_cpu : 1);
        int cpu_y          = 20;
        int num_carte_cpu  = cpu->mano.num_carte;
        int passo          = (num_carte_cpu > 1) ? (100 / num_carte_cpu) + 5 : 0;
        if (passo > 18) passo = 18;

        for (int k = 0; k < num_carte_cpu && k < 15; k++)
            disegna_retro_carta(ctx, cpu_x + k * passo, cpu_y);

        /* Nome e contatore carte */
        char info[64];
        int  attivo = (partita->turno_corrente == ci + 1);
        SDL_Color nc = attivo ? (SDL_Color){255, 220, 60, 255} : (SDL_Color){200, 220, 200, 255};
        snprintf(info, 64, "%s  [%d]", cpu->nome, num_carte_cpu);
        disegna_testo_centrato(r, ctx->font_piccolo, info, nc,
                               cpu_x + 40 + (num_carte_cpu > 1 ? passo * (num_carte_cpu - 1) / 2 : 0),
                               cpu_y + CARD_H + 14);

        /* Evidenziazione turno attivo */
        if (attivo) {
            disegna_rettangolo_arrotondato(r, cpu_x - 5, cpu_y - 5,
                (num_carte_cpu > 1 ? passo * (num_carte_cpu - 1) : 0) + CARD_W + 10,
                CARD_H + 10, 6, (SDL_Color){255, 220, 60, 40});
        }
    }

    /* Area centrale: mazzo e carta sul tavolo */
    int cx = WIN_W / 2;
    int cy = WIN_H / 2 - 20;

    disegna_retro_carta(ctx, cx - CARD_W - 30, cy - CARD_H / 2);

    /* Numero carte nel mazzo */
    char num_mazzo[16];
    snprintf(num_mazzo, 16, "%d CARTE", partita->mazzo.num_carte);
    int nx = cx - CARD_W - 30 + CARD_W / 2;
    int ny = cy - CARD_H / 2 + CARD_H + 12;
    disegna_testo_contorno(r, ctx->font_piccolo, num_mazzo,
                            (SDL_Color){255, 255, 255, 255}, nx, ny, 2, 1);

    int mx_c = cx - CARD_W - 30 + CARD_W / 2;
    int my_c = cy - CARD_H / 2 + CARD_H + 34;
    disegna_testo_contorno(r, ctx->font_piccolo, "MAZZO",
                            (SDL_Color){255, 255, 255, 255}, mx_c, my_c, 2, 1);

    /* Evidenziazione hover sul mazzo */
    if (ui->hover_pesca)
        disegna_rettangolo_arrotondato(r, cx - CARD_W - 34, cy - CARD_H / 2 - 4,
                                       CARD_W + 8, CARD_H + 8, CARD_RADIUS + 2,
                                       (SDL_Color){255, 220, 60, 100});

    /* Carta sul tavolo */
    disegna_carta_png(ctx, partita->carta_tavolo, cx + 30, cy - CARD_H / 2, 1);

    /* Indicatore direzione gioco */
    const char *testo_dir = (partita->direzione == 1) ? "-> Senso orario" : "<- Antiorario";
    int dir_x = cx - 42 + CARD_W / 2;
    int dir_y = cy - CARD_H / 2 + CARD_H + 60;
    disegna_testo_contorno(r, ctx->font_piccolo, testo_dir,
                            (SDL_Color){255, 220, 60, 255}, dir_x, dir_y, 2, 1);

    /* Pannello mano in basso */
    disegna_rettangolo_pieno(r, 0, WIN_H - CARD_H - 60, WIN_W, CARD_H + 60,
                             (SDL_Color){0, 28, 84, 120});

    /* Nome giocatore umano e indicatore turno */
    int attivo_umano = (partita->turno_corrente == 0);
    SDL_Color nc_umano = attivo_umano
        ? (SDL_Color){255, 220, 60, 255}
        : (SDL_Color){200, 220, 200, 255};
    char info_umano[64];
    snprintf(info_umano, 64, "%s  [%d carte]", umano->nome, umano->mano.num_carte);
    disegna_testo(r, ctx->font_piccolo, info_umano, nc_umano, 10, WIN_H - CARD_H - 50);

    if (attivo_umano)
        disegna_testo(r, ctx->font_piccolo, "▲ È Il tuo turno!",
                      (SDL_Color){255, 220, 60, 255}, 10, WIN_H - CARD_H - 30);

    /* Carte della mano umano */
    int num_carte = umano->mano.num_carte;
    for (int i = 0; i < num_carte; i++) {
        int cx_c, cy_c;
        posizione_carta_mano(i, num_carte, &cx_c, &cy_c);

        int giocabile = attivo_umano && carta_giocabile(umano->mano.carte[i], partita->carta_tavolo);
        int hov       = (ui->hover_carta == i);
        int sel       = (ui->carta_selezionata == i);

        if ((hov || sel) && giocabile) cy_c -= 20;
        disegna_carta_png(ctx, umano->mano.carte[i], cx_c, cy_c, 1);

        /* Triangolo verde sopra le carte giocabili */
        if (attivo_umano && giocabile && !hov && !sel) {
            int base_w = 16, alt_t = 12, dist = 11;
            int centro_x = cx_c + (CARD_W / 2) + 9;
            SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 0, 255);
            for (int j = 0; j < alt_t; j++) {
                int riga_w = (base_w * (alt_t - j)) / alt_t;
                int x1 = centro_x - riga_w / 2;
                int x2 = centro_x + riga_w / 2;
                SDL_RenderDrawLine(ctx->renderer, x1, cy_c - dist - alt_t + j,
                                                  x2, cy_c - dist - alt_t + j);
            }
        }
    }

    /* Pulsante UNO (3 secondi di tempo) */
    if (ui->mostra_pulsante_uno && !ui->ha_cliccato_uno) {
        Uint32 trascorso = SDL_GetTicks() - ui->timer_pulsante_uno;
        if (trascorso <= 3000) {
            int mx_b, my_b;
            SDL_GetMouseState(&mx_b, &my_b);
            SDL_Rect rect_uno = {720, 550, 100, 100};

            int hover_uno = (mx_b >= rect_uno.x && mx_b <= rect_uno.x + rect_uno.w &&
                             my_b >= rect_uno.y && my_b <= rect_uno.y + rect_uno.h);
            if (hover_uno) {
                rect_uno.w += 10; rect_uno.h += 6;
                rect_uno.x -= 5;  rect_uno.y -= 3;
            }

            if (ctx->tex_pulsante_uno) {
                SDL_RenderCopy(ctx->renderer, ctx->tex_pulsante_uno, NULL, &rect_uno);
            } else {
                SDL_Color rosso = hover_uno ? (SDL_Color){240, 80, 80, 255}
                                            : (SDL_Color){220, 50, 50, 255};
                disegna_rettangolo_arrotondato(r, rect_uno.x, rect_uno.y,
                                               rect_uno.w, rect_uno.h, 12, rosso);
                disegna_testo_centrato(r, ctx->font_medio, "UNO!",
                    (SDL_Color){255, 255, 255, 255},
                    rect_uno.x + rect_uno.w / 2, rect_uno.y + rect_uno.h / 2);
            }
        }
    }

    /* Messaggio temporaneo */
    if (ui->timer_messaggio > 0 && ui->messaggio[0]) {
        disegna_rettangolo_arrotondato(r, WIN_W / 2 - 200, WIN_H / 2 - 80,
                                       400, 50, 10, (SDL_Color){0, 0, 0, 160});
        disegna_testo_centrato(r, ctx->font_medio, ui->messaggio,
                               (SDL_Color){255, 220, 60, 255}, WIN_W / 2, WIN_H / 2 - 55);
    }

    /* Pannello info turno (angolo alto-destra) */
    disegna_rettangolo_arrotondato(r, WIN_W - 170, 10, 160, 90, 10,
                                   (SDL_Color){1, 23, 67, 200});
    char buf_turno[64];
    snprintf(buf_turno, 64, "Turno: %s", partita->giocatori[partita->turno_corrente].nome);
    disegna_testo_centrato(r, ctx->font_piccolo, buf_turno,
                           (SDL_Color){255, 220, 60, 255}, WIN_W - 90, 35);
    snprintf(buf_turno, 64, "Giocatori: %d", partita->num_giocatori);
    disegna_testo_centrato(r, ctx->font_piccolo, buf_turno,
                           (SDL_Color){255, 255, 255, 200}, WIN_W - 90, 58);

    /* Hint ESC in alto a sinistra */
    disegna_testo_contorno(r, ctx->font_piccolo, "ESC = Pausa",
                            (SDL_Color){255, 255, 255, 255}, 10, 10, 2, 0);

    /* Overlay riciclo mazzo */
    if (ui->mostra_riciclo_mazzo == 1) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        disegna_rettangolo_pieno(r, 0, 0, WIN_W, WIN_H, (SDL_Color){0, 0, 0, 180});

        int bw = 450, bh = 200;
        int bx = WIN_W / 2 - bw / 2, by = WIN_H / 2 - bh / 2;
        disegna_rettangolo_arrotondato(r, bx, by, bw, bh, 20, (SDL_Color){30, 70, 40, 255});

        SDL_SetRenderDrawColor(r, 255, 220, 60, 255);
        SDL_Rect bordo = {bx, by, bw, bh};
        SDL_RenderDrawRect(r, &bordo);

        disegna_testo_centrato(r, ctx->font_medio, "MAZZO ESAURITO",
                               (SDL_Color){255, 255, 255, 255}, WIN_W / 2, WIN_H / 2 - 40);
        disegna_testo_centrato(r, ctx->font_piccolo, "Gli scarti sono stati rimescolati",
                               (SDL_Color){200, 220, 200, 255}, WIN_W / 2, WIN_H / 2);

        disegna_rettangolo_arrotondato(r, WIN_W / 2 - 50, by + 140, 100, 40, 8,
                                       (SDL_Color){255, 220, 60, 255});
        disegna_testo_centrato(r, ctx->font_piccolo, "OK",
                               (SDL_Color){10, 60, 25, 255}, WIN_W / 2, by + 160);
    }
}

/* ═══════════════════════════════════════════
   SCHERMATA SCELTA COLORE (jolly)
═══════════════════════════════════════════ */

void render_scelta_colore(ContestoSDL *ctx, StatoUI *ui) {
    SDL_Renderer *r = ctx->renderer;

    /* Overlay scuro */
    disegna_rettangolo_pieno(r, 0, 0, WIN_W, WIN_H, (SDL_Color){0, 0, 0, 180});

    /* Pannello centrale */
    disegna_rettangolo_arrotondato(r, WIN_W / 2 - 200, WIN_H / 2 - 160, 400, 320, 20,
                                   (SDL_Color){1, 23, 67, 200});

    disegna_testo_centrato(r, ctx->font_medio, "Scegli un colore:",
                           (SDL_Color){255, 255, 255, 255}, WIN_W / 2, WIN_H / 2 - 120);

    SDL_Color colori_sdl[4] = {
        {220, 50, 50, 255}, {230, 190, 30, 255},
        {50, 180, 80, 255}, {50, 100, 210, 255}
    };
    const char *nomi[4] = {"ROSSO", "GIALLO", "VERDE", "BLU"};
    int mx, my; SDL_GetMouseState(&mx, &my);

    for (int i = 0; i < 4; i++) {
        int bx  = WIN_W / 2 - 170 + i * 90;
        int by  = WIN_H / 2 - 70;
        int hov = (mx >= bx && mx <= bx + 70 && my >= by && my <= by + 70);
        SDL_Color bc = colori_sdl[i];
        if (hov) { bc.r = SDL_min(bc.r + 30, 255); bc.g = SDL_min(bc.g + 30, 255); }
        disegna_rettangolo_arrotondato(r, bx, by, 70, 70, 10, bc);
        if (hov)
            disegna_bordo_arrotondato(r, bx, by, 70, 70, 10, (SDL_Color){255, 220, 60, 255}, 2);
        disegna_testo_centrato(r, ctx->font_piccolo, nomi[i],
                               (SDL_Color){255, 255, 255, 255}, bx + 35, by + 80);
    }
}

/* ═══════════════════════════════════════════
   SCHERMATA PAUSA
═══════════════════════════════════════════ */

void render_pausa(ContestoSDL *ctx, StatoUI *ui) {
    SDL_Renderer *r = ctx->renderer;

    /* Disegna il gioco sotto la pausa */
    render_gioco(ctx, ui);

    /* Overlay scuro */
    disegna_rettangolo_pieno(r, 0, 0, WIN_W, WIN_H, (SDL_Color){0, 0, 0, 180});

    /* Pannello centrale */
    int pw = 500, ph = 300;
    int px = WIN_W / 2 - pw / 2, py = WIN_H / 2 - ph / 2;
    disegna_rettangolo_arrotondato(r, px, py, pw, ph, 20, (SDL_Color){20, 20, 60, 230});

    SDL_SetRenderDrawColor(r, 255, 220, 60, 255);
    SDL_Rect bordo = {px, py, pw, ph};
    SDL_RenderDrawRect(r, &bordo);

    /* Titolo PAUSA */
    disegna_testo_contorno(r, ctx->font_grande, "PAUSA",
                            (SDL_Color){230, 190, 30, 255}, WIN_W / 2, py + 50, 3, 1);

    disegna_testo_centrato(r, ctx->font_medio, "Salvare la partita?",
                           (SDL_Color){255, 255, 255, 255}, WIN_W / 2, py + 110);

    int mx, my; SDL_GetMouseState(&mx, &my);
    int bw = 180, bh = 50, by = py + 170;
    SDL_Color nero = {0, 0, 0, 255};

    /* Pulsante SALVA */
    int bx1 = WIN_W / 2 - bw - 20;
    int h1  = (mx >= bx1 && mx <= bx1 + bw && my >= by && my <= by + bh);
    disegna_rettangolo_arrotondato(r, bx1, by, bw, bh, 10,
        h1 ? (SDL_Color){255, 248, 154, 255} : (SDL_Color){255, 237, 0, 255});
    disegna_testo_centrato(r, ctx->font_medio, "SALVA", nero, bx1 + bw / 2, by + bh / 2);

    /* Pulsante NON SALVARE */
    int bx2 = WIN_W / 2 + 20;
    int h2  = (mx >= bx2 && mx <= bx2 + bw && my >= by && my <= by + bh);
    disegna_rettangolo_arrotondato(r, bx2, by, bw, bh, 10,
        h2 ? (SDL_Color){200, 80, 80, 255} : (SDL_Color){160, 50, 50, 255});
    disegna_testo_centrato(r, ctx->font_medio, "NON SALVARE",
                           (SDL_Color){255, 255, 255, 255}, bx2 + bw / 2, by + bh / 2);

    /* Pulsante ANNULLA */
    int bx3 = WIN_W / 2 - 80, by3 = py + 240, bw3 = 160, bh3 = 40;
    int h3  = (mx >= bx3 && mx <= bx3 + bw3 && my >= by3 && my <= by3 + bh3);
    disegna_rettangolo_arrotondato(r, bx3, by3, bw3, bh3, 8,
        h3 ? (SDL_Color){80, 160, 100, 255} : (SDL_Color){40, 120, 60, 255});
    disegna_testo_centrato(r, ctx->font_piccolo, "ANNULLA",
                           (SDL_Color){255, 255, 255, 255}, bx3 + bw3 / 2, by3 + bh3 / 2);
}

/* ═══════════════════════════════════════════
   SCHERMATA VITTORIA / FINE PARTITA
═══════════════════════════════════════════ */

void render_vittoria(ContestoSDL *ctx, StatoUI *ui) {
    SDL_Renderer *r = ctx->renderer;

    if (ctx->tex_sfondo_fine_partita)
        SDL_RenderCopy(r, ctx->tex_sfondo_fine_partita, NULL, NULL);
    else
        disegna_rettangolo_pieno(r, 0, 0, WIN_W, WIN_H, (SDL_Color){15, 70, 30, 255});

    disegna_rettangolo_arrotondato(r, WIN_W / 2 - 250, WIN_H / 2 - 200, 500, 400, 20,
                                   (SDL_Color){20, 20, 20, 170});

    /* Titolo "FINE PARTITA!" con contorno */
    disegna_testo_contorno(r, ctx->font_grande, "FINE PARTITA!",
                            (SDL_Color){230, 190, 30, 255},
                            WIN_W / 2, WIN_H / 2 - 140, 3, 1);

    disegna_testo_centrato(r, ctx->font_medio, ui->messaggio,
                           (SDL_Color){255, 255, 255, 255}, WIN_W / 2, WIN_H / 2 - 70);

    /* Carte rimaste per ogni giocatore */
    StatoPartita *partita = &ui->partita;
    disegna_testo_centrato(r, ctx->font_piccolo, "Carte rimaste:",
                           (SDL_Color){255, 255, 255, 255}, WIN_W / 2, WIN_H / 2 - 20);
    for (int i = 0; i < partita->num_giocatori; i++) {
        char buf[64];
        snprintf(buf, 64, "%s:  %d carte",
                 partita->giocatori[i].nome, partita->giocatori[i].mano.num_carte);
        SDL_Color tc = (partita->giocatori[i].mano.num_carte == 0)
            ? (SDL_Color){230, 190, 30, 255}
            : (SDL_Color){255, 255, 255, 255};
        disegna_testo_centrato(r, ctx->font_piccolo, buf, tc,
                               WIN_W / 2, WIN_H / 2 + 10 + i * 24);
    }

    /* Pulsanti RIVINCITA e MENU */
    int mx, my; SDL_GetMouseState(&mx, &my);
    int bw = 200, bh = 44, by = WIN_H / 2 + 140;
    SDL_Color nero = {0, 0, 0, 255};

    int bx1 = WIN_W / 2 - bw - 15;
    int h1  = (mx >= bx1 && mx <= bx1 + bw && my >= by && my <= by + bh);
    disegna_rettangolo_arrotondato(r, bx1, by, bw, bh, 8,
        h1 ? (SDL_Color){255, 248, 154, 255} : (SDL_Color){255, 237, 0, 255});
    disegna_testo_centrato(r, ctx->font_medio, "RIVINCITA", nero, bx1 + bw / 2, by + bh / 2);

    int bx2 = WIN_W / 2 + 15;
    int h2  = (mx >= bx2 && mx <= bx2 + bw && my >= by && my <= by + bh);
    disegna_rettangolo_arrotondato(r, bx2, by, bw, bh, 8,
        h2 ? (SDL_Color){255, 248, 154, 255} : (SDL_Color){255, 237, 0, 255});
    disegna_testo_centrato(r, ctx->font_medio, "MENU", nero, bx2 + bw / 2, by + bh / 2);
}

/* ═══════════════════════════════════════════
   SCHERMATA CLASSIFICA
═══════════════════════════════════════════ */

void render_classifica(ContestoSDL *ctx, StatoUI *ui) {
    SDL_Renderer *r = ctx->renderer;

    /* Sfondo classifica: immagine se disponibile, altrimenti verde solido */
    if (ctx->tex_sfondo_classifica)
        SDL_RenderCopy(r, ctx->tex_sfondo_classifica, NULL, NULL);
    else
        disegna_rettangolo_pieno(r, 0, 0, WIN_W, WIN_H, (SDL_Color){15, 70, 30, 255});

    /* Pannello semitrasparente sopra lo sfondo */
    disegna_rettangolo_arrotondato(r, 60, 40, WIN_W - 120, WIN_H - 80, 20,
                                   (SDL_Color){0, 0, 0, 130});

    /* Titolo con contorno */
    disegna_testo_contorno(r, ctx->font_grande, "CLASSIFICA",
                            (SDL_Color){230, 190, 30, 255}, WIN_W / 2, 85, 3, 1);

    /* Ordina la classifica */
    ordina_classifica(ui->record, ui->num_record);

    /* Parametri tabella */
    int tbl_x    = 100, tbl_w = WIN_W - 200;
    int y0       = 135, riga_h = 34, header_h = 36;
    int col_pos[6] = {tbl_x + 30, tbl_x + 100, tbl_x + 340,
                      tbl_x + 460, tbl_x + 590, tbl_x + 770};
    const char *nomi_col[6] = {"#", "Nome", "Vinte", "Perse", "Giocate", "Win Rate"};

    /* Header */
    disegna_rettangolo_arrotondato(r, tbl_x, y0, tbl_w, header_h, 6,
                                   (SDL_Color){0, 123, 223, 200});
    for (int c = 0; c < 6; c++)
        disegna_testo_centrato(r, ctx->font_piccolo, nomi_col[c],
                               (SDL_Color){255, 255, 255, 255}, col_pos[c], y0 + header_h / 2);

    SDL_SetRenderDrawColor(r, 255, 220, 60, 120);
    SDL_RenderDrawLine(r, tbl_x, y0 + header_h, tbl_x + tbl_w, y0 + header_h);

    /* Righe dati con scroll */
    int inizio    = ui->scroll_classifica;
    int max_righe = 14;
    int fine      = inizio + max_righe;
    if (fine > ui->num_record) fine = ui->num_record;

    SDL_Color oro     = {255, 215,   0, 255};
    SDL_Color argento = {192, 192, 192, 255};
    SDL_Color bronzo  = {205, 127,  50, 255};
    SDL_Color bianco  = {255, 255, 255, 255};
    SDL_Color verde   = { 60, 180,  80, 255};
    SDL_Color giallo  = {230, 190,  30, 255};

    for (int i = inizio; i < fine; i++) {
        int riga_idx = i - inizio;
        int y = y0 + header_h + riga_idx * riga_h;

        /* Riga alternata */
        if (riga_idx % 2 == 1)
            disegna_rettangolo_pieno(r, tbl_x, y, tbl_w, riga_h,
                                     (SDL_Color){0, 108, 183, 80});

        /* Colore nome per i primi 3 */
        SDL_Color col_nome = bianco;
        if (i == 0)      col_nome = oro;
        else if (i == 1) col_nome = argento;
        else if (i == 2) col_nome = bronzo;

        float wr = (ui->record[i].totali > 0)
            ? ((float)ui->record[i].vinte * 100.0f / (float)ui->record[i].totali)
            : 0.0f;
        ui->record[i].percentuale_vittorie = wr;

        char buf[128];
        snprintf(buf, sizeof(buf), "%d", i + 1);
        disegna_testo_centrato(r, ctx->font_piccolo, buf, col_nome, col_pos[0], y + riga_h / 2);

        snprintf(buf, sizeof(buf), "%s", ui->record[i].nome);
        disegna_testo_centrato(r, ctx->font_piccolo, buf, col_nome, col_pos[1], y + riga_h / 2);

        snprintf(buf, sizeof(buf), "%d", ui->record[i].vinte);
        disegna_testo_centrato(r, ctx->font_piccolo, buf, verde, col_pos[2], y + riga_h / 2);

        snprintf(buf, sizeof(buf), "%d", ui->record[i].perse);
        disegna_testo_centrato(r, ctx->font_piccolo, buf, bianco, col_pos[3], y + riga_h / 2);

        snprintf(buf, sizeof(buf), "%d", ui->record[i].totali);
        disegna_testo_centrato(r, ctx->font_piccolo, buf, bianco, col_pos[4], y + riga_h / 2);

        snprintf(buf, sizeof(buf), "%.1f%%", wr);
        SDL_Color col_wr = bianco;
        if      (wr >= 50.0f) col_wr = verde;
        else if (wr >= 30.0f) col_wr = giallo;
        else if (wr >  0.0f)  col_wr = (SDL_Color){220, 50, 50, 255};
        disegna_testo_centrato(r, ctx->font_piccolo, buf, col_wr, col_pos[5], y + riga_h / 2);

        /* Linea separatrice riga */
        SDL_SetRenderDrawColor(r, 255, 255, 255, 25);
        SDL_RenderDrawLine(r, tbl_x + 10, y + riga_h, tbl_x + tbl_w - 10, y + riga_h);
    }

    if (ui->num_record == 0) {
        disegna_testo_centrato(r, ctx->font_medio, "Nessun giocatore registrato",
                               bianco, WIN_W / 2, WIN_H / 2);
        disegna_testo_centrato(r, ctx->font_piccolo,
                               "Gioca una partita per apparire in classifica!",
                               (SDL_Color){180, 180, 180, 255}, WIN_W / 2, WIN_H / 2 + 30);
    }

    /* Indicatore scroll */
    if (ui->num_record > max_righe) {
        char buf_scroll[64];
        snprintf(buf_scroll, 64, "Scroll: %d-%d di %d  (rotella mouse)",
                 inizio + 1, fine, ui->num_record);
        disegna_testo_centrato(r, ctx->font_piccolo, buf_scroll,
                               (SDL_Color){180, 180, 180, 200}, WIN_W / 2, WIN_H - 60);
    }

    /* Hint ESC con contorno */
    disegna_testo_contorno(r, ctx->font_piccolo, "Premi ESC per tornare al menu",
                            bianco, WIN_W / 2, WIN_H - 35, 2, 1);
}

/* ═══════════════════════════════════════════
   DISPATCHER RENDERING PRINCIPALE
═══════════════════════════════════════════ */

void render_tutto(ContestoSDL *ctx, StatoUI *ui) {
    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx->renderer);

    switch (ui->schermata) {
        case SCHERMATA_MENU:    render_menu(ctx, ui);    break;
        case SCHERMATA_SETUP:   render_setup(ctx, ui);   break;
        case SCHERMATA_GIOCO:   render_gioco(ctx, ui);   break;
        case SCHERMATA_SCELTA_COLORE:
            render_gioco(ctx, ui);
            render_scelta_colore(ctx, ui);
            break;
        case SCHERMATA_PAUSA:      render_pausa(ctx, ui);     break;
        case SCHERMATA_VITTORIA:   render_vittoria(ctx, ui);  break;
        case SCHERMATA_CLASSIFICA: render_classifica(ctx, ui);break;
        default: break;
    }

    SDL_RenderPresent(ctx->renderer);
}
