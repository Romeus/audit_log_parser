CFLAGS ?= -lauparse -ljson-c -lcurl

all: audit_log_parser

audit_log_parser: audit_log_parser.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean: FRC
	rm -f audit_log_parser audit_log_parser.o

# This is an explicit suffix rule. It may be omitted on systems
# that handle simple rules like this automatically.
.c.o:
	$(CC) $(CFLAGS) -c $<

FRC:
.SUFFIXES: .c
