#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <libaudit.h>
#include <unistd.h>
#include <auparse.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFLEN 524288
#define MAX_DESCRIPTORS 8192

static char temporary_buffer[BUFFLEN];

int daemonize(void)
{
	int maxfd, fd;

	switch (fork()) {
	case -1:
		return -1;
	case 0:
		break;
	default:
		_exit(EXIT_SUCCESS);
	}

	if (setsid() == -1)
		return -1;

	switch (fork()) {
	case -1:
		return -1;
	case 0:
		break;
	default:
		_exit(EXIT_SUCCESS);
	}

	umask(0);
	chdir("/");

	maxfd = sysconf(_SC_OPEN_MAX);
	if (maxfd == -1)
		maxfd = MAX_DESCRIPTORS;

	for (fd = 0; fd < maxfd; fd++)
		close(fd);

	close(STDIN_FILENO);

	fd = open("/dev/null", O_RDWR);

	if (fd != STDIN_FILENO)
		return -1;
	if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
		return -1;
	if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
		return -1;

	return 0;
}

char *fetch_next_event(auparse_state_t *au, char *buf)
{
	const char *record;

	do {
		*buf = '\0';
		record = auparse_get_record_text(au);

		if (record == NULL)
			continue;

		strcat(buf, record);
	} while (auparse_next_record(au) > 0);

	return buf;
}

int main(int argc, char *argv[])
{
	int err;
	int server_fd, client_fd;
	int sock_len = sizeof(struct sockaddr_in);
	auparse_state_t *au;
	struct sockaddr_in server, client;

	if (daemonize() == -1) {
		perror("Couldn't create the daemon\n");
		exit(1);
	}

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Couldn't create a socket");
		exit(1);
	}

	memset(&server, 0, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(8888);

	if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Couldn't bind to the socket");
		exit(1);
	}

	for (;;) {
		listen(server_fd, 1);


		client_fd = accept(server_fd, (struct sockaddr *)&client, (socklen_t *)&sock_len);
		if (client_fd > 0) {
			au = auparse_init(AUSOURCE_LOGS, NULL);
			if (au == NULL) {
				perror("You should run that program with the root privelegies\n");
				exit(1);
			}

			err = auparse_first_record(au);
			if (err == -1) {
				perror("Couldn't initialize auparse");
				exit(1);
			}

			do {
				fetch_next_event(au, temporary_buffer);
				send(client_fd, temporary_buffer, strlen(temporary_buffer), 0);
			} while (auparse_next_event(au) > 0);

			auparse_destroy(au);
		} else {
		    perror("accept failed");
		}

		close(client_fd);
	}

	close(server_fd);

	return 0;
}
