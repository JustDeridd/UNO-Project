/*
 * database.c
 * Gestione della classifica giocatori: connessione MySQL, lettura/scrittura
 * su database e file CSV di fallback, aggiornamento statistiche a fine partita.
 */

#include "database.h"

/* ═══════════════════════════════════════════
   ORDINAMENTO CLASSIFICA
═══════════════════════════════════════════ */

/* Funzione di confronto per qsort: vinte decrescenti, poi percentuale, poi nome */
int confronta_classifica(const void *a, const void *b) {
    const RecordGiocatore *ra = (const RecordGiocatore *)a;
    const RecordGiocatore *rb = (const RecordGiocatore *)b;

    if (rb->vinte != ra->vinte) return rb->vinte - ra->vinte;
    if (rb->percentuale_vittorie > ra->percentuale_vittorie) return  1;
    if (rb->percentuale_vittorie < ra->percentuale_vittorie) return -1;
    return strcmp(ra->nome, rb->nome);
}

/* Ordina i record tramite qsort con confronta_classifica */
void ordina_classifica(RecordGiocatore *arr, int n) {
    qsort(arr, n, sizeof(RecordGiocatore), confronta_classifica);
}

/* Ricalcola la percentuale vittorie in base a vinte e perse */
void aggiorna_percentuale(RecordGiocatore *record) {
    record->totali = record->vinte + record->perse;
    record->percentuale_vittorie = (record->totali > 0)
        ? ((float)record->vinte * 100.0f / (float)record->totali)
        : 0.0f;
}

/* Scansione lineare per trovare un record per nome */
int trova_record(RecordGiocatore *arr, int n, const char *nome) {
    for (int i = 0; i < n; i++)
        if (strcmp(arr[i].nome, nome) == 0)
            return i;
    return -1;
}

/* Aggiunge un record vuoto per il nome se non è già presente */
void aggiungi_o_aggiorna_giocatore(StatoUI *ui, const char *nome) {
    if (!nome || !nome[0]) return;
    if (trova_record(ui->record, ui->num_record, nome) >= 0) return;
    if (ui->num_record >= MAX_RECORD_GIOCATORI) return;

    RecordGiocatore nuovo = {0};
    strncpy(nuovo.nome, nome, MAX_NOME - 1);
    ui->record[ui->num_record++] = nuovo;
}

/* ═══════════════════════════════════════════
   ACCESSO DATABASE MySQL
═══════════════════════════════════════════ */

/* Legge tutti i record dalla tabella MySQL e li carica in memoria */
void carica_giocatori_db(ContestoSDL *ctx, StatoUI *ui) {
    ui->num_record = 0;
    if (!ctx->connessione_db) return;

    const char *query =
        "SELECT nome, partite_vinte, partite_perse, partite_giocate "
        "FROM classifica "
        "ORDER BY partite_vinte DESC, partite_giocate ASC";

    if (mysql_query(ctx->connessione_db, query) != 0) {
        fprintf(stderr, "[DB] Errore query carica: %s\n",
                mysql_error(ctx->connessione_db));
        return;
    }

    MYSQL_RES *risultato = mysql_store_result(ctx->connessione_db);
    if (!risultato) {
        fprintf(stderr, "[DB] Errore store_result: %s\n",
                mysql_error(ctx->connessione_db));
        return;
    }

    MYSQL_ROW riga;
    while ((riga = mysql_fetch_row(risultato)) != NULL) {
        if (ui->num_record >= MAX_RECORD_GIOCATORI) break;

        RecordGiocatore rec = {0};
        strncpy(rec.nome, riga[0] ? riga[0] : "", MAX_NOME - 1);
        rec.vinte  = riga[1] ? atoi(riga[1]) : 0;
        rec.perse  = riga[2] ? atoi(riga[2]) : 0;
        rec.totali = riga[3] ? atoi(riga[3]) : 0;
        aggiorna_percentuale(&rec);

        ui->record[ui->num_record++] = rec;
    }

    mysql_free_result(risultato);
}

/* Inserisce o aggiorna ogni record nel database con ON DUPLICATE KEY UPDATE */
void salva_giocatori_db(ContestoSDL *ctx, StatoUI *ui) {
    if (!ctx->connessione_db) return;

    for (int i = 0; i < ui->num_record; i++) {
        char nome_escaped[MAX_NOME * 2 + 1];
        mysql_real_escape_string(ctx->connessione_db, nome_escaped,
                                 ui->record[i].nome,
                                 (unsigned long)strlen(ui->record[i].nome));

        char query[512];
        snprintf(query, sizeof(query),
            "INSERT INTO classifica (nome, partite_vinte, partite_perse, partite_giocate) "
            "VALUES ('%s', %d, %d, %d) "
            "ON DUPLICATE KEY UPDATE "
            "partite_vinte = %d, partite_perse = %d, partite_giocate = %d",
            nome_escaped,
            ui->record[i].vinte, ui->record[i].perse, ui->record[i].totali,
            ui->record[i].vinte, ui->record[i].perse, ui->record[i].totali);

        if (mysql_query(ctx->connessione_db, query) != 0)
            fprintf(stderr, "[DB] Errore salvataggio '%s': %s\n",
                    ui->record[i].nome, mysql_error(ctx->connessione_db));
    }
}

/* Aggiorna le statistiche del singolo giocatore nel DB con incremento atomico */
void aggiorna_stats_db(ContestoSDL *ctx, const char *nome, int e_vincitore) {
    if (!ctx || !ctx->connessione_db) return;
    if (!nome || !nome[0]) return;

    char nome_escaped[MAX_NOME * 2 + 1];
    mysql_real_escape_string(ctx->connessione_db, nome_escaped,
                             nome, (unsigned long)strlen(nome));

    char query[512];
    snprintf(query, sizeof(query),
        "INSERT INTO classifica (nome, partite_vinte, partite_perse, partite_giocate) "
        "VALUES ('%s', %d, %d, 1) "
        "ON DUPLICATE KEY UPDATE "
        "partite_vinte    = partite_vinte    + %d, "
        "partite_perse    = partite_perse    + %d, "
        "partite_giocate  = partite_giocate  + 1",
        nome_escaped,
        e_vincitore ? 1 : 0,
        e_vincitore ? 0 : 1,
        e_vincitore ? 1 : 0,
        e_vincitore ? 0 : 1);

    if (mysql_query(ctx->connessione_db, query) != 0)
        fprintf(stderr, "[DB] Errore aggiornamento '%s': %s\n",
                nome, mysql_error(ctx->connessione_db));
}

/* ═══════════════════════════════════════════
   FALLBACK CSV
═══════════════════════════════════════════ */

/* Legge il file CSV di backup e carica i record in memoria */
void carica_giocatori(StatoUI *ui) {
    ui->num_record = 0;
    FILE *fp = fopen(FILE_GIOCATORI, "r");
    if (!fp) return;

    char linea[256];
    while (fgets(linea, sizeof(linea), fp)) {
        char *nome    = strtok(linea, ";");
        char *vinte   = strtok(NULL, ";");
        char *perse   = strtok(NULL, ";");
        char *totali  = strtok(NULL, ";");
        char *wr      = strtok(NULL, ";\n");

        if (!nome || !vinte || !perse || !totali) continue;
        if (ui->num_record >= MAX_RECORD_GIOCATORI) break;

        RecordGiocatore rec = {0};
        strncpy(rec.nome, nome, MAX_NOME - 1);
        rec.vinte  = atoi(vinte);
        rec.perse  = atoi(perse);
        rec.totali = atoi(totali);
        rec.percentuale_vittorie = wr ? (float)atof(wr) : 0.0f;
        aggiorna_percentuale(&rec);

        ui->record[ui->num_record++] = rec;
    }

    fclose(fp);
}

/* Scrive tutti i record nel file CSV di backup */
void salva_giocatori(StatoUI *ui) {
    FILE *fp = fopen(FILE_GIOCATORI, "w");
    if (!fp) return;

    for (int i = 0; i < ui->num_record; i++)
        fprintf(fp, "%s;%d;%d;%d;%.2f\n",
                ui->record[i].nome,
                ui->record[i].vinte,
                ui->record[i].perse,
                ui->record[i].totali,
                ui->record[i].percentuale_vittorie);

    fclose(fp);
}

/* ═══════════════════════════════════════════
   STATISTICHE FINE PARTITA
═══════════════════════════════════════════ */

/* Aggiorna in memoria le statistiche di tutti i giocatori umani a fine partita */
void aggiorna_statistiche_fine_partita(StatoUI *ui, const char *nome_vincitore,
                                        int num_giocatori) {
    for (int i = 0; i < num_giocatori; i++) {
        Giocatore *g = &ui->partita.giocatori[i];
        if (!g->e_umano) continue;

        int idx = trova_record(ui->record, ui->num_record, g->nome);
        if (idx < 0) {
            if (ui->num_record >= MAX_RECORD_GIOCATORI) continue;
            idx = ui->num_record++;
            memset(&ui->record[idx], 0, sizeof(RecordGiocatore));
            strncpy(ui->record[idx].nome, g->nome, MAX_NOME - 1);
        }

        ui->record[idx].totali += 1;
        if (strcmp(g->nome, nome_vincitore) == 0)
            ui->record[idx].vinte += 1;
        else
            ui->record[idx].perse += 1;

        aggiorna_percentuale(&ui->record[idx]);
    }

    /* Backup immediato su CSV */
    salva_giocatori(ui);
}
