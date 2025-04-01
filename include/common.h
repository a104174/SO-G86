#ifndef COMMON_H
#define COMMON_H

#include <limits.h>

#define MAX_TITLE      200
#define MAX_AUTHORS    200
#define MAX_YEAR       4
#define MAX_PATH       64
#define MAX_ARG_SIZE   512
#define FIFO_SERVER    "/tmp/dserver_fifo"

// Define command types that the client can send.
typedef enum {
    CMD_ADD,         // -a "title" "authors" "year" "path"
    CMD_QUERY,       // -c "key"
    CMD_REMOVE,      // -d "key"
    CMD_LINE_COUNT,  // -l "key" "keyword"
    CMD_SEARCH,      // -s "keyword" [nr_processes]
    CMD_SHUTDOWN     // -f
} CommandType;

// Message structure sent from client to server.
typedef struct {
    CommandType command;
    char client_fifo[256];  // Client FIFO path for response.
    char args[MAX_ARG_SIZE]; // Arguments as a single string (fields separated by a known delimiter, e.g., '|')
} Message;

// Structure to store document meta-information.
typedef struct {
    int id;                           // Unique document identifier
    char title[MAX_TITLE + 1];
    char authors[MAX_AUTHORS + 1];
    char year[MAX_YEAR + 1];
    char path[MAX_PATH + 1];
} DocumentMeta;

#endif
