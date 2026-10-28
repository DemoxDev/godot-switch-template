#pragma once

// Interface enumeration is not available through libnx's socket layer; getifaddrs() fails and
// Godot's IP::get_local_interfaces() simply returns an empty list.
#include <sys/socket.h>

#include <cerrno>

struct ifaddrs {
	struct ifaddrs *ifa_next;
	char *ifa_name;
	unsigned int ifa_flags;
	struct sockaddr *ifa_addr;
	struct sockaddr *ifa_netmask;
	struct sockaddr *ifa_dstaddr;
	void *ifa_data;
};

static inline int getifaddrs(struct ifaddrs **) {
	errno = ENOSYS;
	return -1;
}
static inline void freeifaddrs(struct ifaddrs *) {}
