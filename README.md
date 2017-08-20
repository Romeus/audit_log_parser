Implement a simple Audit D log parser, using the AuditD API to fetch, process and forward AuditD messages to an HTTP/2 API interface.
The implementation should be able to fetch events from AuditD, parse the event into a JSON object, and pass the log to a network host for processing.
The interface should be non-blocking to the system, and forward any log information to the collection interface in a very performant manner.
Any included libraries should reference reasoning behind the specific selection, and assumptions for the API side should be noted.

Mandatory:

- The implementation should be built in C, and leverage compatibility of early AuditD API features
- The program should restart if there is a failure

Optional:

- The program should be hidden from the system process list
- The program should protect itself from being stopped by a standard user.

Testing:

To test, we should be able to use a simple netcat session, listening on port 8888

===================================================================
Prerequisites

I tested that project on centos 7.0.
$ uname -r
3.10.0-514.6.2.el7.x86_64


Before you compile it you need to install the following libraries:

$ sudo yum install json-c jsonc-devel json-c-doc audit

===================================================================
Installation

Clone that project to a folder, go there and type:
$ make

The build result is a binary executable: 
audit_log_parser

===================================================================
Run

$ sudo ./audit_log_parser

It returns immediately because it is daemonized themself.


===================================================================
Test
$ netcat localhost 8888

When the ouptut is complete you could break the session using <CTRL-C>
And you could try this again if you want.


===================================================================
Terminate

$ ps aux | grep audit_log_parser

Then obtain its PID and run:

$ sudo kill -SIGTERM <PID>
