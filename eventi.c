/*
 * eventi.c
 * Gestione degli input SDL2 per ogni schermata del gioco:
 * click del mouse, tastiera e rotella di scorrimento.
 */

#include "eventi.h"
#include "logica.h"
#include "database.h"
#include "salvataggio.h"
#include "schermate.h"

/* ═══════════════════════════════════════════
   MENU
═══════════════════════════════════════════ */

void evento_menu(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui) {
    if (evento->type != SDL_MOUSEBUTTONDOWN || evento->button.button != SDL_BUTTON_LEFT)
        return;

    int mx = evento->button.x, my = evento->button.y;

    /* Parametri identici al rendering del menu */
    int larg_carta = 200, alto_carta = 320, spaziatura = 10;
    int totale_w  = (larg_carta * 4) + (spaziatura * 3);
    int inizio_x  = (WIN_W - totale_w) / 2;
    int inizio_y  = WIN_H - 420;

    for (int i = 0; i < 4; i++) {
        SDL_Rect r = { inizio_x + i * (larg_carta + spaziatura), inizio_y, larg_carta, alto_carta };

        if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) {
            switch (i) {
                case 0: /* Nuova partita */
                    ui->scelta_sfondo_setup = rand() % 4;
                    ui->schermata = SCHERMATA_SETUP;
                    break;

                case 1: /* Carica partita */
                    if (carica_partita_json(ui))
                        ui->schermata = SCHERMATA_GIOCO;
                    else {
                        snprintf(ui->messaggio, 128, "Nessuna partita salvata trovata");
                        ui->timer_messaggio = 80;
                    }
                    break;

                case 2: /* Classifica */
                    carica_giocatori_db(ctx, ui);
                    ui->schermata = SCHERMATA_CLASSIFICA;
                    ui->scroll_classifica = 0;
                    break;

                case 3: /* Esci */
                    ui->schermata = SCHERMATA_ESCI;
                    break;
            }
            return;
        }
    }
}

/* ═══════════════════════════════════════════
   SETUP
═══════════════════════════════════════════ */

void evento_setup(SDL_Event *evento, StatoUI *ui) {
    if (evento->type == SDL_KEYDOWN) {
        if (evento->key.keysym.sym == SDLK_ESCAPE) {
            ui->schermata = SCHERMATA_MENU;
            return;
        }
        if (evento->key.keysym.sym == SDLK_BACKSPACE && ui->lunghezza_nome > 0) {
            ui->nome_input[--ui->lunghezza_nome] = '\0';
        }
        return;
    }

    if (evento->type == SDL_TEXTINPUT) {
        if (ui->lunghezza_nome < MAX_NOME - 1) {
            ui->nome_input[ui->lunghezza_nome++] = evento->text.text[0];
            ui->nome_input[ui->lunghezza_nome]   = '\0';
        }
        return;
    }

    if (evento->type == SDL_MOUSEBUTTONDOWN && evento->button.button == SDL_BUTTON_LEFT) {
        int mx = evento->button.x, my = evento->button.y;

        /* Selezione numero di avversari */
        for (int i = 1; i <= MAX_AVVERSARI; i++) {
            int bx = WIN_W / 2 - (MAX_AVVERSARI * 50) / 2 + (i - 1) * 50;
            int by = 345;
            if (mx >= bx && mx <= bx + 40 && my >= by && my <= by + 40) {
                ui->num_avversari_setup = i;
                return;
            }
        }

        /* Pulsante INIZIA */
        int bx = WIN_W / 2 - 120, by = 430, bw = 240, bh = 50;
        if (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh &&
            ui->nome_input[0] != '\0') {
            init_partita(ui);
            ui->schermata = SCHERMATA_GIOCO;
        }
    }
}

/* ═══════════════════════════════════════════
   GIOCO
═══════════════════════════════════════════ */

void evento_gioco(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui) {
    StatoPartita *partita = &ui->partita;

    if (evento->type == SDL_KEYDOWN && evento->key.keysym.sym == SDLK_ESCAPE) {
        ui->schermata = SCHERMATA_PAUSA;
        return;
    }

    /* Aggiornamento hover carta e mazzo al movimento del mouse */
    if (evento->type == SDL_MOUSEMOTION) {
        int mx = evento->motion.x, my = evento->motion.y;
        ui->hover_carta = -1;
        ui->hover_pesca = 0;

        if (partita->turno_corrente == 0) {
            Mano *mano = &partita->giocatori[0].mano;
            int   n    = mano->num_carte;

            /* Scansione inversa per favorire la carta in primo piano */
            for (int i = n - 1; i >= 0; i--) {
                int cx, cy;
                posizione_carta_mano(i, n, &cx, &cy);
                if (mx >= cx && mx <= cx + CARD_W && my >= cy && my <= cy + CARD_H) {
                    ui->hover_carta = i;
                    break;
                }
            }

            /* Hover sul mazzo */
            int mz_x = WIN_W / 2 - CARD_W - 30;
            int mz_y = WIN_H / 2 - 20 - CARD_H / 2;
            ui->hover_pesca = (mx >= mz_x && mx <= mz_x + CARD_W &&
                               my >= mz_y && my <= mz_y + CARD_H &&
                               ui->timer_messaggio == 0);
        }
        return;
    }

    if (evento->type != SDL_MOUSEBUTTONDOWN || evento->button.button != SDL_BUTTON_LEFT)
        return;

    if (partita->turno_corrente != 0) return; /* Non è il turno dell'umano */

    int  mx   = evento->button.x, my = evento->button.y;
    Mano *mano = &partita->giocatori[0].mano;
    int   n    = mano->num_carte;

    /* Intercetta il click sul pulsante UNO */
    if (ui->mostra_pulsante_uno && !ui->ha_cliccato_uno) {
        if (mx >= 720 && mx <= 820 && my >= 550 && my <= 650) {
            ui->ha_cliccato_uno     = 1;
            ui->mostra_pulsante_uno = 0;
            snprintf(ui->messaggio, 128, "UNO!");
            ui->timer_messaggio = 50;
            return;
        }
    }

    /* Click sul mazzo: pesca una carta */
    int mz_x = WIN_W / 2 - CARD_W - 30;
    int mz_y = WIN_H / 2 - 20 - CARD_H / 2;
    if (mx >= mz_x && mx <= mz_x + CARD_W && my >= mz_y && my <= mz_y + CARD_H) {
        if (ui->mostra_riciclo_mazzo == 0) {
            /* Ricicla gli scarti se il mazzo è vuoto */
            if (partita->mazzo.num_carte == 0 && partita->scarti.num_carte > 0) {
                partita->mazzo.num_carte = 0;
                for (int i = 0; i < partita->scarti.num_carte; i++)
                    partita->mazzo.carte[i] = partita->scarti.carte[i];
                partita->mazzo.num_carte  = partita->scarti.num_carte;
                partita->scarti.num_carte = 0;
                mazzo_mescola(&partita->mazzo);
                ui->mostra_riciclo_mazzo = 1;
            }

            /* Pesca la carta */
            if (partita->mazzo.num_carte > 0) {
                Carta pescata = mazzo_pesca(&partita->mazzo);
                mano_aggiungi(mano, pescata);
                snprintf(ui->messaggio, 128, "Hai pescato una carta");
                ui->timer_messaggio = 30;

                if (carta_giocabile(pescata, partita->carta_tavolo)) {
                    /* La carta è giocabile: la preseleziona */
                    ui->carta_selezionata = mano->num_carte - 1;
                    ui->hover_pesca       = 0;
                } else {
                    /* Carta non giocabile: passa il turno */
                    partita->turno_corrente = (partita->turno_corrente + partita->direzione +
                                              partita->num_giocatori) % partita->num_giocatori;
                    ui->carta_selezionata = -1;
                }
            }
        }
        return;
    }

    /* Click su una carta in mano */
    for (int i = n - 1; i >= 0; i--) {
        int cx, cy;
        posizione_carta_mano(i, n, &cx, &cy);
        if (mx >= cx && mx <= cx + CARD_W && my >= cy && my <= cy + CARD_H) {
            if (!carta_giocabile(mano->carte[i], partita->carta_tavolo)) {
                snprintf(ui->messaggio, 128, "Carta non giocabile!");
                ui->timer_messaggio = 30;
                return;
            }

            /* Jolly: apre la schermata di scelta colore */
            if (mano->carte[i].tipo == CHANGE_COLOR || mano->carte[i].tipo == DRAW_4) {
                ui->carta_da_giocare = i;
                ui->schermata        = SCHERMATA_SCELTA_COLORE;
                return;
            }

            Carta giocata = mano->carte[i];

            /* Salva la carta del tavolo negli scarti */
            partita->scarti.carte[partita->scarti.num_carte++] = partita->carta_tavolo;

            /* Attiva il pulsante UNO se il giocatore sta per restare con 1 carta */
            if (mano->num_carte == 2) {
                ui->mostra_pulsante_uno  = 1;
                ui->timer_pulsante_uno   = SDL_GetTicks();
                ui->ha_cliccato_uno      = 0;
            }

            mano_rimuovi(mano, i);
            partita->carta_tavolo = giocata;
            ui->carta_selezionata = -1;

            if (mano->num_carte == 0) {
                snprintf(ui->messaggio, 128, "%s ha vinto!", partita->giocatori[0].nome);
                ui->timer_messaggio = 100;
                aggiorna_statistiche_fine_partita(ui, partita->giocatori[0].nome,
                                                  partita->num_giocatori);
                aggiorna_stats_db(ctx, partita->giocatori[0].nome, 1);
                ui->schermata = SCHERMATA_VITTORIA;
                return;
            }

            applica_effetto(ui, giocata);
            return;
        }
    }
}

/* ═══════════════════════════════════════════
   SCELTA COLORE (jolly)
═══════════════════════════════════════════ */

void evento_scelta_colore(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui) {
    if (evento->type != SDL_MOUSEBUTTONDOWN || evento->button.button != SDL_BUTTON_LEFT)
        return;

    int mx = evento->button.x, my = evento->button.y;
    Colore mappa[4] = { ROSSO, GIALLO, VERDE, BLU };

    for (int i = 0; i < 4; i++) {
        int bx = WIN_W / 2 - 170 + i * 90;
        int by = WIN_H / 2 - 70;
        if (mx >= bx && mx <= bx + 70 && my >= by && my <= by + 70) {
            StatoPartita *partita = &ui->partita;
            Mano *mano = &partita->giocatori[0].mano;
            int   idx  = ui->carta_da_giocare;

            Carta giocata      = mano->carte[idx];
            giocata.colore_scelto = mappa[i];
            giocata.colore        = mappa[i];

            /* Salva la carta sul tavolo prima di cambiarla */
            partita->scarti.carte[partita->scarti.num_carte++] = partita->carta_tavolo;

            mano_rimuovi(mano, idx);
            partita->carta_tavolo = giocata;

            if (mano->num_carte == 0) {
                snprintf(ui->messaggio, 128, "%s ha vinto!", partita->giocatori[0].nome);
                ui->timer_messaggio = 100;
                aggiorna_statistiche_fine_partita(ui, partita->giocatori[0].nome,
                                                  partita->num_giocatori);
                aggiorna_stats_db(ctx, partita->giocatori[0].nome, 1);
                ui->schermata = SCHERMATA_VITTORIA;
                return;
            }

            if (mano->num_carte == 1) {
                snprintf(ui->messaggio, 128, "UNO!");
                ui->timer_messaggio = 60;
            }

            applica_effetto(ui, giocata);
            ui->carta_da_giocare  = -1;
            ui->carta_selezionata = -1;
            ui->schermata         = SCHERMATA_GIOCO;
            return;
        }
    }
}

/* ═══════════════════════════════════════════
   PAUSA
═══════════════════════════════════════════ */

void evento_pausa(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui) {
    if (evento->type == SDL_KEYDOWN && evento->key.keysym.sym == SDLK_ESCAPE) {
        ui->schermata = SCHERMATA_GIOCO;
        return;
    }

    if (evento->type != SDL_MOUSEBUTTONDOWN || evento->button.button != SDL_BUTTON_LEFT)
        return;

    int mx = evento->button.x, my = evento->button.y;
    int pw = 500, ph = 300;
    int px = WIN_W / 2 - pw / 2, py = WIN_H / 2 - ph / 2;
    int bw = 180, bh = 50, by = py + 170;

    /* SALVA e torna al menu */
    int bx1 = WIN_W / 2 - bw - 20;
    if (mx >= bx1 && mx <= bx1 + bw && my >= by && my <= by + bh) {
        salva_partita_json(ui);
        snprintf(ui->messaggio, 128, "Partita salvata!");
        ui->timer_messaggio = 60;
        ui->schermata = SCHERMATA_MENU;
        return;
    }

    /* NON SALVARE e torna al menu */
    int bx2 = WIN_W / 2 + 20;
    if (mx >= bx2 && mx <= bx2 + bw && my >= by && my <= by + bh) {
        elimina_salvataggio();
        ui->schermata = SCHERMATA_MENU;
        return;
    }

    /* ANNULLA: torna al gioco */
    int bx3 = WIN_W / 2 - 80, by3 = py + 240, bw3 = 160, bh3 = 40;
    if (mx >= bx3 && mx <= bx3 + bw3 && my >= by3 && my <= by3 + bh3) {
        ui->schermata = SCHERMATA_GIOCO;
        return;
    }
}

/* ═══════════════════════════════════════════
   CLASSIFICA
═══════════════════════════════════════════ */

void evento_classifica(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui) {
    if (evento->type == SDL_KEYDOWN && evento->key.keysym.sym == SDLK_ESCAPE) {
        ui->schermata = SCHERMATA_MENU;
        return;
    }
    if (evento->type == SDL_MOUSEWHEEL) {
        ui->scroll_classifica -= evento->wheel.y;
        if (ui->scroll_classifica < 0) ui->scroll_classifica = 0;
        if (ui->scroll_classifica > ui->num_record - 1)
            ui->scroll_classifica = ui->num_record - 1;
    }
}

/* ═══════════════════════════════════════════
   VITTORIA / FINE PARTITA
═══════════════════════════════════════════ */

void evento_vittoria(SDL_Event *evento, StatoUI *ui) {
    if (evento->type != SDL_MOUSEBUTTONDOWN || evento->button.button != SDL_BUTTON_LEFT)
        return;

    int mx = evento->button.x, my = evento->button.y;
    int bw = 200, bh = 44, by = WIN_H / 2 + 140;

    /* RIVINCITA */
    int bx1 = WIN_W / 2 - bw - 15;
    if (mx >= bx1 && mx <= bx1 + bw && my >= by && my <= by + bh) {
        init_partita(ui);
        ui->schermata = SCHERMATA_GIOCO;
        return;
    }

    /* MENU */
    int bx2 = WIN_W / 2 + 15;
    if (mx >= bx2 && mx <= bx2 + bw && my >= by && my <= by + bh) {
        elimina_salvataggio();
        ui->schermata = SCHERMATA_MENU;
    }
}

/* ═══════════════════════════════════════════
   DISPATCHER PRINCIPALE EVENTI
═══════════════════════════════════════════ */

void gestisci_evento(SDL_Event *evento, ContestoSDL *ctx, StatoUI *ui) {
    /* Chiude il popup di riciclo mazzo a qualsiasi click */
    if (ui->mostra_riciclo_mazzo && evento->type == SDL_MOUSEBUTTONDOWN)
        ui->mostra_riciclo_mazzo = 0;

    switch (ui->schermata) {
        case SCHERMATA_MENU:          evento_menu(evento, ctx, ui);          break;
        case SCHERMATA_SETUP:         evento_setup(evento, ui);              break;
        case SCHERMATA_GIOCO:         evento_gioco(evento, ctx, ui);         break;
        case SCHERMATA_SCELTA_COLORE: evento_scelta_colore(evento, ctx, ui); break;
        case SCHERMATA_PAUSA:         evento_pausa(evento, ctx, ui);         break;
        case SCHERMATA_CLASSIFICA:    evento_classifica(evento, ctx, ui);    break;
        case SCHERMATA_VITTORIA:      evento_vittoria(evento, ui);           break;
        default: break;
    }
}
