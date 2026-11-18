/*
 * salvataggio.c
 * Serializzazione e deserializzazione della partita in formato JSON.
 * Non usa librerie esterne: il JSON viene scritto e letto manualmente.
 */

#include "salvataggio.h"

/* ═══════════════════════════════════════════
   HELPER CONVERSIONE STRINGA ↔ ENUM
═══════════════════════════════════════════ */

/* Converte un Colore nel nome stringa usato nel JSON */
static const char *colore_in_stringa(Colore c) {
    switch (c) {
        case ROSSO:  return "ROSSO";
        case GIALLO: return "GIALLO";
        case VERDE:  return "VERDE";
        case BLU:    return "BLU";
        default:     return "NESSUNO";
    }
}

/* Converte una stringa JSON nel Colore corrispondente */
static Colore stringa_in_colore(const char *s) {
    if (strcmp(s, "ROSSO")  == 0) return ROSSO;
    if (strcmp(s, "GIALLO") == 0) return GIALLO;
    if (strcmp(s, "VERDE")  == 0) return VERDE;
    if (strcmp(s, "BLU")    == 0) return BLU;
    return NESSUNO;
}

/* Converte un TipoCarta nel nome stringa usato nel JSON */
static const char *tipo_in_stringa(TipoCarta t) {
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

/* Converte una stringa JSON nel TipoCarta corrispondente */
static TipoCarta stringa_in_tipo(const char *s) {
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

/* ═══════════════════════════════════════════
   HELPER PARSING JSON
═══════════════════════════════════════════ */

/* Legge una stringa JSON tra virgolette; restituisce il puntatore avanzato */
static const char *leggi_stringa_json(const char *p, char *buf, int dim_buf) {
    if (!p || *p != '"') return p;
    p++; /* salta la virgolette aperta */
    int i = 0;
    while (*p && *p != '"' && i < dim_buf - 1)
        buf[i++] = *p++;
    buf[i] = '\0';
    if (*p == '"') p++; /* salta la virgolette chiusa */
    return p;
}

/* Legge un intero dalla stringa; aggiorna il puntatore tramite fine_ptr */
static int leggi_intero_json(const char *p, const char **fine_ptr) {
    return (int)strtol(p, (char **)fine_ptr, 10);
}

/* Legge il contenuto di un file nel buffer allocato con malloc (da liberare) */
static char *leggi_file_in_buffer(const char *percorso) {
    FILE *fp = fopen(percorso, "r");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long dim = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = malloc(dim + 1);
    if (!buf) { fclose(fp); return NULL; }
    fread(buf, 1, dim, fp);
    buf[dim] = '\0';
    fclose(fp);
    return buf;
}

/* Analizza un oggetto carta JSON { "colore": "...", "tipo": "...", "colore_scelto": "..." } */
static Carta analizza_carta_json(const char **puntatore) {
    Carta carta = {NESSUNO, ZERO, NESSUNO};
    const char *p = *puntatore;
    char buf[64];

    while (*p && strncmp(p, "\"colore\"", 8) != 0) p++;
    if (*p) {
        p += 8;
        while (*p && *p != '"') p++;
        p = leggi_stringa_json(p, buf, sizeof(buf));
        carta.colore = stringa_in_colore(buf);
    }

    while (*p && strncmp(p, "\"tipo\"", 6) != 0) p++;
    if (*p) {
        p += 6;
        while (*p && *p != '"') p++;
        p = leggi_stringa_json(p, buf, sizeof(buf));
        carta.tipo = stringa_in_tipo(buf);
    }

    while (*p && strncmp(p, "\"colore_scelto\"", 15) != 0) p++;
    if (*p) {
        p += 15;
        while (*p && *p != '"') p++;
        p = leggi_stringa_json(p, buf, sizeof(buf));
        carta.colore_scelto = stringa_in_colore(buf);
    }

    /* Avanza fino alla chiusura dell'oggetto */
    while (*p && *p != '}') p++;
    if (*p) p++;

    *puntatore = p;
    return carta;
}

/* ═══════════════════════════════════════════
   SALVATAGGIO PARTITA
═══════════════════════════════════════════ */

/* Serializza l'intera partita in un file JSON leggibile */
void salva_partita_json(StatoUI *ui) {
    StatoPartita *p = &ui->partita;
    FILE *fp = fopen(FILE_SALVATAGGIO, "w");
    if (!fp) return;

    fprintf(fp, "{\n");
    fprintf(fp, "  \"num_giocatori\": %d,\n",    p->num_giocatori);
    fprintf(fp, "  \"turno_corrente\": %d,\n",   p->turno_corrente);
    fprintf(fp, "  \"direzione\": %d,\n",         p->direzione);
    fprintf(fp, "  \"carta_tavolo\": {\"colore\": \"%s\", \"tipo\": \"%s\", \"colore_scelto\": \"%s\"},\n",
            colore_in_stringa(p->carta_tavolo.colore),
            tipo_in_stringa(p->carta_tavolo.tipo),
            colore_in_stringa(p->carta_tavolo.colore_scelto));

    /* Mazzo */
    fprintf(fp, "  \"mazzo_num_carte\": %d,\n", p->mazzo.num_carte);
    fprintf(fp, "  \"mazzo\": [\n");
    for (int i = 0; i < p->mazzo.num_carte; i++) {
        fprintf(fp, "    {\"colore\": \"%s\", \"tipo\": \"%s\", \"colore_scelto\": \"%s\"}%s\n",
                colore_in_stringa(p->mazzo.carte[i].colore),
                tipo_in_stringa(p->mazzo.carte[i].tipo),
                colore_in_stringa(p->mazzo.carte[i].colore_scelto),
                (i < p->mazzo.num_carte - 1) ? "," : "");
    }
    fprintf(fp, "  ],\n");

    /* Scarti */
    fprintf(fp, "  \"scarti_num_carte\": %d,\n", p->scarti.num_carte);
    fprintf(fp, "  \"scarti\": [\n");
    for (int i = 0; i < p->scarti.num_carte; i++) {
        fprintf(fp, "    {\"colore\": \"%s\", \"tipo\": \"%s\", \"colore_scelto\": \"%s\"}%s\n",
                colore_in_stringa(p->scarti.carte[i].colore),
                tipo_in_stringa(p->scarti.carte[i].tipo),
                colore_in_stringa(p->scarti.carte[i].colore_scelto),
                (i < p->scarti.num_carte - 1) ? "," : "");
    }
    fprintf(fp, "  ],\n");

    /* Giocatori */
    fprintf(fp, "  \"giocatori\": [\n");
    for (int g = 0; g < p->num_giocatori; g++) {
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"nome\": \"%s\",\n",         p->giocatori[g].nome);
        fprintf(fp, "      \"e_umano\": %d,\n",          p->giocatori[g].e_umano);
        fprintf(fp, "      \"mano_num_carte\": %d,\n",   p->giocatori[g].mano.num_carte);
        fprintf(fp, "      \"mano\": [\n");
        for (int i = 0; i < p->giocatori[g].mano.num_carte; i++) {
            fprintf(fp, "        {\"colore\": \"%s\", \"tipo\": \"%s\", \"colore_scelto\": \"%s\"}%s\n",
                    colore_in_stringa(p->giocatori[g].mano.carte[i].colore),
                    tipo_in_stringa(p->giocatori[g].mano.carte[i].tipo),
                    colore_in_stringa(p->giocatori[g].mano.carte[i].colore_scelto),
                    (i < p->giocatori[g].mano.num_carte - 1) ? "," : "");
        }
        fprintf(fp, "      ]\n");
        fprintf(fp, "    }%s\n", (g < p->num_giocatori - 1) ? "," : "");
    }
    fprintf(fp, "  ]\n");

    /* Stato UI extra */
    fprintf(fp, "  ,\"nome_input\": \"%s\",\n",           ui->nome_input);
    fprintf(fp, "  \"num_avversari_setup\": %d\n",         ui->num_avversari_setup);
    fprintf(fp, "}\n");
    fclose(fp);
}

/* ═══════════════════════════════════════════
   CARICAMENTO PARTITA
═══════════════════════════════════════════ */

/* Deserializza la partita dal file JSON. Restituisce 1 se riuscito, 0 altrimenti */
int carica_partita_json(StatoUI *ui) {
    char *json = leggi_file_in_buffer(FILE_SALVATAGGIO);
    if (!json) return 0;

    StatoPartita *p = &ui->partita;
    const char   *ptr = json;

#define CERCA(chiave, len) \
    while (*ptr && strncmp(ptr, chiave, len) != 0) ptr++; \
    if (!*ptr) { free(json); return 0; } \
    ptr += len

    /* Numero di giocatori */
    CERCA("\"num_giocatori\"", 15);
    while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
    p->num_giocatori = leggi_intero_json(ptr, &ptr);

    /* Turno corrente */
    CERCA("\"turno_corrente\"", 16);
    while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
    p->turno_corrente = leggi_intero_json(ptr, &ptr);

    /* Direzione (-1 o +1; leggiamo come intero, gestendo il segno meno) */
    CERCA("\"direzione\"", 11);
    while (*ptr && *ptr != '-' && (*ptr < '0' || *ptr > '9')) ptr++;
    p->direzione = leggi_intero_json(ptr, &ptr);

    /* Carta sul tavolo */
    CERCA("\"carta_tavolo\"", 14);
    while (*ptr && *ptr != '{') ptr++;
    p->carta_tavolo = analizza_carta_json(&ptr);

    /* Numero carte nel mazzo */
    CERCA("\"mazzo_num_carte\"", 17);
    while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
    p->mazzo.num_carte = leggi_intero_json(ptr, &ptr);

    /* Array del mazzo */
    CERCA("\"mazzo\"", 7);
    while (*ptr && *ptr != '[') ptr++;
    if (*ptr) ptr++;
    for (int i = 0; i < p->mazzo.num_carte; i++) {
        while (*ptr && *ptr != '{') ptr++;
        if (*ptr) p->mazzo.carte[i] = analizza_carta_json(&ptr);
    }

    /* Numero carte negli scarti */
    CERCA("\"scarti_num_carte\"", 18);
    while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
    p->scarti.num_carte = leggi_intero_json(ptr, &ptr);

    /* Array degli scarti */
    CERCA("\"scarti\"", 8);
    while (*ptr && *ptr != '[') ptr++;
    if (*ptr) ptr++;
    for (int i = 0; i < p->scarti.num_carte; i++) {
        while (*ptr && *ptr != '{') ptr++;
        if (*ptr) p->scarti.carte[i] = analizza_carta_json(&ptr);
    }

    /* Giocatori */
    CERCA("\"giocatori\"", 11);
    while (*ptr && *ptr != '[') ptr++;
    if (*ptr) ptr++;

    for (int g = 0; g < p->num_giocatori; g++) {
        CERCA("\"nome\"", 6);
        while (*ptr && *ptr != '"') ptr++;
        ptr = leggi_stringa_json(ptr, p->giocatori[g].nome, MAX_NOME);

        CERCA("\"e_umano\"", 9);
        while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
        p->giocatori[g].e_umano = leggi_intero_json(ptr, &ptr);

        CERCA("\"mano_num_carte\"", 16);
        while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
        p->giocatori[g].mano.num_carte = leggi_intero_json(ptr, &ptr);

        CERCA("\"mano\"", 6);
        while (*ptr && *ptr != '[') ptr++;
        if (*ptr) ptr++;
        for (int i = 0; i < p->giocatori[g].mano.num_carte; i++) {
            while (*ptr && *ptr != '{') ptr++;
            if (*ptr) p->giocatori[g].mano.carte[i] = analizza_carta_json(&ptr);
        }
    }

#undef CERCA

    /* Nome input */
    while (*ptr && strncmp(ptr, "\"nome_input\"", 12) != 0) ptr++;
    if (*ptr) {
        ptr += 12;
        while (*ptr && *ptr != '"') ptr++;
        ptr = leggi_stringa_json(ptr, ui->nome_input, MAX_NOME);
        ui->lunghezza_nome = (int)strlen(ui->nome_input);
    }

    /* Numero avversari */
    while (*ptr && strncmp(ptr, "\"num_avversari_setup\"", 21) != 0) ptr++;
    if (*ptr) {
        ptr += 21;
        while (*ptr && (*ptr < '0' || *ptr > '9')) ptr++;
        ui->num_avversari_setup = leggi_intero_json(ptr, &ptr);
    }

    /* Reset stato UI post-caricamento */
    ui->carta_selezionata   = -1;
    ui->hover_carta         = -1;
    ui->hover_pesca         = 0;
    ui->carta_da_giocare    = -1;
    ui->messaggio[0]        = '\0';
    ui->timer_messaggio     = 0;
    ui->ritardo_cpu         = 0;
    ui->mostra_pulsante_uno = 0;
    ui->ha_cliccato_uno     = 0;
    ui->mostra_riciclo_mazzo = 0;

    free(json);
    elimina_salvataggio(); /* Il salvataggio viene consumato dopo il caricamento */
    return 1;
}

/* Rimuove il file di salvataggio dal filesystem */
void elimina_salvataggio(void) {
    remove(FILE_SALVATAGGIO);
}
