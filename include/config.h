#ifndef CONFIG_H
#define CONFIG_H


#define APPNAME         "lcdsrv"
#define MAXFD           1024
#define UMASK           0
#define WORKDIR         "/"
#define CONFDIR         "/etc"
#define PID_FILE        "/var/run/" APPNAME ".pid"
#define UNIX_SOCK       "/var/run/" APPNAME
#define MAP_FILE        "area.map"
#define CONFIG_FILE     APPNAME ".conf"
#define COUNTOF(x)      (sizeof(x)/sizeof(x[0]))
#define DEFAULT_PORT    6116
#define MAX_CLIENTS     7
#define MAX_BUF_SIZE    256


#endif // CONFIG_H
