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
#include <syslog.h>
#include <sys/stat.h>
#include <json/json.h>


#define BUFFLEN 524288
#define MAX_DESCRIPTORS 8192

static char temporary_buffer[BUFFLEN];

int daemonize(void)
{
	int maxfd, fd;

	/* daemonize itself */
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

	/* prevents itself from become process leader */
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

	json_object *jobj = json_object_new_object();
	*buf = '\0';

	do {
		const char *type = auparse_get_type_name(au);
		json_object *json_type = json_object_new_string(type);

		json_object_object_add(jobj, "type", json_type);

		do {
			json_object *json_field_value = json_object_new_string(auparse_get_field_str(au));
			json_object_object_add(jobj, auparse_get_field_name(au), json_field_value);
		} while (auparse_next_field(au) > 0);

		strcat(buf, json_object_to_json_string(jobj));
		strcat(buf, ",\n\n");

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

	openlog("audit_log_parser", LOG_PID, LOG_DAEMON);

	syslog(LOG_INFO, "Auditd to JSON daemon has been started");

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		syslog(LOG_ERR, "Couldn't create a socket");
		exit(1);
	}

	memset(&server, 0, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(8888);

	if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		syslog(LOG_ERR, "Couldn't bind to the socket");
		exit(1);
	}

	for (;;) {
		listen(server_fd, 1);


		client_fd = accept(server_fd, (struct sockaddr *)&client, (socklen_t *)&sock_len);
		if (client_fd > 0) {
			au = auparse_init(AUSOURCE_LOGS, NULL);
			if (au == NULL) {
				syslog(LOG_ERR, "You should run that program with the root privelegies\n");
				exit(1);
			}

			err = auparse_first_record(au);
			if (err == -1) {
				syslog(LOG_ERR, "Couldn't initialize auparse");
				exit(1);
			}

			do {
				fetch_next_event(au, temporary_buffer);
				send(client_fd, temporary_buffer, strlen(temporary_buffer), 0);
			} while (auparse_next_event(au) > 0);

			auparse_destroy(au);
		} else {
		    syslog(LOG_ERR, "accept failed");
		}

		close(client_fd);
	}

	close(server_fd);

	syslog(LOG_INFO, "Auditd to JSON daemon has been finished");

	return 0;
}
