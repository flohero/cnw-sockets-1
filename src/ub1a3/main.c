//
// Created by florian on 18.12.20.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "../WordCheck.c"

#define MAX_CONN 1

typedef enum {
    info,
    error
} log_level;

void logger(log_level lvl, char *msg) {
    switch (lvl) {
        case info:
            printf("[INFO] %s\n", msg);
            break;
        case error:
            fprintf(stderr, "[ERR] %s\n", msg);
            break;
        default:
            break;
    }
}

void log_with_template(log_level lvl, char *template, const void *msg) {
    switch (lvl) {
        case info:
            printf("[INFO] ");
            printf(template, msg);
            printf("\n");
            break;
        case error:
            fprintf(stderr, "[ERR] ");
            fprintf(stderr, template, msg);
            fprintf(stderr, "\n");
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    if (argc != 2) {
        log_with_template(error, "Usage: %s port", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(NULL, argv[1], &hints, &result);
    if (s != 0) {
        log_with_template(error, "getaddrinfo: %s", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
            logger(error, "Was not able to rebind address!");
            exit(1);
        }
        if (sfd == -1) {
            continue;
        }
        // bind the socket to a local port
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;                  /* Success */
        }
        close(sfd);
    }

    // No address worked
    if (rp == NULL) {
        logger(error, "Was not able to bind address");
        exit(EXIT_FAILURE);
    }
    logger(info, "Bound successfully");

    freeaddrinfo(result);

    if (listen(sfd, MAX_CONN) < 0) {
        logger(error, "Was not able to listen!");
        exit(EXIT_FAILURE);
    }
    logger(info, "Listening...");

    /* Wait for incoming connection requests */
    for (;;) {
        logger(info, "Wait for incoming connection requests");
        int cfd = accept(sfd, rp->ai_addr, &rp->ai_addrlen);
        if (cfd < 0) {
            logger(error, "Was not able to connect!");
        } else {
            log_with_template(info, "Accepted with cfd: %d", cfd);

            // sock_addr to ip representation
            struct sockaddr_in *sock_in = (struct sockaddr_in *) rp->ai_addr;
            char ip_addr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(sock_in->sin_addr), ip_addr, INET_ADDRSTRLEN);

            log_with_template(info, "IP Address: %s", ip_addr);

            serve(cfd);
            close(cfd);
            logger(info, "Disconnected...");
        }
    }
}

