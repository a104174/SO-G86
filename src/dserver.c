#include "common.h"
#include "server.h"
#include "index.h"

DocumentMeta docs[MAX_DOCUMENTS];
CacheEntry cache[MAX_CACHE];
int doc_count = 0;
int cache_size = 0;
int next_id = 1;
char document_folder[256] = {0};

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
    
    int id = index_add(title, authors, year, path);
    char response[RESPONSE_SIZE];
    
    if (id != -1) {
        snprintf(response, sizeof(response), "Document %d indexed", id);
    } else {
        snprintf(response, sizeof(response), "Error indexing document");
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

    int fd_pipe[2];
    if (pipe(fd_pipe) == -1) {
        snprintf(response, sizeof(response), "Pipe error");
        send_response(msg->client_fifo, response);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(fd_pipe[0]);
        dup2(fd_pipe[1], STDOUT_FILENO);
        close(fd_pipe[1]);
        
        char *grep_args[] = {"grep", "-c", keyword, doc->path, NULL};
        execvp("grep", grep_args);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        close(fd_pipe[1]);
        char count_buf[64];
        ssize_t n = read(fd_pipe[0], count_buf, sizeof(count_buf)-1);
        
        if (n > 0) {
            count_buf[n] = '\0';
            snprintf(response, sizeof(response), "%s", count_buf);
        } else {
            snprintf(response, sizeof(response), "0");
        }
        
        close(fd_pipe[0]);
        wait(NULL);
        send_response(msg->client_fifo, response);
    } else {
        snprintf(response, sizeof(response), "Fork error");
        send_response(msg->client_fifo, response);
    }
}

void handle_search(Message *msg) {
    char keyword[128] = {0};
    int nr_processes = 1;
    char *token = strtok(msg->args, "|");
    
    if (token) strncpy(keyword, token, sizeof(keyword));
    token = strtok(NULL, "|");
    if (token) nr_processes = atoi(token);
    
    if (nr_processes < 1) nr_processes = 1;
    if (nr_processes > 8) nr_processes = 8;

    int docs_per_process = doc_count / nr_processes;
    int pipes[nr_processes][2];
    pid_t pids[nr_processes];
    char final_result[32768] = {0};

    for (int i = 0; i < nr_processes; i++) {
        if (pipe(pipes[i]) == -1) {
            send_response(msg->client_fifo, "Pipe creation failed");
            return;
        }

        pids[i] = fork();
        if (pids[i] == 0) {
            close(pipes[i][0]);
            char result[4096] = {0};
            int start = i * docs_per_process;
            int end = (i == nr_processes-1) ? doc_count : start + docs_per_process;
            
            for (int j = start; j < end; j++) {
                char cmd[512];
                snprintf(cmd, sizeof(cmd), "grep -q \"%s\" \"%s/%s\"", 
                        keyword, document_folder, docs[j].path);
                
                if (system(cmd) == 0) {
                    char entry[256];
                    snprintf(entry, sizeof(entry), "%d: %s\n", docs[j].id, docs[j].title);
                    strcat(result, entry);
                }
            }
            
            write(pipes[i][1], result, strlen(result)+1);
            close(pipes[i][1]);
            exit(EXIT_SUCCESS);
        } else {
            close(pipes[i][1]);
        }
    }

    for (int i = 0; i < nr_processes; i++) {
        char buffer[4096];
        ssize_t n = read(pipes[i][0], buffer, sizeof(buffer));
        if (n > 0) {
            strcat(final_result, buffer);
        }
        close(pipes[i][0]);
        waitpid(pids[i], NULL, 0);
    }

    if (strlen(final_result) > 0) {
        // Send header first
        send_response(msg->client_fifo, "Matches:");
        
        // Send results in chunks
        char *chunk = final_result;
        while (*chunk) {
            char response[RESPONSE_SIZE];
            int chunk_len = strlen(chunk);
            int send_len = chunk_len > RESPONSE_SIZE-1 ? RESPONSE_SIZE-1 : chunk_len;
            
            strncpy(response, chunk, send_len);
            response[send_len] = '\0';
            send_response(msg->client_fifo, response);
            
            chunk += send_len;
        }
    } else {
        send_response(msg->client_fifo, "No matches found");
    }
}

void handle_shutdown(Message *msg) {
    char response[RESPONSE_SIZE];
    snprintf(response, sizeof(response), "Server shutting down");
    send_response(msg->client_fifo, response);
    
    index_save("meta_data.txt");
    unlink(FIFO_SERVER);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <document_folder> [cache_size]\n", argv[0]);
        return EXIT_FAILURE;
    }

    strncpy(document_folder, argv[1], sizeof(document_folder));
    mkdir(document_folder, 0777);
    
    if (argc >= 3) {
        cache_size = atoi(argv[2]);
        if (cache_size > MAX_CACHE) cache_size = MAX_CACHE;
    }

    index_load("meta_data.txt");
    unlink(FIFO_SERVER);
    
    if (mkfifo(FIFO_SERVER, 0666) == -1) {
        perror("mkfifo");
        return EXIT_FAILURE;
    }

    printf("Server started. Document folder: %s\n", document_folder);
    printf("Loaded %d documents. Cache size: %d\n", doc_count, cache_size);

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
            case CMD_SEARCH: handle_search(&msg); break;
            case CMD_SHUTDOWN: handle_shutdown(&msg); break;
            default:
                fprintf(stderr, "Unknown command\n");
                break;
        }
    }

    close(fd);
    return EXIT_SUCCESS;
}