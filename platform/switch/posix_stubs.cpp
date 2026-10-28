// Horizon has no processes, pipes or signals. newlib declares these but doesn't link them, and
// OS_Unix / FileAccessUnixPipe reference them, so fail cleanly with ENOSYS instead of at link time.

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" {

int execvp(const char *, char *const[]) {
	errno = ENOSYS;
	return -1;
}

int mkfifo(const char *, mode_t) {
	errno = ENOSYS;
	return -1;
}

int pclose(FILE *) {
	errno = ENOSYS;
	return -1;
}

int pipe(int[2]) {
	errno = ENOSYS;
	return -1;
}

FILE *popen(const char *, const char *) {
	errno = ENOSYS;
	return nullptr;
}

pid_t setsid(void) {
	errno = ENOSYS;
	return -1;
}

int sigaction(int, const struct sigaction *, struct sigaction *) {
	errno = ENOSYS;
	return -1;
}

pid_t waitpid(pid_t, int *, int) {
	errno = ENOSYS;
	return -1;
}

} // extern "C"
