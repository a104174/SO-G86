#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "common.h"
#include "client.h"

#define RESPONSE_SIZE 1024

// Helper function to print usage
void usage(const char *prog) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s -a \"title\" \"authors\" \"year\" \"path\"\n", prog);
    fprintf(stderr, "  %s -c \"key\"\n", prog);
    fprintf(stderr, "  %s -d \"key\"\n", prog);
    fprintf(stderr, "  %s -l \"key\" \"keyword\"\n", prog);
    fprintf(stderr, "  %s -s \"keyword\" [nr_processes]\n", prog);
    fprintf(stderr, "  %s -f\n", prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 2)
        usage(argv[0]);

    Message msg;
    memset(&msg, 0, sizeof(msg));

    // Create a unique FIFO name for the client response based on PID.
    char client_fifo[256];
    snprintf(client_fifo, sizeof(client_fifo), "/tmp/dclient_%d_fifo", getpid());
    strncpy(msg.client_fifo, client_fifo, sizeof(msg.client_fifo));
    // Create the FIFO.
    if (mkfifo(client_fifo, 0666) == -1) {
        // If FIFO exists, it is ok.
    }

    // Build the message based on the option.
    if (strcmp(argv[1], "-a") == 0) {
        if (argc != 6)
            usage(argv[0]);
        msg.command = CMD_ADD;
        // Use '|' as delimiter
        snprintf(msg.args, sizeof(msg.args), "%s|%s|%s|%s", argv[2], argv[3], argv[4], argv[5]);
    } else if (strcmp(argv[1], "-c") == 0) {
        if (argc != 3)
            usage(argv[0]);
        msg.command = CMD_QUERY;
        snprintf(msg.args, sizeof(msg.args), "%s", argv[2]);
    } else if (strcmp(argv[1], "-d") == 0) {
        if (argc != 3)
            usage(argv[0]);
        msg.command = CMD_REMOVE;
        snprintf(msg.args, sizeof(msg.args), "%s", argv[2]);
    } else if (strcmp(argv[1], "-l") == 0) {
        if (argc != 4)
            usage(argv[0]);
        msg.command = CMD_LINE_COUNT;
        snprintf(msg.args, sizeof(msg.args), "%s|%s", argv[2], argv[3]);
    } else if (strcmp(argv[1], "-s") == 0) {
        if (argc < 3 || argc > 4)
            usage(argv[0]);
        msg.command = CMD_SEARCH;
        if (argc == 4)
            snprintf(msg.args, sizeof(msg.args), "%s|%s", argv[2], argv[3]);
        else
            snprintf(msg.args, sizeof(msg.args), "%s", argv[2]);
    } else if (strcmp(argv[1], "-f") == 0) {
        msg.command = CMD_SHUTDOWN;
        snprintf(msg.args, sizeof(msg.args), ""); // no arguments
    } else {
        usage(argv[0]);
    }

    // Send message to server FIFO.
    int fd = open(FIFO_SERVER, O_WRONLY);
    if (fd == -1) {
        perror("open server FIFO");
        unlink(client_fifo);
        exit(EXIT_FAILURE);
    }
    write(fd, &msg, sizeof(msg));
    close(fd);

    // Open client FIFO to read server response.
    fd = open(client_fifo, O_RDONLY);
    if (fd == -1) {
        perror("open client FIFO");
        unlink(client_fifo);
        exit(EXIT_FAILURE);
    }
    char response[RESPONSE_SIZE];
    int n = read(fd, response, sizeof(response) - 1);
    if (n > 0) {
        response[n] = '\0';
        printf("%s\n", response);
    }
    close(fd);
    unlink(client_fifo);
    return 0;
}
