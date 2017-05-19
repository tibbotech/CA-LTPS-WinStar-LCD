#ifndef COMMON_H
#define COMMON_H


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdarg.h>
#include <getopt.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>


enum listen_on {
    UNIX,
    TCP
};

typedef struct run_options {
    int goDaemon;
    int doChroot;
    char *chrootDir;
    char *pidfile;
    char *mapFile;
    char *unixSock;
    char *ip;
    int port;
    int uid;
    int gid;
    listen_on lint;
    int sock;
} run_options_t;


#endif // COMMON_H
