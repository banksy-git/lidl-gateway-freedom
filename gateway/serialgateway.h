/*
    Serial port gateway for Silvercrest (Lidl) Smart Home Gateway
  =================================================================
  Author: Paul Banks [https://paulbanks.org/]

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#ifndef SPG_SERIALGATEWAY_H
#define SPG_SERIALGATEWAY_H

#ifdef ENABLE_DEBUGLOG
#define LOG_DEBUG(format, ...) fprintf(stderr, format "\n", __VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif

#define LOG_ERROR(format, ...) fprintf(stderr, format "\n", __VA_ARGS__)

#endif // End header guard