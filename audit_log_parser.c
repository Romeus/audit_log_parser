#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libaudit.h>
#include <unistd.h>
#include <auparse.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFLEN 524288

static char temporary_buffer[BUFFLEN];

char* fetch_next_event(auparse_state_t* au, char* buf) {
	do {
		*buf = '\0';
		const char* record = auparse_get_record_text(au);
		if(record == NULL)
			continue;

		strcat(buf, record);
	} while (auparse_next_record(au) > 0);

	return buf;
}

int main(int argc, char *argv[])
{
	int err;
	int server_fd, client_fd;
	int sock_len;
        auparse_state_t *au;
	struct sockaddr_in server, client;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Couldn't create a socket");
		exit(1);
	}

	memset(&server, 0, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(8888);

	if(bind(server_fd,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("Couldn't bind to the socket");
		exit(1);
	}

	for(;;) {
		listen(server_fd, 1);

		sock_len = sizeof(struct sockaddr_in);

		client_fd = accept(server_fd, (struct sockaddr *)&client, (socklen_t*)&sock_len);
		if (client_fd > 0)
		{
			au = auparse_init(AUSOURCE_LOGS, NULL);
			if (au == NULL) {
				perror("You should run that program with the root privelegies\n");
				exit(1);
			}

			err = auparse_first_record(au);
			if (err == -1) {
				perror("Couldn't initialize auparse");
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
