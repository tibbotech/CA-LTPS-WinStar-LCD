#include "common.h"
#include "logging.h"
#include "configfile.h"
#include "utils.h"
#include <pwd.h>
#include <grp.h>
#include <netdb.h>
#include <sys/socket.h>


void
ConfigFile::cleanup()
{
    if(NULL != _fp)
        fclose(_fp);
    ::free(_filename);

    _fp = NULL;
    _filename = NULL;
}


void
ConfigFile::setName(const char *fn)
{
    ::free(_filename);
    _filename = ::strdup(fn);
}


bool
ConfigFile::setError(Error errcode)
{
    if(NULL != _fp)
        fclose(_fp);
    _fp = NULL;
    _err = errcode;
    return _err == SUCCESS;
}


bool
ConfigFile::load(run_options_t *opts)
{
    _options = opts;

    if(NULL == _filename)
        return setError(NOT_INTITALIZED);

    _fp = fopen(_filename, "r");
    if(NULL == _fp) {
        ERR("\"%s\" file not found. Using defaults.", _filename);
        return setError(FILE_NOT_FOUND);
    }

    doLoad();

    return setError(SUCCESS);
}


bool
ConfigFile::doLoad()
{
    char temp[512];
    char *str, *p;
    int line;

    for(line=1; !feof(_fp); ++line) {
        memset(temp, 0, sizeof(temp));

        fgets(temp, sizeof(temp)-1, _fp);

        /* Wipe out comments, if found */
        p = strchr(temp, '#');
        if(NULL != p)
            *p = '\0';

        /* Cut trailing and leading spaces */
        str = trim(temp);

        /* Entire string is a comment? */
        if(*str == '\0')
            continue;

        /* Find separator character between keyword and argument
         */
        p = strchr(str, ' ');
        if(NULL == p) {
            p = strchr(str, '\t');
            if(NULL == p) {
                WARN("%s(%d): Invalid line format", _filename, line);
                continue;
            }
        }

        *p++ = '\0';
        while((*p==' ') || (*p=='\t'))
            ++p;

        /* Here 'str' points to the keyword and 'p' points to the
         * first non-space character of keyword argument
         */
        parseArg(str, p, line, _options);
    }

    return true;
}


int
ConfigFile::whatKeyword(const char *kw)
{
    static const char *kws[] = {
        "daemonize",
        "mapfile",
        "spislot",
        "pidfile",
        "user",
        "group",
        "chroot",
        "chrootpath",
        "listenon",
        "unixsocket",
        "ip",
        "port",

        NULL};

    const char *kwptr;
    int i;

    for(kwptr=kws[0], i=0; NULL != kwptr; kwptr=kws[++i])
        if(0 == strcasecmp(kwptr, kw))
            return i;

    return -1;
}


void
ConfigFile::parse_daemonize(const char *arg, int line, run_options_t *opts)
{
    int res;

    if(!get_bool(arg, &res))
         ERR("%s(%d): Argument must be boolean, got '%s'", _filename, line, arg);
    else
        opts->goDaemon = res;
}


void
ConfigFile::parse_mapfile(const char *arg, int line, run_options_t *opts)
{
    ::free(opts->mapFile);
    opts->mapFile = strdup(arg);
    LOG("Map file: %s", arg);
}


void
ConfigFile::parse_spislot(const char *arg, int line, run_options_t *opts)
{
    if(arg[0] != 's' && arg[0] != 'S') {
        ERR("%s(%d): Invalid slot name '%s'", _filename, line, arg);
    } else {
        LOG("SPI slot: %s", arg);
    }
}


void
ConfigFile::parse_pidfile(const char *arg, int line, run_options_t *opts)
{
    ::free(opts->pidfile);
    opts->pidfile = strdup(arg);
    LOG("PID file: %s", arg);
}


void
ConfigFile::parse_user(const char *arg, int line, run_options_t *opts)
{
    char *end;
    int uid;
    struct passwd *pw;

    uid = strtol(arg, &end, 10);
    if(*end != '\0') {
        /* may be symbolic user name given? */
        pw = getpwnam(arg);
        if(NULL == pw) {
            ERR("%s(%d): user '%s' was not found", _filename, line, arg);
        } else {
            opts->uid = pw->pw_uid;
            opts->gid = pw->pw_gid;
        }
    } else {
        opts->uid = uid;
    }
}


void
ConfigFile::parse_group(const char *arg, int line, run_options_t *opts)
{
    char *end;
    struct group *grp;
    int gid;

    gid = strtol(arg, &end, 10);
    if(*end != '\0') {
        /* may be symbolic group name given? */
        grp = getgrnam(arg);
        if(NULL == grp) {
            ERR("%s(%d): group '%s' was not found", _filename, line, arg);
        } else {
            opts->gid = grp->gr_gid;
        }
    } else {
        opts->gid = gid;
    }
}


void
ConfigFile::parse_chroot(const char *arg, int line, run_options_t *opts)
{
    int res;

    if(!get_bool(arg, &res))
         ERR("%s(%d): Argument must be boolean, got '%s'", _filename, line, arg);
    else
        opts->doChroot = res;
}


void
ConfigFile::parse_chrootpath(const char *arg, int line, run_options_t *opts)
{
    ::free(opts->chrootDir);
    opts->chrootDir = strdup(arg);
    LOG("Chroot dir: %s", arg);
}


void
ConfigFile::parse_listenon(const char *arg, int line, run_options_t *opts)
{
    if(0 == strcasecmp(arg, "unix"))
        opts->lint = UNIX;
    else if(0 == strcasecmp(arg, "tcp"))
        opts->lint = TCP;
    else
        ERR("%s(%d): Argument of ListenOn must be either 'unix' or 'tcp'", _filename, line);
}


void
ConfigFile::parse_unixsocket(const char *arg, int line, run_options_t *opts)
{
    ::free(opts->unixSock);
    opts->unixSock = strdup(arg);
    LOG("Unix socket: %s", arg);
}


void
ConfigFile::parse_ip(const char *arg, int line, run_options_t *opts)
{
    struct hostent *h;

    h = gethostbyname(arg);
    if(NULL != h) {
        ::free(opts->ip);
        opts->ip = strdup(arg);
    } else {
        ERR("%s(%d): host name '%s' not resolved", _filename, line, arg);
    }
}


void
ConfigFile::parse_port(const char *arg, int line, run_options_t *opts)
{
    char *end;
    int port;

    port = strtol(arg, &end, 10);
    if(*end == '\0') {
        if(port < 0 || port > 65535)
            ERR("%s(%d): port number %s is out of range", _filename, line, arg);
        opts->port = port;
    } else {
        ERR("%s(%d): Invalid port number: %s", _filename, line, arg);
    }
}


void
ConfigFile::parseArg(const char *kw, const char *arg, int line, run_options_t *opts)
{
    static struct conf_keyword _keywords[] = {
        { "daemonize",  &ConfigFile::parse_daemonize },
        { "mapfile",    &ConfigFile::parse_mapfile },
        { "spislot",    &ConfigFile::parse_spislot },
        { "pidfile",    &ConfigFile::parse_pidfile },
        { "user",       &ConfigFile::parse_user },
        { "group",      &ConfigFile::parse_group },
        { "chroot",     &ConfigFile::parse_chroot },
        { "chrootpath", &ConfigFile::parse_chrootpath },
        { "listenon",   &ConfigFile::parse_listenon },
        { "unixsocket", &ConfigFile::parse_unixsocket },
        { "ip",         &ConfigFile::parse_ip },
        { "port",       &ConfigFile::parse_port }
    };

    int i;

    for(i=0; i<sizeof(_keywords)/sizeof(_keywords[0]); ++i) {
        if(0 == strcasecmp(kw, _keywords[i].kw)) {
            if(NULL != _keywords[i].func) {
                (this->*_keywords[i].func)(arg, line, opts);
            } else {
                DBG("%s(%d): keyword '%s' currently not impemented", _filename, line, kw);
            }
            return;
        }
    }

    ERR("%s(%d): Unknown keyword: '%s'", _filename, line, kw);
}
