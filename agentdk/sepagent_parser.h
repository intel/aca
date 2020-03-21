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

#ifndef _SEPAGENT_PARSER_H_
#define _SEPAGENT_PARSER_H_
/*
 *  File: sepagent_parser.h
 */

/* ------------------------------------------------------------------------- */
/*!
 * @fn          void sepagent_Print_help (void)
 *
 * @brief       Print the usage messsage
 *
 * @param       NONE
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *              Prints out detailed usage message.
 */
extern void
SEPAGENT_Print_Help(
    void
);


/* ------------------------------------------------------------------------- */
/*!
 * @fn          int sepagent_Parser
 *
 * @brief       Parse the command line arguments
 *
 * @param       IN  num_args            - Number of arguments
 * @param       IN  my_argv[]           - the args list
 * @param       OUT *data_transfer_mode - data transfer mode
 *
 * @return      NONE
 *
 * <I>Special Notes:</I> None
 */
// Partial implemetation
extern int
SEPAGENT_Parser(
    int    num_args,
    char  *options_arr[],
    int   *data_transfer_mode
);

#endif // _SEPAGENT_PARSER_H_

