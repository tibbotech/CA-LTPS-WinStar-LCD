#ifndef LOGGING_H
#define LOGGING_H


enum log_level
{
    LVL_DBG = 0,
    LVL_INFO,
    LVL_WARN,
    LVL_ERR
};


extern void log(log_level, const char *, ...);


#define ERR(s, ...)  log(LVL_ERR, s, ##__VA_ARGS__)
#define WARN(s, ...) log(LVL_WARN, s,  ##__VA_ARGS__)
#define LOG(s, ...)  log(LVL_INFO, s, ##__VA_ARGS__)
#define DBG(s, ...)  log(LVL_DBG, s, ##__VA_ARGS__)


#endif // LOGGING_H
