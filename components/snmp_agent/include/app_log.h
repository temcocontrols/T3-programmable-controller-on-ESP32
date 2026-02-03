#ifndef APPLOG_H
#define APPLOG_H

void app_log(const char *tag, const char *format, ...);
void check_heap_fragmentation(void);

#endif //APPLOG