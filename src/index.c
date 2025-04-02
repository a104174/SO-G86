#include "common.h"
#include "index.h"

static DocumentMeta docs[MAX_DOCUMENTS];
static int doc_count = 0;
static int next_id = 1;

int index_add(const char *title, const char *authors, const char *year, const char *path) {
    if (doc_count >= MAX_DOCUMENTS) return -1;

    docs[doc_count].id = next_id++;
    strncpy(docs[doc_count].title, title, MAX_TITLE);
    strncpy(docs[doc_count].authors, authors, MAX_AUTHORS);
    strncpy(docs[doc_count].year, year, MAX_YEAR);
    strncpy(docs[doc_count].path, path, MAX_PATH);
    doc_count++;
    
    return docs[doc_count-1].id;
}

DocumentMeta* index_query(int id) {
    for (int i = 0; i < doc_count; i++) {
        if (docs[i].id == id) return &docs[i];
    }
    return NULL;
}

int index_remove(int id) {
    int found = 0;
    for (int i = 0; i < doc_count; i++) {
        if (docs[i].id == id) found = 1;
        if (found && i < doc_count-1) docs[i] = docs[i+1];
    }
    return found ? doc_count-- : -1;
}

int index_load(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1;

    char line[1024];
    while (fgets(line, sizeof(line), fp) && doc_count < MAX_DOCUMENTS) {
        int id;
        char title[MAX_TITLE+1], authors[MAX_AUTHORS+1], year[MAX_YEAR+1], path[MAX_PATH+1];
        
        if (sscanf(line, "%d|%200[^|]|%200[^|]|%4[^|]|%64s", 
                  &id, title, authors, year, path) == 5) {
            docs[doc_count].id = id;
            strncpy(docs[doc_count].title, title, MAX_TITLE);
            strncpy(docs[doc_count].authors, authors, MAX_AUTHORS);
            strncpy(docs[doc_count].year, year, MAX_YEAR);
            strncpy(docs[doc_count].path, path, MAX_PATH);
            
            if (id >= next_id) next_id = id + 1;
            doc_count++;
        }
    }
    fclose(fp);
    return 0;
}

int index_save(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return -1;

    for (int i = 0; i < doc_count; i++) {
        fprintf(fp, "%d|%s|%s|%s|%s\n", 
               docs[i].id, docs[i].title, docs[i].authors, docs[i].year, docs[i].path);
    }
    fclose(fp);
    return 0;
}