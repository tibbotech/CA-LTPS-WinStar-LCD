#include "config.h"
#include "common.h"
#include "logging.h"
#include "configfile.h"
#include "winstar_lcd.h"


extern char *trim(char *);


struct client_t {
    int fd;
    struct in_addr ip;
    char buf[MAX_BUF_SIZE+1];
    uint32_t cb;
    struct client_t *next;
};


/* Global application options
 */
run_options_t opts;


static struct option _options[] = {
    { "nodaemon",   no_argument,       &opts.goDaemon, 1 }, // 0
    { "pidfile",    required_argument, NULL, 0 },           // 1
    { "socket",     required_argument, NULL, 0 },           // 2
    { "ip",         required_argument, NULL, 0 },           // 3
    { "port",       required_argument, &opts.port, DEFAULT_PORT }, // 4
    { "slot",       required_argument, NULL, 0 },           // 5
    { "help",       no_argument,       NULL, 0 },           // 6
    { "config",     required_argument, NULL, 0 },           // 7
    { NULL, 0, NULL, 0 } // end
};


WinStarLCD _lcd;


static char _optstr[] = "dp:s:i:t:o:h";
static char _helpstr[] =
"%s: LCD gateway.\r\n"
"Usage: %s <options>\r\n"
"Options:\r\n"
"   -c FILE, --config=FILE      Set configuration file location\n"
"                               Default: " CONFIG_FILE "\n"
"   -d, --nodaemon              Do not switch to daemon mode, work in foreground as ordinary application\n"
"   -p FILE, --pidfile=FILE     Set PID file name and location\r\n"
"                               Default: " PID_FILE "\n"
"   -s SOCK --socket=SOCK       Set UNIX domain socket file name, where service will listen for requests\n"
"   -i IP, --ip=IP              Set IP address of socket, where service will listen for requests.\n"
"                               (this option is mutually exclusive with --socket)\n"
"   -t PORT, --port=PORT        Set port number for IP socket\n"
"   -o SLOT, --slot=SLOT        Set slot number where SPI interface with decaWave chip is resided\n"
"                               (this option is specific for Tibbo's LTPS hardware)\n"
"   -h, --help                  Print this message and exit\n"
"\r\n"
"Mapfile is a text file where decaWave anchor IDs along with coordinates are listed, one record per line.\r\n"
"Fields in file is separated by spaces or tabs, comments is starting with # character\r\n"
"\r\n"
;


static struct client_t *
remove_client(struct client_t *head, int fd)
{
    struct client_t *cli, *prev;

    if(NULL == head)
        return NULL;

    for(cli=head, prev=NULL; cli != NULL; prev=cli, cli=cli->next)
        if(cli->fd == fd)
            break;

    if(NULL == cli)
        return head; // not found

    if(NULL == prev) { // head
        cli = head->next;
        free(head);
        return cli;
    }

    // found at middle or end of the list
    prev->next = cli->next;
    free(cli);
    return head;
}


static struct client_t *
find_client(struct client_t *list, int fd)
{
    for(; list != NULL; list=list->next)
        if(list->fd == fd)
            return list;

    return NULL;
}


static void
fill_fds(struct client_t *list, pollfd *fds)
{
    int i;

    // fds[0] is always reserved for listening socket
    //
    for(i=1; list != NULL; ++i, list=list->next) {
        fds[i].fd = list->fd;
        fds[i].events = POLLIN | POLLOUT | POLLHUP;
        fds[i].revents = 0;
    }
}


static void
redirectFileDescriptors()
{
    struct rlimit rlim;
    int fd, maxfd;

    getrlimit(RLIMIT_NOFILE, &rlim);
    maxfd = rlim.rlim_max;
    if(maxfd == RLIM_INFINITY)
        maxfd = MAXFD;

    // Close all TTYs
    for(fd=0; fd<maxfd; ++fd) {
        if(NULL != ttyname(fd))
            close(fd);
    }

    fd = open("/dev/null", O_RDWR);

    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDERR_FILENO);
}


static void
writePid(struct run_options *opts, pid_t pid)
{
    FILE *fp;

    fp = fopen(opts->pidfile, "w");
    if(NULL != fp) {
        fprintf(fp, "%d", pid);
        fclose(fp);
    }
}


static void
initOptions(struct run_options *opts)
{
    opts->goDaemon = true;
    opts->pidfile = strdup(PID_FILE);
    opts->ip = strdup("localhost");
    opts->port = DEFAULT_PORT;
    opts->mapFile = strdup(MAP_FILE);
    opts->unixSock = strdup(UNIX_SOCK);
}


static void
cleanupOptions(struct run_options *opts)
{
    free(opts->pidfile);
    free(opts->ip);
    free(opts->mapFile);
    free(opts->unixSock);
}


/* This is our variant of daemon() syscall. Unlike stdlib's daemon() function
 * it implements so-called "double fork()" techinque to avoid becoming
 * a session leader.
 */
static int
daemonize(bool noClose)
{
    int pid;

    // Fork once to go into the background.
    syslog(LOG_DEBUG, "Forking first child");
    pid = fork();
    if(0 != pid) {
        // Parent. Exit using os._exit(), which doesn't fire any atexit
        // functions.
        cleanupOptions(&opts);
        _exit(EXIT_SUCCESS);
    }

    // First child. Create a new session. setsid() creates the session
    // and makes this (child) process the process group leader. The process
    // is guaranteed not to have a control terminal.
    syslog(LOG_DEBUG, "Creating new session");
    pid = setsid();

    // Fork a second child to ensure that the daemon never reacquires
    // a control terminal.
    syslog(LOG_DEBUG, "Forking second child");
    pid = fork();
    if(0 != pid) {
        // Original child. Exit.
        cleanupOptions(&opts);
        _exit(EXIT_SUCCESS);
    }

    writePid(&opts, pid);

    // This is the second child. Set the umask.
    syslog(LOG_DEBUG, "Setting umask");
    umask(UMASK);

    // Go to a neutral corner (i.e., the primary file system, so
    // the daemon doesn't prevent some other file system from being
    // unmounted).
    syslog(LOG_DEBUG, "Changing working directory to \"%s\"", WORKDIR);
    chdir(WORKDIR);

    // Unless noClose was specified, close all file descriptors.
    if(!noClose) {
        syslog(LOG_DEBUG, "Redirecting file descriptors");
        redirectFileDescriptors();
    }

    syslog(LOG_INFO, "Switched to daemon mode");
    return 0;
}


static void
parseCommandLine(struct run_options *opts, int argc, char *argv[])
{
    int n, index;

    for(;;) {
        index = -1;
        if((n = getopt_long(argc, argv, _optstr, _options, &index)) < 0)
            break;

        if(index < 0) {
            switch(n) {
                case 'p':
                    index = 1;
                    break;
                case 's':
                    index = 2;
                    break;
                case 'h':
                    index = 6;
                    break;
            }
        }

        switch(index) {
            case 0: // --nodaemon
                opts->goDaemon = 0;
                break;

            case 1: // --PID_FILE
                printf("Arg: %s\n", optarg);
                break;

            case 6: // --help
                fprintf(stdout, _helpstr, argv[0], argv[0]);
                cleanupOptions(opts);
                _exit(EXIT_SUCCESS);
                break;

            default:
                break;
        }
    }
}


static int
initSocket(struct run_options *opts)
{
    struct sockaddr_in addr;
    struct in_addr ina;

#if 0
    struct hostent *hn;
    struct in_addr *resolved;

    hn = gethostbyname(opts->ip);
    if(NULL == hn) {
        ERR("Cannot resolve host name '%s'", opts->ip);
        return -1;
    }
    resolved = (struct in_addr *)hn->h_addr_list[hn->h_length-1];
#endif

    inet_aton(opts->ip, &ina);

    opts->sock = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == opts->sock)
        return -1;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(opts->port);
#if 0
    addr.sin_addr.s_addr = resolved->s_addr;
#else
    addr.sin_addr.s_addr = ina.s_addr;
#endif

    if(-1 == bind(opts->sock, (struct sockaddr *)&addr, sizeof(addr))) {
        ERR("Cannot bind socket to %s:%u. %s", inet_ntoa(ina), opts->port, strerror(errno));
        return -1;
    }

    if(-1 == listen(opts->sock, 0)) {
        ERR("listen() failed. %s", strerror(errno));
        return -1;
    }

    /*hn->h_name*/
    LOG("Listening for connections on %s:%d", inet_ntoa(ina), opts->port);
    return 0;
}


static int
allocateResources(struct run_options *opts)
{
    if(-1 == _lcd.init(4)) {
        ERR("Failed to init LCD");
        return -1;
    }

    if(-1 == initSocket(opts))
        return -1;

    /* And, finally daemonize, if flag 'nodaemon' is not specified
     */
    if(opts->goDaemon) {
        LOG("Daemonizing");
        if(0 != daemonize(false))
            return -1;
    } else {
        LOG("Running in foreground");
    }

    return 0;
}


static void
runService(struct run_options *opts)
{
    struct client_t *clients, *newc;
    struct sockaddr_in addr;
    struct pollfd fds[MAX_CLIENTS+1];
    socklen_t slen;
    int i, clicnt, sock, rama;
    int res, nb;
    char *s;

    LOG(APPNAME " service is up and running");

    clients = NULL;
    clicnt = 0;

    fds[0].fd = opts->sock;
    fds[0].events = POLLIN | POLLOUT;
    fds[0].revents = 0;

    for(;;) {
        res = poll(fds, clicnt+1, 1000);
        if(0 == res)
            continue; // timeout

        if(res < 0) {
            ERR("poll(): %s", strerror(errno));
            break;
        }

        if(0 != (fds[0].revents & POLLIN)) {
            // New client connection accepted
            memset(&addr, 0, sizeof(addr));
            slen = sizeof(addr);

            sock = accept(fds[0].fd, (struct sockaddr *)&addr, &slen);
            if(-1 == sock) {
                ERR("Client connection failed on fd %d: %s", fds[0].fd, strerror(errno));

            } else {
                newc = (struct client_t *)malloc(sizeof(clients[0]));
                if(NULL == newc) {
                    ERR("malloc() failed.");
                    break;
                }

                newc->fd = sock;
                newc->ip = addr.sin_addr;
                newc->cb = 0;
                newc->next = clients;
                clients = newc;

                fill_fds(clients, fds);
                ++clicnt;

                LOG("Client #%d %s connected", clicnt, inet_ntoa(addr.sin_addr));
            }
        }

        for(i=1; i<clicnt+1; ++i) {
            if(0 != (fds[i].revents & POLLIN)) {
                newc = find_client(clients, fds[i].fd);
                if(NULL == newc) {
                    ERR("Unknown client in read() (internal error)");
                    return;
                }

                if((MAX_BUF_SIZE-1) == newc->cb)
                    newc->cb = 0; // Discard all received data to prevent communication hangup

                nb = read(fds[i].fd, &newc->buf[newc->cb], MAX_BUF_SIZE-newc->cb);
                if(nb > 0) {
                    newc->cb += nb;
                    newc->buf[newc->cb+1] = '\0';

                    s = strstr(newc->buf, "\r\n");
                    if(NULL != s) {
                        *s = '\0';
                        nb = newc->buf + newc->cb - s - 2;
                        LOG("Cmd: %s", newc->buf);

                        switch(newc->buf[0]) {
                            case 'A':
                            case 'a':
                                sscanf(&newc->buf[1], "%2x", &rama);
                                _lcd.setAddr(rama & 0xFF);
                                break;
                            case 'C':
                            case 'c':
                                _lcd.clear();
                                break;
                            case 'H':
                            case 'h':
                                _lcd.home();
                                break;
                            case '\\':
                                _lcd.echo(&newc->buf[1]);
                                break;
                            default:
                                _lcd.echo(newc->buf);
                                break;
                        }

                        _lcd.flush();

                        memcpy(s+2, newc->buf, nb);
                        newc->cb = nb;
                    }
                } else if(0 == nb) {
                    /* Client dropped? */
                    LOG("Client disconneced");
                    clients = remove_client(clients, fds[i].fd);
                    --clicnt;
                    fill_fds(clients, fds);
                }
            }
            if(0 != (fds[i].revents & POLLHUP)) {
                LOG("Client disconneced");
                clients = remove_client(clients, fds[i].fd);
                --clicnt;
                fill_fds(clients, fds);
            }
        }
    }
}


int
main(int argc, char *argv[])
{
    ConfigFile cfg;

    initOptions(&opts);
    parseCommandLine(&opts, argc, argv);

    cfg.setName(CONFIG_FILE);
    cfg.load(&opts);

    LOG("Starting");
    if(0 == allocateResources(&opts))
        runService(&opts);

    cleanupOptions(&opts);
    return EXIT_SUCCESS;
}
