#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>

#include "ep_network.h"

#define MAXEVENTS 64

int main (int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit (EXIT_FAILURE);
	}

	int sfd = create_and_bind(argv[1]);
	if (sfd == -1) {
		abort ();
	}

	int s;
	s = make_socket_non_blocking(sfd);
	if (s == -1) {
		abort ();
	}

	s = listen(sfd, SOMAXCONN);
	if (s == -1) {
		perror ("listen");
		abort ();
	}

	int efd = epoll_create1(0);
	if (efd == -1) {
		perror ("epoll_create");
		abort ();
	}

	struct epoll_event event;
	event.data.fd = sfd;
	event.events = EPOLLIN | EPOLLET;
	s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
	if (s == -1) {
		perror ("epoll_ctl");
		abort ();
	}

	/* Buffer where events are returned */
	struct epoll_event* events = calloc(MAXEVENTS, sizeof event);

	/* The event loop */
	while (1) {

		int n = epoll_wait(efd, events, MAXEVENTS, -1);
		int i;
		for (i = 0; i < n; i++) {
			if ((events[i].events & EPOLLERR) ||
					(events[i].events & EPOLLHUP) ||
					(!(events[i].events & EPOLLIN))) {
				/* An error has occured on this fd, or the socket is not
				   ready for reading (why were we notified then?) */
				fprintf(stderr, "epoll error\n");
				close(events[i].data.fd);
				continue;
			} else if (sfd == events[i].data.fd) {
				/* We have a notification on the listening socket, which
				   means one or more incoming connections. */
				while (1) {
					struct sockaddr in_addr;
					socklen_t in_len = sizeof in_addr;
					int infd = accept(sfd, &in_addr, &in_len);
					if (infd == -1) {
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
							/* We have processed all incoming
							   connections. */
							break;
						} else {
							perror ("accept");
							break;
						}
					}

					char hbuf[NI_MAXHOST];
					char sbuf[NI_MAXSERV];
					s = getnameinfo(&in_addr, in_len,
							hbuf, sizeof hbuf,
							sbuf, sizeof sbuf,
							NI_NUMERICHOST | NI_NUMERICSERV);
					if (s == 0) {
						printf("Accepted connection on descriptor %d "
								"(host=%s, port=%s)\n", infd, hbuf, sbuf);
					}

					/* Make the incoming socket non-blocking and add it to the
					   list of fds to monitor. */
					s = make_socket_non_blocking(infd);
					if (s == -1) {
						abort ();
					}

					event.data.fd = infd;
					event.events = EPOLLIN | EPOLLET;
					s = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
					if (s == -1) {
						perror ("epoll_ctl");
						abort ();
					}
				}
				continue;
			} else {
				/* We have data on the fd waiting to be read. Read and
				   display it. We must read whatever data is available
				   completely, as we are running in edge-triggered mode
				   and won't get a notification again for the same
				   data. */
				int done = 0;

				while (1) {
					char buf[512];
					ssize_t count = read(events[i].data.fd, buf, sizeof buf);
					if (count == -1) {
						/* If errno == EAGAIN, that means we have read all
						   data. So go back to the main loop. */
						if (errno != EAGAIN) {
							perror ("read");
							done = 1;
						}
						break;
					} else if (count == 0) {
						/* End of file. The remote has closed the
						   connection. */
						done = 1;
						break;
					}

					/* Write the buffer to standard output */
					s = write (1, buf, count);
					if (s == -1) {
						perror ("write");
						abort ();
					}
				}

				if (done) {
					printf ("Closed connection on descriptor %d\n",
							events[i].data.fd);

					/* Closing the descriptor will make epoll remove it
					   from the set of descriptors which are monitored. */
					close (events[i].data.fd);
				}
			}
		}
	}

	free(events);

	close(sfd);

	return EXIT_SUCCESS;
}
