/*
 * debug.h
 *
 *  Created on: 23.09.2016
 *      Author: drakon
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include "chprintf.h"

#define DEBUG TRUE

#define OUTPUT_DBG  (BaseSequentialStream *)&SD1
#define OUTPUT_ERR  (BaseSequentialStream *)&SD1
#define OUTPUT_STD  (BaseSequentialStream *)&SD1

#if DEBUG == FALSE

#define print_std(fmt, ...)
#define print_dbg(fmt, ...)
#define print_err(fmt, ...)

#else

    #define print_dbg(fmt, ...)  chprintf(OUTPUT_DBG, fmt, ##__VA_ARGS__)
    #define print_err(fmt, ...)  chprintf(OUTPUT_ERR, fmt, ##__VA_ARGS__)
    #define print_std(fmt, ...)  chprintf(OUTPUT_STD, fmt, ##__VA_ARGS__)

#endif



#endif /* DEBUG_H_ */
