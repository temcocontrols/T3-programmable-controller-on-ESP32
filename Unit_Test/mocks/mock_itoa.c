#include <stdio.h>
#include "mock_itoa.h"

#ifndef _WIN32
char *itoa(int value, char *str, int base)
{
    if (!str)
        return NULL;

    switch (base) {
    case 10:
        sprintf(str, "%d", value);
        break;

    case 16:
        sprintf(str, "%x", value);
        break;

    case 8:
        sprintf(str, "%o", value);
        break;

    default:
        str[0] = '\0';
        break;
    }

    return str;
}
#endif
