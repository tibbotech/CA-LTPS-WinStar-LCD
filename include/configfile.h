#ifndef CONFIGFILE_H
#define CONFIGFILE_H


#include <stdio.h>


class ConfigFile {
private:
    typedef void (ConfigFile::*parsefunc_f)(const char *, int, run_options_t *);

    struct conf_keyword {
        const char *kw;
        parsefunc_f func;
    };
public:
    enum Error {
        SUCCESS = 0,
        FAILED, /* Generic failure */
        NOT_INTITALIZED,
        FILE_NOT_FOUND,
        SYNTAX_ERROR,
        NUM_ERRORS
    };

    ConfigFile(): _err(SUCCESS), _filename(NULL), _fp(NULL), _options(NULL) {}
    ~ConfigFile() { cleanup(); }
    bool load(run_options_t *);
    Error error() const { return _err; }
    const char *name() const { return _filename; }
    void setName(const char *);
    bool setError(Error);
    void cleanup();
protected:
    bool doLoad();
    void parseArg(const char *, const char *, int, run_options_t *);
    int whatKeyword(const char *);
private:
    /* option parsers */
    void parse_daemonize(const char *, int, run_options_t *);
    void parse_mapfile(const char *, int, run_options_t *);
    void parse_spislot(const char *, int, run_options_t *);
    void parse_pidfile(const char *, int, run_options_t *);
    void parse_user(const char *, int, run_options_t *);
    void parse_group(const char *, int, run_options_t *);
    void parse_chroot(const char *, int, run_options_t *);
    void parse_chrootpath(const char *, int, run_options_t *);
    void parse_listenon(const char *, int, run_options_t *);
    void parse_unixsocket(const char *, int, run_options_t *);
    void parse_ip(const char *, int, run_options_t *);
    void parse_port(const char *, int, run_options_t *);
private:
    Error _err;
    char *_filename;
    FILE *_fp;
    run_options_t *_options;
};


#endif // CONFIGFILE_H
