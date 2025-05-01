#include "common.h"
#include "server.h"
#include "index.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

CacheEntry cache[MAX_CACHE];
int cache_size = 0;
int next_id = 1;
char document_folder[256] = {0};
extern void cache_print_stats();
extern void cache_export_snapshot(const char *filename);


void send_response(const char *client_fifo, const char *response) {
    int fd = open(client_fifo, O_WRONLY);
    if (fd != -1) {
        write(fd, response, strlen(response));
        close(fd);
    }
}

void handle_add(Message *msg) {
    char title[MAX_TITLE+1], authors[MAX_AUTHORS+1], year[MAX_YEAR+1], path[MAX_PATH+1];
    
    sscanf(msg->args, "%200[^|]|%200[^|]|%4[^|]|%64[^|]", title, authors, year, path);
    
    char fullpath[512];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", document_folder, path);

    char response[RESPONSE_SIZE];
    if (access(fullpath, F_OK) == -1) {
        snprintf(response, sizeof(response), "Erro: Arquivo %s n\u00e3o encontrado", path);
        send_response(msg->client_fifo, response);
        return;
    }

    int id = index_add(title, authors, year, path);
    if (id > 0) {
        snprintf(response, sizeof(response), "Document added with ID: %d", id);
        index_save("data/index.txt");
    } else {
        snprintf(response, sizeof(response), "Error adding document");
    }
    send_response(msg->client_fifo, response);
}

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

void handle_remove(Message *msg) {
    int id = atoi(msg->args);
    char response[RESPONSE_SIZE];

    if (index_remove(id) == 0) {
        snprintf(response, sizeof(response), "Document %d removed", id);
        index_save("data/index.txt");
    } else {
        snprintf(response, sizeof(response), "Document %d not found", id);
    }
    send_response(msg->client_fifo, response);
}

void handle_line_count(Message *msg) {
    char *token = strtok(msg->args, "|");
    int id = atoi(token);
    token = strtok(NULL, "|");
    char keyword[128] = {0};
    if (token) strncpy(keyword, token, sizeof(keyword));

    DocumentMeta *doc = index_query(id);
    char response[RESPONSE_SIZE];

    if (!doc) {
        snprintf(response, sizeof(response), "Document %d not found", id);
        send_response(msg->client_fifo, response);
        return;
    }

    char fullpath[512];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", document_folder, doc->path);

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        snprintf(response, sizeof(response), "Pipe error");
        send_response(msg->client_fifo, response);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Processo filho
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execlp("grep", "grep", "-c", keyword, fullpath, (char *)NULL);
        _exit(1);
    } else {
        // Processo pai
        close(pipefd[1]);
        char buf[64] = {0};
        read(pipefd[0], buf, sizeof(buf)-1);
        close(pipefd[0]);
        waitpid(pid, NULL, 0);

        snprintf(response, sizeof(response), "%s", strlen(buf) > 0 ? buf : "0");
        send_response(msg->client_fifo, response);
    }
}

void handle_search(Message *msg) {
    char *keyword = strtok(msg->args, "|");
    char *nproc_str = strtok(NULL, "|");

    int nproc = (nproc_str != NULL) ? atoi(nproc_str) : 0;
    int total = index_total();

    // Aloca espaço dinâmico seguro
    char *result = malloc(65536);
    if (!result) {
        send_response(msg->client_fifo, "[]");
        return;
    }
    strcpy(result, "[");

    // ---------- MODO SEQUENCIAL ----------
    if (nproc <= 0 || nproc == 1 || total <= 1) {
        int first = 1;

        for (int i = 0; i < total; i++) {
            DocumentMeta *doc = index_get(i);
            if (!doc) continue;

            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", document_folder, doc->path);

            FILE *fp = fopen(fullpath, "r");
            if (!fp) continue;

            char line[512];
            int found = 0;
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, keyword)) {
                    found = 1;
                    break;
                }
            }
            fclose(fp);

            if (found) {
                if (!first) strncat(result, ", ", 65536 - strlen(result) - 1);
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", doc->id);
                strncat(result, buf, 65536 - strlen(result) - 1);
                first = 0;
            }
        }

        strncat(result, "]", 65536 - strlen(result) - 1);
        send_response(msg->client_fifo, strlen(result) > 2 ? result : "[]");
        free(result);
        return;
    }

    // ---------- MODO CONCORRENTE ----------
    int fds[nproc][2];
    pid_t pids[nproc];
    int docs_per_proc = total / nproc;
    int rest = total % nproc;

    for (int i = 0, start = 0; i < nproc; i++) {
        int count = docs_per_proc + (i < rest ? 1 : 0);
        pipe(fds[i]);

        if ((pids[i] = fork()) == 0) {
            // Filho
            close(fds[i][0]);

            char partial[4096] = "";
            int first = 1;

            for (int j = 0; j < count; j++) {
                int index = start + j;
                DocumentMeta *doc = index_get(index);
                if (!doc) continue;

                char fullpath[512];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", document_folder, doc->path);

                FILE *fp = fopen(fullpath, "r");
                if (!fp) continue;

                char line[512];
                int found = 0;
                while (fgets(line, sizeof(line), fp)) {
                    if (strstr(line, keyword)) {
                        found = 1;
                        break;
                    }
                }
                fclose(fp);

                if (found) {
                    if (!first) strncat(partial, ", ", sizeof(partial) - strlen(partial) - 1);
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%d", doc->id);
                    strncat(partial, buf, sizeof(partial) - strlen(partial) - 1);
                    first = 0;
                }
            }

            write(fds[i][1], partial, strlen(partial));
            close(fds[i][1]);
            exit(0);
        } else {
            // Pai
            close(fds[i][1]);
            start += count;
        }
    }

    int first = 1;
    for (int i = 0; i < nproc; i++) {
        char buffer[4096] = {0};
        read(fds[i][0], buffer, sizeof(buffer) - 1);
        close(fds[i][0]);
        waitpid(pids[i], NULL, 0);

        if (strlen(buffer) > 0) {
            if (!first) strncat(result, ", ", 65536 - strlen(result) - 1);
            strncat(result, buffer, 65536 - strlen(result) - 1);
            first = 0;
        }
    }

    strncat(result, "]", 65536 - strlen(result) - 1);
    send_response(msg->client_fifo, strlen(result) > 2 ? result : "[]");
    free(result);
}


void handle_shutdown(Message *msg) {
    char response[RESPONSE_SIZE];
    snprintf(response, sizeof(response), "Server shutting down");
    send_response(msg->client_fifo, response);
    index_save("data/index.txt");
    cache_print_stats();
    unlink(FIFO_SERVER);
    cache_export_snapshot("data/cache_snapshot.txt");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <document_folder> [cache_size]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (index_load("data/index.txt") == 0) {
        printf("[INFO] Índice carregado com sucesso.\n");
    } else {
        printf("[INFO] Nenhum índice carregado.\n");
    }

    strncpy(document_folder, argv[1], sizeof(document_folder));
    mkdir(document_folder, 0777);

    if (argc >= 3) {
        cache_size = atoi(argv[2]);
        if (cache_size > MAX_CACHE) cache_size = MAX_CACHE;
    }

    unlink(FIFO_SERVER);
    if (mkfifo(FIFO_SERVER, 0666) == -1) {
        perror("mkfifo");
        return EXIT_FAILURE;
    }

    printf("Server started. Document folder: %s\n", document_folder);
    printf("Loaded %d documents. Cache size: %d\n", index_get_count(), cache_size);

    int fd = open(FIFO_SERVER, O_RDWR);
    if (fd == -1) {
        perror("open FIFO");
        unlink(FIFO_SERVER);
        return EXIT_FAILURE;
    }

    Message msg;
    while (1) {
        ssize_t bytes = read(fd, &msg, sizeof(msg));
        if (bytes <= 0) continue;

        switch (msg.command) {
            case CMD_ADD: handle_add(&msg); break;
            case CMD_QUERY: handle_query(&msg); break;
            case CMD_REMOVE: handle_remove(&msg); break;
            case CMD_LINE_COUNT: handle_line_count(&msg); break;
            case CMD_SHUTDOWN: handle_shutdown(&msg); break;
            default:
                fprintf(stderr, "Unknown command\n");
                break;
        }
    }

    close(fd);
    return EXIT_SUCCESS;
}

