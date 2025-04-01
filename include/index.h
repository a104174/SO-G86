#ifndef INDEX_H
#define INDEX_H

#include "common.h"

#define MAX_DOCUMENTS 100

// Functions for managing document meta-information.
int index_add(const char *title, const char *authors, const char *year, const char *path);
DocumentMeta* index_query(int id);
int index_remove(int id);

// Functions for persistence (load/save from disk).
int index_load(const char *filename);
int index_save(const char *filename);

#endif
