#pragma once

// Horizon's newlib has no AF_UNIX. Godot only needs the type to compile net_socket_unix.h.
#include <sys/socket.h>

#ifndef AF_UNIX
#define AF_UNIX 1
#endif

struct sockaddr_un {
	sa_family_t sun_family;
	char sun_path[108];
};
