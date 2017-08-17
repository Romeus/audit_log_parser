#include <stdio.h>
#include <stdlib.h>
#include <libaudit.h>
#include <auparse.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	int err;
        auparse_state_t *au;

	au = auparse_init(AUSOURCE_LOGS, NULL);
	if (au == NULL) {
		printf("You should run that program with the root privelegies\n");
		printf("The error is: %s\n", strerror(errno));
		exit(1);
	}

        err = auparse_first_record(au);
	if (err == -1) {
		printf("%s", "Couldn't initialize auparse");
	}

        do {
                do {
                        char buf[32];
                        const char *type = auparse_get_type_name(au);
                        if (type == NULL) {
                                snprintf(buf, sizeof(buf), "%d",
                                        auparse_get_type(au));
                                type = buf;
                        }
                        printf("Record type: %s - ", type);
                        do {
                                const char *name = auparse_get_field_name(au);
                                printf("%s,", name);
                        } while (auparse_next_field(au) > 0);
                        printf("\b \n");
                } while (auparse_next_record(au) > 0);
        } while (auparse_next_event(au) > 0);

        auparse_destroy(au);

        return 0;
}
