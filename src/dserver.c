#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"
#include "server.h"
#include "index.h"

#define RESPONSE_SIZE 1024

// Utility function to send response back to a client.
void send_response(const char *client_fifo, const char *response) {
    int fd = open(client_fifo, O_WRONLY);
    if (fd != -1) {
        write(fd, response, strlen(response));
        close(fd);
    }
}

// Handler for adding a document.
void handle_add(Message *msg) {
    // Parse arguments: "title|authors|year|path"
    char title[MAX_TITLE+1], authors[MAX_AUTHORS+1], year[MAX_YEAR+1], path[MAX_PATH+1];
    sscanf(msg->args, "%200[^|]|%200[^|]|%4[^|]|%64[^|]", title, authors, year, path);
    int id = index_add(title, authors, year, path);
    char response[RESPONSE_SIZE];
    if (id != -1)
        snprintf(response, sizeof(response), "Document %d indexed", id);
    else
        snprintf(response, sizeof(response), "Error indexing document");
    send_response(msg->client_fifo, response);
}

// Handler for querying a document.
void handle_query(Message *msg) {
    int id = atoi(msg->args);
    DocumentMeta *doc = index_query(id);
    char response[RESPONSE_SIZE];
    if (doc) {
        snprintf(response, sizeof(response),
                 "Title: %s\nAuthors: %s\nYear: %s\nPath: %s",
                 doc->title, doc->authors, doc->year, doc->path);
    } else {
        snprintf(response, sizeof(response), "Document %d not found", id);
    }
    send_response(msg->client_fifo, response);
}

// Handler for removing a document.
void handle_remove(Message *msg) {
    int id = atoi(msg->args);
    char response[RESPONSE_SIZE];
    if (index_remove(id) == 0)
        snprintf(response, sizeof(response), "Index entry %d deleted", id);
    else
        snprintf(response, sizeof(response), "Document %d not found", id);
    send_response(msg->client_fifo, response);
}

// Handler for counting lines containing a keyword.
void handle_line_count(Message *msg) {
    // Parse arguments: "key|keyword"
    char *token = strtok(msg->args, "|");
    int id = atoi(token);
    token = strtok(NULL, "|");
    char keyword[128];
    if (token)
        strncpy(keyword, token, sizeof(keyword));
    else
        strcpy(keyword, "");

    DocumentMeta *doc = index_query(id);
    char response[RESPONSE_SIZE];

    if (!doc) {
        snprintf(response, sizeof(response), "Document %d not found", id);
        send_response(msg->client_fifo, response);
        return;
    }

    // Create a pipe to capture the output from grep.
    int fd_pipe[2];
    if (pipe(fd_pipe) == -1) {
        snprintf(response, sizeof(response), "Pipe error");
        send_response(msg->client_fifo, response);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child: execute grep -c "keyword" <doc->path>
        close(fd_pipe[0]);
        dup2(fd_pipe[1], STDOUT_FILENO);
        close(fd_pipe[1]);
        char *grep_args[] = {"grep", "-c", keyword, doc->path, NULL};
        execvp("grep", grep_args);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent: read result from pipe.
        close(fd_pipe[1]);
        char count_buf[64];
        int n = read(fd_pipe[0], count_buf, sizeof(count_buf)-1);
        if (n > 0) {
            count_buf[n] = '\0';
            snprintf(response, sizeof(response), "%s", count_buf);
        } else {
            snprintf(response, sizeof(response), "Error reading line count");
        }
        close(fd_pipe[0]);
        wait(NULL);
        send_response(msg->client_fifo, response);
    } else {
        snprintf(response, sizeof(response), "Fork error");
        send_response(msg->client_fifo, response);
    }
}

// Handler for searching documents containing a keyword.
void handle_search(Message *msg) {
    // Parse arguments: "keyword" or "keyword|nr_processes"
    char keyword[128] = {0};
    int nr_processes = 1;
    char *token = strtok(msg->args, "|");
    if (token)
        strncpy(keyword, token, sizeof(keyword));
    token = strtok(NULL, "|");
    if (token)
        nr_processes = atoi(token);

    // For each document, we check if it contains the keyword.
    char result[RESPONSE_SIZE] = "[";
    char temp[32];
    int first = 1;
    for (int i = 0; i < MAX_DOCUMENTS; i++) {
        DocumentMeta *doc = index_query(i+1);
        if (!doc) 
            continue;

        // Use fork+exec to call grep -q "keyword" doc->path.
        int status;
        pid_t pid = fork();
        if (pid == 0) {
            execlp("grep", "grep", "-q", keyword, doc->path, NULL);
            exit(1);
        }
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            if (!first)
                strncat(result, ", ", sizeof(result) - strlen(result) - 1);
            snprintf(temp, sizeof(temp), "%d", doc->id);
            strncat(result, temp, sizeof(result) - strlen(result) - 1);
            first = 0;
        }
    }
    strncat(result, "]", sizeof(result) - strlen(result) - 1);
    send_response(msg->client_fifo, result);
}

// Handler for shutting down the server.
void handle_shutdown(Message *msg) {
    char response[RESPONSE_SIZE];
    snprintf(response, sizeof(response), "Server is shutting down");
    send_response(msg->client_fifo, response);
    // Save persistent meta-information.
    index_save("meta_data.txt");
    // Remove the server FIFO and exit.
    unlink(FIFO_SERVER);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <document_folder> <cache_size>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // (Opcional) podes guardar estes argumentos para usar no cache.
    char *document_folder = argv[1];
    int cache_size = atoi(argv[2]);

    // Debug prints para confirmar arranque
    printf("[SERVER] Starting server...\n");
    fflush(stdout);

    // Remove o FIFO antigo, se existir
    unlink(FIFO_SERVER);

    // Criar FIFO
    if (mkfifo(FIFO_SERVER, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    printf("[SERVER] FIFO created: %s\n", FIFO_SERVER);
    fflush(stdout);

    // Carrega persistência
    index_load("meta_data.txt");

    // IMPORTANTE: abrir o FIFO em O_RDWR para não fechar automaticamente se não houver writers
    int fd = open(FIFO_SERVER, O_RDWR);
    if (fd == -1) {
        perror("open FIFO");
        unlink(FIFO_SERVER);
        exit(EXIT_FAILURE);
    }
    printf("[SERVER] FIFO opened in O_RDWR mode. Waiting for requests...\n");
    fflush(stdout);

    Message msg;
    // Ciclo principal de leitura
    while (1) {
        ssize_t bytes = read(fd, &msg, sizeof(msg));
        if (bytes == 0) {
            // EOF no FIFO (se o último writer fechar)
            // Em macOS ou Linux pode acontecer se não houver clientes a escrever
            printf("[SERVER] EOF on FIFO. No more writers. Still running...\n");
            fflush(stdout);
            // Para evitar sair, podemos continuar o loop, mas é arriscado.
            // Se quiseres sair do servidor se não houver writers, faz break.
            continue;
        }
        if (bytes < 0) {
            perror("[SERVER] read error");
            break; // ou continua
        }

        // Processar a mensagem
        switch (msg.command) {
            case CMD_ADD:
                handle_add(&msg);
                break;
            case CMD_QUERY:
                handle_query(&msg);
                break;
            case CMD_REMOVE:
                handle_remove(&msg);
                break;
            case CMD_LINE_COUNT:
                handle_line_count(&msg);
                break;
            case CMD_SEARCH:
                handle_search(&msg);
                break;
            case CMD_SHUTDOWN:
                handle_shutdown(&msg);
                break;
            default:
                fprintf(stderr, "[SERVER] Unknown command\n");
                break;
        }
    }

    // Fechar FIFO e remover
    close(fd);
    unlink(FIFO_SERVER);
    printf("[SERVER] Shutting down normally.\n");
    return 0;
}
