#ifndef INDEX_H
#define INDEX_H

int index_add(const char *title, const char *authors, const char *year, const char *path);
DocumentMeta* index_query(int id);
int index_remove(int id);
int index_load(const char *filename);
int index_save(const char *filename);

#endif