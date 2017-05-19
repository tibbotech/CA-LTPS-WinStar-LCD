#include <stdio.h>
#include <stdarg.h>
#include "config.h"
#include "logging.h"


void
log(log_level lvl, const char *msg, ...)
{
    va_list ap;
    char tmp[1024];
    FILE *out;
    const char *cat;

    va_start(ap, msg);
    vsnprintf(tmp, sizeof(tmp), msg, ap);
    va_end(ap);

    switch(lvl) {
        case LVL_DBG:
            out = stdout;
            cat = "DEBUG:";
            break;

        case LVL_INFO:
            out = stdout;
            cat = "INFO:";
            break;

        case LVL_WARN:
            out = stderr;
            cat = "WARN:";
            break;

        case LVL_ERR:
            out = stderr;
            cat = "ERROR:";
            break;

        default:
            out = NULL;
            cat = "";
            break;
    }


    if(NULL != out)
        fprintf(out, "%s: %-6s %s\n", APPNAME, cat, tmp);
}
