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

#ifndef __ERROR_REPORTING_UTILS_H__
#define __ERROR_REPORTING_UTILS_H__

#define DRV_ASSERT_N_RET_VAL(ret_val)                                    \
    DRV_ASSERT((ret_val) == VT_SUCCESS);                                 \
    DRV_CHECK_N_RETURN_N_FAIL(ret_val);

#define DRV_ASSERT_N_CONTINUE(ret_val)                                   \
    if ((ret_val) != VT_SUCCESS) {                                       \
        LOG_ERR1(VTSA_T("Operation failed with error code "),(ret_val)); \
    }

#define DRV_CHECK_N_RETURN_N_FAIL(ret_val)                               \
    if ((ret_val) != VT_SUCCESS) {                                       \
        LOG_ERR1(VTSA_T("Operation failed with error code "),(ret_val)); \
        return (ret_val);                                                \
    }

#define DRV_CHECK_N_RETURN_N_NULL(ret_val)                               \
    if ((ret_val) != VT_SUCCESS) {                                       \
        LOG_ERR1(VTSA_T("Operation failed with error code "),(ret_val)); \
        return NULL;                                                \
    }

#define DRV_CHECK_N_RETURN_NO_RETVAL(ret_val)                            \
    if ((ret_val) != VT_SUCCESS) {                                       \
        LOG_ERR1(VTSA_T("Operation failed with error code "),(ret_val)); \
        return;                                                          \
    }

#define DRV_CHECK_N_RETURN_N_USERDEFINED(ret_val, user_defined_val)      \
    if ((ret_val) != VT_SUCCESS) {                                       \
        LOG_ERR1(VTSA_T("Operation failed with error code "),(ret_val)); \
        return user_defined_val;                                         \
    }

#define DRV_CHECK_N_CONTINUE(ret_val)                                    \
    if ((ret_val) != VT_SUCCESS) {                                       \
        LOG_ERR1(VTSA_T("Operation failed with error code "),(ret_val)); \
        continue;                                                        \
    }

#define DRV_CHECK_N_SET_RET_N_GOTO_LABEL(ret_val, ret_var, ret_status, goto_label)   \
    if ((ret_val) != VT_SUCCESS) {                                                   \
        ret_var = ret_status;                                                        \
        LOG_ERR1(VTSA_T("Operation failed with error code "),(ret_val));             \
        goto goto_label;                                                             \
    }

#define DRV_CHECK_PTR_N_RET_VAL(ptr)                                     \
    if ((ptr) == NULL) {                                                 \
        LOG_ERR0(VTSA_T("Encountered null pointer"));                    \
        return VT_SAM_ERROR;                                             \
    }

#define DRV_CHECK_PTR_N_RET_GIVEN_VAL(ptr, ret_val)                      \
    if ((ptr) == NULL) {                                                 \
        LOG_ERR0(VTSA_T("Encountered null pointer"));                    \
        return ret_val;                                                  \
    }

#define DRV_CHECK_PTR_N_RET_NULL(ptr)                                    \
    if ((ptr) == NULL) {                                                 \
        LOG_ERR0(VTSA_T("Encountered null pointer"));                    \
        return NULL;                                                     \
    }

#define DRV_CHECK_PTR_N_RET_FALSE(ptr)                                   \
    if ((ptr) == NULL) {                                                 \
        LOG_ERR0(VTSA_T("Encountered null pointer"));                    \
        return FALSE;                                                    \
    }

#define DRV_CHECK_PTR_N_RET(ptr)                                         \
    if ((ptr) == NULL) {                                                 \
        LOG_ERR0(VTSA_T("Encountered null pointer"));                    \
        return;                                                          \
    }

#define DRV_CHECK_PTR_N_LOG_NO_RETURN(ptr)                               \
    if ((ptr) == NULL) {                                                 \
        LOG_ERR0(VTSA_T("Encountered null pointer"));                    \
    }

#define DRV_CHECK_PTR_N_CLEANUP(ptr, gotolabel, ret_val)                 \
    if ((ptr) == NULL) {                                                 \
        LOG_ERR0(VTSA_T("Encountered null pointer"));                    \
        ret_val = VT_SAM_ERROR;                                          \
        goto gotolabel;                                                  \
    }

#define DRV_CHECK_PTR_ON_NULL_CLEANUP_N_RETURN(ptr, gotolabel)         \
    if ((ptr) == NULL) {                                               \
        DRV_CHECK_PTR_N_LOG_NO_RETURN(ptr);                            \
        goto gotolabel;                                                \
    }

#define DRV_CHECK_PTR_N_RET_ASSIGNED_VAL(ptr, return_val)                                                      \
    if ( !(ptr)) {                                                                                             \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d Encountered null pointer\n",__FUNCTION__, __LINE__));  \
        return return_val;                                                                                     \
    }

#define DRV_CHECK_PTR_N_SET_RET_N_GOTO_CLEANUP(status, ret_var, ret_status, goto_label)                \
    if (status == NULL) {                                                                              \
        ret_var = ret_status;                                                                          \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d error %d\n",__FUNCTION__, __LINE__, status));  \
        goto goto_label;                                                                               \
    }

#define DRV_CHECK_N_LOG_NO_RETURN(ret_val)                               \
    if ((ret_val) != VT_SUCCESS) {                                       \
        LOG_ERR1(VTSA_T("Operation failed with error code "),(ret_val)); \
    }

#define DRV_CHECK_N_RET_NEG_ONE(ret_val)                                 \
    if ((ret_val) == -1) {                                               \
        LOG_ERR0(VTSA_T("Operation failed with error code = -1"));       \
        return VT_SAM_ERROR;                                             \
    }

#define DRV_CHECK_N_RET_NEG_ONE_N_CLEANUP(ret_val, gotolabel)            \
    if ((ret_val) == -1) {                                               \
        LOG_ERR0(VTSA_T("Operation failed with error code = -1"));       \
        goto gotolabel;                                                  \
    }

#define DRV_REQUIRES_TRUE_COND_RET_N_FAIL( cond )                        \
    if ( !(cond) ) {                                                     \
        LOG_ERR0(VTSA_T("Condition check failed"));                      \
        return VT_SAM_ERROR;                                             \
    }

#define DRV_REQUIRES_TRUE_COND_RET_ASSIGNED_VAL( cond, ret_val)         \
    if ( !(cond) ) {                                                    \
        LOG_ERR0(VTSA_T("Condition check failed"));                     \
        return ret_val;                                                 \
    }

#define DRV_CHECK_N_ERR_LOG_ERR_STRNG_N_RET( rise_err )                \
    if (rise_err != VT_SUCCESS) {                                      \
        PVOID rise_ptr = NULL;                                         \
        const VTSA_CHAR *error_str = NULL;                             \
        RISE_open(&rise_ptr);                                          \
        RISE_translate_err_code(rise_ptr, rise_err, &error_str);       \
        LogItW(LOG_LEVEL_ERROR|LOG_AREA_GENERAL, L"Operation failed with error [ %d ] = %s\n",rise_err,error_str); \
        RISE_close(rise_ptr);                                          \
        return rise_err;                                               \
    }

#define DRV_CHECK_ON_FAIL_CLEANUP_N_RETURN(ret_val, gotolabel)         \
    if ((ret_val) != VT_SUCCESS) {                                     \
        DRV_CHECK_N_LOG_NO_RETURN(ret_val);                            \
        goto gotolabel;                                                \
    }


#define DRV_CHECK_N_CLEANUP_N_RETURN_RET_NEG_ONE(ret_val, gotolabel)   \
    if ((ret_val) == -1) {                                             \
        DRV_CHECK_N_LOG_NO_RETURN(ret_val);                            \
        goto gotolabel;                                                \
    }

#define DRV_CHECK_IF_NEG_RET_N_RETURN_GIVEN_VAL(inp_val, ret_val)        \
    if ((inp_val) < 0) {                                                 \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "Operation failed with error code %d\n", ret_val)); \
        return ret_val;                                                  \
    }

#define DRV_CHECK_IF_NEG_RET_N_RETURN_NO_VAL(inp_val)                    \
    if ((inp_val) < 0) {                                                 \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "Operation failed with error code %d\n", ret_val)); \
        return;                                                          \
    }

#define FREE_N_SET_NULL(ptr)                                           \
    if (ptr != NULL) {                                                 \
        free(ptr);                                                     \
        ptr = NULL;                                                    \
    }

#define DELETE_N_SET_NULL(ptr)                                         \
        delete ptr;                                                    \
        ptr = NULL;

#define DRV_CHECK_RET_N_SET_RET_N_GOTO_CLEANUP(status, ret_var, ret_status, goto_label)                \
    if (status != VT_SUCCESS) {                                                                        \
        ret_var = ret_status;                                                                          \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d error %d\n",__FUNCTION__, __LINE__, status));  \
        goto goto_label;                                                                               \
    }

#define DRV_CHECK_IF_NEG_RET_N_SET_RET_N_GOTO_CLEANUP(status, ret_var, ret_status, goto_label)         \
    if (status < 0) {                                                                                  \
        ret_var = ret_status;                                                                          \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d error %d\n",__FUNCTION__, __LINE__, status));  \
        goto goto_label;                                                                               \
    }

/*
 * Memory management error handling macros
 */
// Check for NULL ptr and return VT_NO_MEMORY
#define SEP_CHECK_ALLOCATION_N_RET_VAL(loc)                                                                    \
    if ( !(loc)) {                                                                                             \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d Encountered null pointer\n",__FUNCTION__, __LINE__));  \
        return VT_NO_MEMORY;                                                                                   \
    }

// Check for NULL ptr and exit with -1 status
#define SEP_CHECK_ALLOCATION_N_EXIT_WITH_FAILURE(loc)                                                          \
    if ( !(loc) ) {                                                                                            \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d Encountered null pointer\n",__FUNCTION__, __LINE__));  \
        exit(-1);                                                                                              \
    }

// Check for NULL ptr and return void
#define SEP_CHECK_ALLOCATION_N_RET_NOVAL(loc)                                                                  \
    if ( !(loc) ) {                                                                                            \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d Encountered null pointer\n",__FUNCTION__, __LINE__));  \
        return;                                                                                                \
    }

// Check for NULL ptr and return False
#define SEP_CHECK_ALLOCATION_N_RET_BOOL(loc)                                                                   \
    if ( !(loc) ) {                                                                                            \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d Encountered null pointer\n",__FUNCTION__, __LINE__));  \
        return FALSE;                                                                                          \
    }

// Check for NULL ptr and return NULL
#define SEP_CHECK_ALLOCATION_N_RET_NULL(loc)                                                                   \
    if ( !(loc) ) {                                                                                            \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d Encountered null pointer\n",__FUNCTION__, __LINE__));  \
        return NULL;                                                                                           \
    }

// Check for NULL ptr and goto provided label
#define SEP_CHECK_ALLOCATION_N_GOTO_CLEANUP(loc, goto_label)                                                   \
    if ( !(loc) ) {                                                                                            \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d Encountered null pointer\n",__FUNCTION__, __LINE__));  \
        goto goto_label;                                                                                       \
    }

// Check for NULL ptr and continue the loop
#define SEP_CHECK_ALLOCATION_N_CONTINUE(loc)                                                                   \
    if ( !(loc) ) {                                                                                            \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d Encountered null pointer\n",__FUNCTION__, __LINE__));  \
        continue;                                                                                              \
    }

// Check for NULL ptr, set return var with provided status and goto provided label
#define SEP_CHECK_ALLOCATION_SET_RETURN_N_GOTO_CLEANUP(loc, ret_var, ret_status, goto_label)                   \
    if ( !(loc) ) {                                                                                            \
        ret_var = ret_status;                                                                                  \
        LOGIT((LOG_AREA_GENERAL|LOG_LEVEL_ERROR, "%s:%d Encountered null pointer\n",__FUNCTION__, __LINE__));  \
        goto goto_label;                                                                                       \
    }

#define SEP_CHECK_ALLOCATION_N_RET_ASSIGNED_VAL DRV_CHECK_PTR_N_RET_ASSIGNED_VAL

#endif

