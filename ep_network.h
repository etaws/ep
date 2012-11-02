#ifndef _ET_NETWORK_H_
#define _ET_NETWORK_H_

// create socket fd for listen on some ports
int create_and_bind (char* ports);

// set a socket fd as a non blocking socket
int make_socket_non_blocking(int sfd);

#endif
