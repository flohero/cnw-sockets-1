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

extern void serve(int fd);

void print_ip_addr(const struct sockaddr *sock_add);

int main(int argc, char *argv[]) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd, s;

  if (argc != 2) {
    exit(EXIT_FAILURE);
  }

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  s = getaddrinfo(NULL, argv[1], &hints, &result);
  if (s != 0) {
    exit(EXIT_FAILURE);
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
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
    exit(EXIT_FAILURE);
  }
  if (listen(sfd, MAX_CONN) < 0) {
    exit(EXIT_FAILURE);
  }

  /* Wait for incoming connection requests */
  int running = 1;
  while (running) {
    int cfd = accept(sfd, rp->ai_addr, &rp->ai_addrlen);
    if (cfd < 0) {
      running = 0;
    } else {
      print_ip_addr(rp->ai_addr);
      serve(cfd);
      close(cfd);
    }
  }
  close(sfd);
}


void print_ip_addr(const struct sockaddr *sock_add) {
  switch (sock_add->sa_family) {
    case AF_INET: {
      char str_addr[INET_ADDRSTRLEN];
      struct sockaddr_in * sa = (struct sockaddr_in *) sock_add;
      inet_ntop(AF_INET, &(sa->sin_addr), str_addr, INET_ADDRSTRLEN);
      printf("%s:%d\n", str_addr, ntohs(sa->sin_port));
      break;
    }
    case AF_INET6: {
      char str_addr[INET6_ADDRSTRLEN];
      struct sockaddr_in6 *sa = (struct sockaddr_in6 *) sock_add;
      inet_ntop(AF_INET6, &((sa)->sin6_addr), str_addr, INET6_ADDRSTRLEN);
      printf("%s:%d\n", str_addr, ntohs(sa->sin6_port));
      break;
    }
    default:
      fprintf(stderr, "Address Family not implemented\n");
  }
}
