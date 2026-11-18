/*
 * grafica.c
 * Primitivi SDL2: rettangoli, testo, disegno carte PNG e caricamento
 * delle texture/font usati nell'intera interfaccia grafica.
 */

#include "grafica.h"

/* ═══════════════════════════════════════════
   PRIMITIVI RETTANGOLO
═══════════════════════════════════════════ */

/* Riempie un rettangolo con BLENDMODE per il supporto alla trasparenza */
void disegna_rettangolo_pieno(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

/* Approssima un rettangolo arrotondato con tre rettangoli e archi a riga singola */
void disegna_rettangolo_arrotondato(SDL_Renderer *r, int x, int y, int w, int h,
                                     int raggio, SDL_Color c) {
    if (w <= 0 || h <= 0) return;
    if (raggio < 0)     raggio = 0;
    if (raggio > w / 2) raggio = w / 2;
    if (raggio > h / 2) raggio = h / 2;

    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    /* Rettangolo centrale orizzontale */
    SDL_Rect centro = { x + raggio, y, w - 2 * raggio, h };
    SDL_RenderFillRect(r, &centro);

    /* Fasce laterali */
    SDL_Rect sinistra = { x,             y + raggio, raggio, h - 2 * raggio };
    SDL_Rect destra   = { x + w - raggio, y + raggio, raggio, h - 2 * raggio };
    SDL_RenderFillRect(r, &sinistra);
    SDL_RenderFillRect(r, &destra);

    /* Angoli arrotondati con linee orizzontali per ogni riga dell'arco */
    for (int dy = 0; dy < raggio; dy++) {
        int dx = (int)(sqrtf((float)(raggio * raggio - dy * dy)));

        int y_top = y + raggio - dy - 1;
        int y_bot = y + h - raggio + dy;

        SDL_RenderDrawLine(r, x + raggio - dx, y_top, x + raggio - 1,      y_top);
        SDL_RenderDrawLine(r, x + w - raggio,  y_top, x + w - raggio + dx - 1, y_top);
        SDL_RenderDrawLine(r, x + raggio - dx, y_bot, x + raggio - 1,      y_bot);
        SDL_RenderDrawLine(r, x + w - raggio,  y_bot, x + w - raggio + dx - 1, y_bot);
    }
}

/* Bordo arrotondato — riempito (usato per evidenziare elementi UI) */
void disegna_bordo_arrotondato(SDL_Renderer *r, int x, int y, int w, int h,
                                int raggio, SDL_Color c, int spessore) {
    if (spessore <= 0 || w <= 0 || h <= 0) return;
    if (raggio < 0)     raggio = 0;
    if (raggio > w / 2) raggio = w / 2;
    if (raggio > h / 2) raggio = h / 2;

    disegna_rettangolo_arrotondato(r, x, y, w, h, raggio, c);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
}

/* ═══════════════════════════════════════════
   TESTO
═══════════════════════════════════════════ */

/* Renderizza testo UTF-8 centrato sul punto (cx, cy) */
void disegna_testo_centrato(SDL_Renderer *r, TTF_Font *font,
                             const char *testo, SDL_Color c, int cx, int cy) {
    if (!font || !testo || testo[0] == '\0') return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, testo, c);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    int tw = surf->w, th = surf->h;
    SDL_FreeSurface(surf);
    if (!tex) return;
    SDL_Rect dst = { cx - tw / 2, cy - th / 2, tw, th };
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

/* Renderizza testo UTF-8 con allineamento a sinistra a partire da (x, y) */
void disegna_testo(SDL_Renderer *r, TTF_Font *font,
                   const char *testo, SDL_Color c, int x, int y) {
    if (!font || !testo || testo[0] == '\0') return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, testo, c);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_FreeSurface(surf);
    if (!tex) return;
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

/* ═══════════════════════════════════════════
   DISEGNO CARTE
═══════════════════════════════════════════ */

/* Disegna una carta con la texture PNG; se scoperta=0 mostra il retro */
void disegna_carta_png(ContestoSDL *ctx, Carta c, int x, int y, int scoperta) {
    SDL_Rect rect = { x, y, 90, 140 };
    SDL_Texture *tex = NULL;

    if (!scoperta) {
        tex = ctx->tex_retro;
    } else {
        if (c.tipo == CHANGE_COLOR || c.tipo == DRAW_4) {
            tex = ctx->tex_carte[4][(int)c.tipo];
        } else {
            tex = ctx->tex_carte[(int)c.colore][(int)c.tipo];
        }
    }

    if (tex) {
        SDL_RenderCopy(ctx->renderer, tex, NULL, &rect);
    } else {
        /* Fallback: rettangolo grigio se la texture manca */
        SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(ctx->renderer, &rect);
    }
}

/* Disegna solo il retro della carta con un'ombra leggera */
void disegna_retro_carta(ContestoSDL *ctx, int x, int y) {
    if (!ctx || !ctx->tex_retro) {
        /* Fallback: rettangolo blu scuro */
        SDL_Rect r_back = { x, y, CARD_W, CARD_H };
        SDL_SetRenderDrawColor(ctx->renderer, 30, 60, 160, 255);
        SDL_RenderFillRect(ctx->renderer, &r_back);
        return;
    }

    /* Ombra leggera offset di 3px */
    SDL_Rect ombra = { x + 3, y + 3, CARD_W, CARD_H };
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 80);
    SDL_RenderFillRect(ctx->renderer, &ombra);

    SDL_Rect rect = { x, y, CARD_W, CARD_H };
    SDL_RenderCopy(ctx->renderer, ctx->tex_retro, NULL, &rect);
}

/* ═══════════════════════════════════════════
   CARICAMENTO RISORSE
═══════════════════════════════════════════ */

/* Carica tutte le texture PNG delle carte dal filesystem */
void carica_texture_carte(ContestoSDL *ctx) {
    char percorso[256];
    const char *nomi_colori[] = {"red", "yellow", "green", "blue", "wild"};

    /* Dorso */
    ctx->tex_retro = IMG_LoadTexture(ctx->renderer, "./individual/card_back/card_back.png");
    if (!ctx->tex_retro)
        printf("ERRORE: dorso carta non trovato: %s\n", IMG_GetError());

    /* Carte colorate (4 colori × 13 valori) */
    for (int c = 0; c < 4; c++) {
        for (int v = 0; v <= 12; v++) {
            if (v <= 9)
                sprintf(percorso, "individual/%s/%d_%s.png",
                        nomi_colori[c], v, nomi_colori[c]);
            else if (v == 10)
                sprintf(percorso, "individual/%s/block_%s.png",
                        nomi_colori[c], nomi_colori[c]);
            else if (v == 11)
                sprintf(percorso, "individual/%s/inverse_%s.png",
                        nomi_colori[c], nomi_colori[c]);
            else
                sprintf(percorso, "individual/%s/2plus_%s.png",
                        nomi_colori[c], nomi_colori[c]);

            ctx->tex_carte[c][v] = IMG_LoadTexture(ctx->renderer, percorso);
            if (!ctx->tex_carte[c][v])
                printf("Immagine mancante: %s\n", percorso);
        }
    }

    /* Jolly */
    ctx->tex_carte[4][13] = IMG_LoadTexture(ctx->renderer,
                                            "individual/wild/wild_card.png");
    ctx->tex_carte[4][14] = IMG_LoadTexture(ctx->renderer,
                                            "individual/wild/4_plus.png");
}

/* Carica sfondi, pulsante UNO e icone del menu */
void carica_risorse_menu(ContestoSDL *ctx) {
    ctx->tex_sfondo_menu = IMG_LoadTexture(ctx->renderer, "sfondo_menu.jpg");
    if (!ctx->tex_sfondo_menu)
        printf("Errore sfondo menu: %s\n", IMG_GetError());

    ctx->tex_sfondo_tavolo = IMG_LoadTexture(ctx->renderer, "sfondo_tavolo.jpg");
    if (!ctx->tex_sfondo_tavolo)
        printf("Errore sfondo tavolo: %s\n", IMG_GetError());

    ctx->tex_sfondo_fine_partita = IMG_LoadTexture(ctx->renderer, "sfondo_fine_partita.jpg");
    if (!ctx->tex_sfondo_fine_partita)
        printf("Errore sfondo fine partita: %s\n", IMG_GetError());

    ctx->tex_sfondo_classifica = IMG_LoadTexture(ctx->renderer, "sfondo_classifica.jpg");
    if (!ctx->tex_sfondo_classifica)
        printf("Errore sfondo classifica: %s\n", IMG_GetError());

    ctx->tex_pulsante_uno = IMG_LoadTexture(ctx->renderer, "pulsante_uno.png");
    if (ctx->tex_pulsante_uno) {
        SDL_SetTextureBlendMode(ctx->tex_pulsante_uno, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(ctx->tex_pulsante_uno, 255);
    } else {
        printf("Errore pulsante UNO: %s\n", IMG_GetError());
    }

    ctx->sfondo_setup_rosso  = IMG_LoadTexture(ctx->renderer, "sfondo_setup_rosso.jpg");
    ctx->sfondo_setup_giallo = IMG_LoadTexture(ctx->renderer, "sfondo_setup_giallo.jpg");
    ctx->sfondo_setup_verde  = IMG_LoadTexture(ctx->renderer, "sfondo_setup_verde.jpg");
    ctx->sfondo_setup_blu    = IMG_LoadTexture(ctx->renderer, "sfondo_setup_blu.jpg");

    const char *icone[] = {
        "individual/menu/nuova_partita.png",
        "individual/menu/carica_partita.png",
        "individual/menu/classifica.png",
        "individual/menu/esci.png"
    };
    for (int i = 0; i < 4; i++) {
        ctx->tex_menu_icone[i] = IMG_LoadTexture(ctx->renderer, icone[i]);
        if (!ctx->tex_menu_icone[i])
            printf("Errore icona menu %d: %s\n", i, IMG_GetError());
    }
}

/* Apre un font TTF con fallback automatico su più percorsi di sistema */
TTF_Font *carica_font(int dimensione) {
    TTF_Font *f = TTF_OpenFont(FONT_PATH, dimensione);
    if (!f) f = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", dimensione);
    if (!f) f = TTF_OpenFont("/Library/Fonts/Arial Bold.ttf", dimensione);
    if (!f) f = TTF_OpenFont("C:/Windows/Fonts/arialbd.ttf", dimensione);
    return f;
}
