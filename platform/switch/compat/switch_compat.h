#pragma once

// Force-included into every C++ unit (see detect.py): the few BSD-socket names Godot's unix
// drivers expect that libnx / newlib leave out. aarch64 Horizon is little-endian.
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h> // htons / ntohs live here in libnx.
#include <sys/resource.h>

// newlib's <sys/resource.h> has the header but not the stack limit API; Godot only reads it for
// the memory-info dictionary, so report "unknown" (0).
#ifndef RLIMIT_STACK
#define RLIMIT_STACK 3
struct rlimit {
	unsigned long long rlim_cur;
	unsigned long long rlim_max;
};
static inline int getrlimit(int, struct rlimit *r_limit) {
	r_limit->rlim_cur = 0;
	r_limit->rlim_max = 0;
	return 0;
}
#endif

static inline unsigned int if_nametoindex(const char *) {
	return 0;
}

struct ipv6_mreq {
	struct in6_addr ipv6mr_multiaddr;
	unsigned int ipv6mr_interface;
};
