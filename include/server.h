#ifndef SERVER_H
#define SERVER_H

// Here you can declare functions used by the server for each operation.
void handle_add(Message *msg);
void handle_query(Message *msg);
void handle_remove(Message *msg);
void handle_line_count(Message *msg);
void handle_search(Message *msg);
void handle_shutdown(Message *msg);

#endif
