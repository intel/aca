/****
    Copyright (C) 2019-2020 Intel Corporation.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.






****/

#ifndef _SEPAGENT_LOG_H_
#define _SEPAGENT_LOG_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

extern FILE *fptr;
extern DRV_BOOL verbose;

#define SEPAGENT_PRINT_DEBUG(fmt,args...) {                           \
    if (verbose) {                                                    \
        fprintf (stdout, SEPAGENT_MSG_PREFIX " [DEBUG] " fmt,##args); \
        fflush(stdout);                                               \
    }                                                                 \
    if (fptr != NULL)  {                                             \
        fprintf (fptr, SEPAGENT_MSG_PREFIX" [DEBUG] %s [%s/%s:%d] "   \
            fmt, __TIME__, __FILENAME__, __func__, __LINE__, ##args); \
        fflush(fptr);                                                 \
    }                                                                 \
}

#define SEPAGENT_PRINT(fmt,args...) {                                 \
    fprintf (stdout, SEPAGENT_MSG_PREFIX "  " fmt,##args);            \
    fflush(stdout);                                                   \
    if (fptr != NULL)  {                                              \
        fprintf (fptr, SEPAGENT_MSG_PREFIX " %s [%s/%s:%d] "          \
            fmt, __TIME__, __FILENAME__, __func__, __LINE__, ##args); \
        fflush(fptr);                                                 \
    }                                                                 \
}

#define SEPAGENT_PRINT_WARNING(fmt,args...) {                         \
    if (verbose) {                                                    \
        fprintf (stdout, SEPAGENT_MSG_PREFIX " [WARN] " fmt,##args);  \
        fflush(stdout);                                               \
    }                                                                 \
    if (fptr != NULL)  {                                              \
        fprintf (fptr, SEPAGENT_MSG_PREFIX" [WARN] %s [%s/%s:%d] "    \
            fmt, __TIME__, __FILENAME__, __func__, __LINE__, ##args); \
        fflush(fptr);                                                 \
    }                                                                 \
}

#define SEPAGENT_PRINT_ERROR(fmt,args...) {                           \
    fprintf (stderr, SEPAGENT_MSG_PREFIX" [ERROR] " fmt, ##args);     \
    fflush(stdout);                                                   \
    if (fptr != NULL)  {                                              \
        fprintf (fptr, SEPAGENT_MSG_PREFIX" [ERROR] %s [%s/%s:%d] "   \
            fmt, __TIME__, __FILENAME__, __func__, __LINE__, ##args); \
        fflush(fptr);                                                 \
    }                                                                 \
}

#if defined(__cplusplus)
}
#endif

#endif
