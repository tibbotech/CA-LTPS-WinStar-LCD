#include "common.h"


char *
trim(char *buf)
{
    char *end;

    while('\0' != *buf && isspace(*buf))
        ++buf;

    end = strchr(buf, '\0');
    if(NULL != end) {
        while(end > buf) {
            if(!isspace(*(end-1)))
                break;
            --end;
        }
        *end = '\0';
    }

    return buf;
}


bool
get_bool(const char *arg, int *res)
{
    if(0 == strcasecmp(arg, "yes") || 0 == strcasecmp(arg, "true") || 0 == strcasecmp(arg, "on")) {
        *res = true;
        return true;
    }

    if(0 == strcasecmp(arg, "no") || 0 == strcasecmp(arg, "false") || 0 == strcasecmp(arg, "off")) {
        *res = false;
        return true;
    }

    return false;
}

