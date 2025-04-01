#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "index.h"

static DocumentMeta docs[MAX_DOCUMENTS];
static int doc_count = 0;
static int next_id = 1;
const char *PERSIST_FILE = "meta_data.txt";

// Add a new document. Returns the new document’s id or -1 on error.
int index_add(const char *title, const char *authors, const char *year, const char *path) {
    if (doc_count >= MAX_DOCUMENTS)
        return -1; // No space available.

    docs[doc_count].id = next_id++;
    strncpy(docs[doc_count].title, title, MAX_TITLE);
    strncpy(docs[doc_count].authors, authors, MAX_AUTHORS);
    strncpy(docs[doc_count].year, year, MAX_YEAR);
    strncpy(docs[doc_count].path, path, MAX_PATH);
    doc_count++;

    // Append the new document to the persistent file.
    int fd = open(PERSIST_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        char buffer[1024];
        int len = snprintf(buffer, sizeof(buffer), "%d|%s|%s|%s|%s\n", docs[doc_count-1].id, title, authors, year, path);
        write(fd, buffer, len);
        close(fd);
    }
    return docs[doc_count-1].id;
}

// Return pointer to document meta-information given an id (or NULL if not found).
DocumentMeta* index_query(int id) {
    for (int i = 0; i < doc_count; i++) {
        if (docs[i].id == id)
            return &docs[i];
    }
    return NULL;
}

// Remove document meta-information with the given id. Returns 0 if successful.
int index_remove(int id) {
    int found = 0;
    for (int i = 0; i < doc_count; i++) {
        if (docs[i].id == id) {
            found = 1;
        }
        if (found && i < doc_count - 1) {
            docs[i] = docs[i + 1];
        }
    }
    if (found) {
        doc_count--;
        // Rewrite the persistent file.
        int fd = open(PERSIST_FILE, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        if (fd != -1) {
            for (int i = 0; i < doc_count; i++) {
                char buffer[1024];
                int len = snprintf(buffer, sizeof(buffer), "%d|%s|%s|%s|%s\n", docs[i].id, docs[i].title, docs[i].authors, docs[i].year, docs[i].path);
                write(fd, buffer, len);
            }
            close(fd);
        }
        return 0;
    }
    return -1; // Not found.
}

// Load meta-information from the persistent file.
int index_load(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        return -1;

    char buf[1024];
    int bytes = read(fd, buf, sizeof(buf)-1);
    if (bytes > 0) {
        buf[bytes] = '\0';
        char *line = strtok(buf, "\n");
        while (line && doc_count < MAX_DOCUMENTS) {
            int id;
            char title[MAX_TITLE+1], authors[MAX_AUTHORS+1], year[MAX_YEAR+1], path[MAX_PATH+1];
            if (sscanf(line, "%d|%200[^|]|%200[^|]|%4[^|]|%64s", &id, title, authors, year, path) == 5) {
                docs[doc_count].id = id;
                strncpy(docs[doc_count].title, title, MAX_TITLE);
                strncpy(docs[doc_count].authors, authors, MAX_AUTHORS);
                strncpy(docs[doc_count].year, year, MAX_YEAR);
                strncpy(docs[doc_count].path, path, MAX_PATH);
                if (id >= next_id) next_id = id + 1;
                doc_count++;
            }
            line = strtok(NULL, "\n");
        }
    }
    close(fd);
    return 0;
}

// Save meta-information to the persistent file.
int index_save(const char *filename) {
    int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd == -1)
        return -1;
    for (int i = 0; i < doc_count; i++) {
        char buffer[1024];
        int len = snprintf(buffer, sizeof(buffer), "%d|%s|%s|%s|%s\n", docs[i].id, docs[i].title, docs[i].authors, docs[i].year, docs[i].path);
        write(fd, buffer, len);
    }
    close(fd);
    return 0;
}
