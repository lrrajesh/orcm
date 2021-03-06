/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007      Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * Copyright (c) 2014      Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "orcm_config.h"
#include "orcm/constants.h"

#include <string.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif  /* HAVE_SYS_TIME_H */
#ifdef HAVE_HNP_H
#include <hnp.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "opal/util/show_help.h"
#include "opal/dss/dss.h"
#include "opal/dss/dss_types.h"

#include "orte/util/show_help.h"
#include "orte/runtime/orte_globals.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orcm/mca/notifier/base/base.h"
#include "notifier_hnp.h"

/* Static API's */
static int init(void);
static void finalize(void);
static void mylog(orcm_notifier_base_severity_t severity, int errcode,
                  const char *msg, va_list ap);
static void myhelplog(orcm_notifier_base_severity_t severity, int errcode,
                      const char *filename, const char *topic, va_list ap);
static void mypeerlog(orcm_notifier_base_severity_t severity, int errcode,
                      orte_process_name_t *peer_proc, const char *msg,
                      va_list ap);
static void myeventlog(const char *msg);

/* Module definition */
orcm_notifier_base_module_t orcm_notifier_hnp_module = {
    init,
    finalize,
    mylog,
    myhelplog,
    mypeerlog,
    myeventlog
};

static int send_command(orcm_notifier_base_severity_t severity, int errcode,
                        char *msg)
{
    opal_buffer_t *buf;
    int rc;
    uint8_t u8 = (uint8_t) severity;
    uint32_t u32 = (uint32_t) errcode;

    buf = OBJ_NEW(opal_buffer_t);
    if (NULL == buf) {
        return ORCM_ERR_OUT_OF_RESOURCE;
    }

    /* Pack the severity (need to use a fixed-size type) */
    if (ORCM_SUCCESS != (rc = opal_dss.pack(buf, &u8, 1, OPAL_UINT8))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(buf);
        return rc;
    }

    /* Pack the errcode (need to use a fixed-size type) */
    if (ORCM_SUCCESS != (rc = opal_dss.pack(buf, &u32, 1, OPAL_UINT32))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(buf);
        return rc;
    }

    /* Pack the message */
    if (ORCM_SUCCESS != (rc = opal_dss.pack(buf, &msg, 1, OPAL_STRING))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(buf);
        return rc;
    }

    /* Now send the buffer (rc = number of bytes sent) */
    rc = orte_rml.send_buffer_nb(ORTE_PROC_MY_HNP, buf,
                                 ORTE_RML_TAG_NOTIFIER_HNP,
                                 orte_rml_send_callback, NULL);
    if (ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(buf);
        return rc;
    }

    return ORCM_SUCCESS;
}

static int init(void)
{
    /* If I'm the HNP, post a non-blocking RML receive */
    if (ORTE_PROC_IS_HNP) {
        orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD,
                                ORTE_RML_TAG_NOTIFIER_HNP,
                                ORTE_RML_PERSISTENT,
                                orcm_notifier_hnp_recv_cb,
                                NULL);
    }

    return ORCM_SUCCESS;
}

static void finalize(void)
{
    /* If I'm the HNP, then cancel the non-blocking RML receive */
    if (ORTE_PROC_IS_HNP) {
        orte_rml.recv_cancel(ORTE_NAME_WILDCARD, ORTE_RML_TAG_NOTIFIER_HNP);
    }
}

static void mylog(orcm_notifier_base_severity_t severity, int errcode,
                  const char *msg, va_list ap)
{
    char *output;

    /* If there was a message, output it */
    vasprintf(&output, msg, ap);

    if (NULL != output) {
        if (ORTE_PROC_IS_HNP) {
            /* output it locally */
            orte_show_help("orcm_notifier_hnp.txt", "notifier message", false, output);
        } else {
            send_command(severity, errcode, output);
        }
        free(output);
    }
}

static void myhelplog(orcm_notifier_base_severity_t severity, int errcode,
                      const char *filename, const char *topic, va_list ap)
{
    char *output;

    output = opal_show_help_vstring(filename, topic, false, ap);

    if (NULL != output) {
        if (ORTE_PROC_IS_HNP) {
            /* output it locally */
            orte_show_help("orcm_notifier_hnp.txt", "notifier message", false, output);
         } else {
            send_command(severity, errcode, output);
        }
        free(output);
    }
}

static void mypeerlog(orcm_notifier_base_severity_t severity, int errcode,
                      orte_process_name_t *peer_proc, const char *msg,
                      va_list ap)
{
    char *buf = orcm_notifier_base_peer_log(errcode, peer_proc, msg, ap);

    if (NULL != buf) {
        if (ORTE_PROC_IS_HNP) {
            /* output it locally */
            orte_show_help("orcm_notifier_hnp.txt", "notifier message", false, buf);
        } else {
            send_command(severity, errcode, buf);
        }
        free(buf);
    }
}

static void myeventlog(const char *msg)
{
    if (ORTE_PROC_IS_HNP) {
        /* output it locally */
        orte_show_help("orcm_notifier_hnp.txt", "notifier message", false, (char*)msg);
    } else {
        send_command(ORCM_NOTIFIER_NOTICE, ORCM_SUCCESS, (char *)msg);
    }
}
