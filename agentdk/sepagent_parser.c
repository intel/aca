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

#include "lwpmudrv_defines.h"
#include "lwpmudrv_version.h"
#include "lwpmudrv_types.h"
#include "rise_errors.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_ioctl.h"
#include "lwpmudrv_struct.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "sepagent_parser.h"
#include "log.h"

DRV_BOOL verbose = FALSE;
extern int sepagent_Print_Version();

// Macros to parse command line args
#define IS_OPTION(token,str1)               (strcmp(token, str1) == 0)
#define IS_EITHER_OPTION(token,str1,str2)   (strcmp(token, str1) == 0 || \
                                             strcmp(token, str2) == 0)
#define CHECK_END_OF_OPTION_AND_EXIT(a,b,c)       \
    if ((a) == (b)) { fprintf(stderr, (c)); return VT_SEP_OPTIONS_ERROR; }

#define NUM_FIELDS_RUN_INFO      1
static U32 is_dup_run_info[NUM_FIELDS_RUN_INFO];
/*******************************************
/ is_dup_run_info: what each index represents
/ [0] transfer mode
*******************************************/

/* ------------------------------------------------------------------------- */
/*!
 * @fn          void sep_parser_Str_Lower (char *token)
 *
 * @brief       convert all characters in string to lower case
 *
 * @param       token: string to process
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 *
 * ------------------------------------------------------------------------- */
static void
sep_parser_Str_Lower (
    char  *token
)
{
    U32 i = 0;
    while (token[i] != '\0') {
        token[i] = (char)tolower(token[i]);
        i++;
    }
    return;
}

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
void SEPAGENT_Print_Help(
    void
)
{
    fprintf(stdout, "Usage:\n");
    fprintf(stdout, "\tsepagent [Options]\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "\t-start \t\t\t Start the collection\n");
    fprintf(stdout, "\t [-tm \t Specify type of transfer [IMMEDIATE_TRANSFER/DELAYED_TRANSFER]}\n");
    fprintf(stdout, "\t-version \t\t Display sepagent version info\n");
    fprintf(stdout, "\t-v \t Verbose mode \n");
}
/* ------------------------------------------------------------------------- */
/*!
 * @fn          U32 sep_parser_tarnsfer_mode (INOUT U32         *i,
 *                                            IN    const U32    num_args,
 *                                            IN    STCHAR      *options_arr[],
 *                                            OUT   U32         *data_transfer_mode
 *                                            )
 * @brief       helper function used by parser to parse transfer mode
 *
 * @param       IN i: index into options_arr
 * @param       IN num_args: size of options_arr
 * @param       IN options_arr: character array filled out with options
 * @param       OUT data_transfer_mode: transfer mode info is filled into this variable
 *
 * @return      VT_SUCCESS on success, otherwise on failure
 *
 * <I>Special Notes:</I>
 *
 * ------------------------------------------------------------------------- */
static int
sep_parser_transfer_mode (
    int    *i,
    int    num_args,
    char  *options_arr[],
    int  *data_transfer_mode
)
{
    char   *token;
    (*i)++;
    CHECK_END_OF_OPTION_AND_EXIT(*i, num_args, "Error: Invalid transfer mode value!\n");
    token = options_arr[*i];
    if (token[0] == '-') {
        fprintf (stderr, "Error: invalid transfer mode value!\n");
        return VT_SEP_OPTIONS_ERROR;
    }

    if (is_dup_run_info[0] == 1) {
        fprintf(stderr, "\nWarning: duplicate values for transfer mode are given!");
    }
    else {
        if (strcmp(token, "DELAYED_TRANSFER") == 0) {
            *data_transfer_mode = DELAYED_TRANSFER;
        }
        else if (strcmp(token, "IMMEDIATE_TRANSFER") == 0) {
            *data_transfer_mode = IMMEDIATE_TRANSFER;
        }
        else {
            fprintf (stderr, "Error: invalid transfer mode value!\n");
            return VT_SEP_OPTIONS_ERROR;
        }
        is_dup_run_info[0]  = 1;
    }
    return VT_SUCCESS;
}


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
int SEPAGENT_Parser(
    int   num_args,
    char  *options_arr[],
    int   *data_transfer_mode
)
{
    int     status = VT_SUCCESS;
    U32     i      = 0;
    U32     print_version = FALSE;
    char   *token;

    if (num_args > 1) {
        token  = options_arr[1];
        sep_parser_Str_Lower(token);
        if (IS_OPTION(token, "-start")) {
            for (i = 2; i < num_args; i++) {
                token  = options_arr[i];
                sep_parser_Str_Lower(token);
                if (IS_EITHER_OPTION(token, "-tm", "-transfer-mode")) {
                    status = sep_parser_transfer_mode(&i, num_args, options_arr, data_transfer_mode);
                }
                else if (IS_OPTION(token, "-v")) {
                    verbose = TRUE;
                }
                else {
                    fprintf(stdout, "Error: Incorrect option provided!\n");
                    status =  VT_SEP_OPTIONS_ERROR;
                }
                if (status != VT_SUCCESS) {
                    return VT_SEP_OPTIONS_ERROR;
                }
            }
        }
        else if (IS_OPTION(token, "-version")) {
            status = sepagent_Print_Version();
            if (status == VT_SUCCESS) {
                exit(0);
            }
        }
        else if (IS_OPTION(token, "-help")){
            status =  VT_SEP_OPTIONS_ERROR;
        }
        else {
            fprintf(stdout, "Error: Incorrect option provided!\n");
            status =  VT_SEP_OPTIONS_ERROR;
        }
        if (status != VT_SUCCESS) {
            return VT_SEP_OPTIONS_ERROR;
        }
    }
    else {
        return VT_SEP_OPTIONS_ERROR;
    }
    return VT_SUCCESS;
}

