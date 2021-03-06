/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2013 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2007      Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2007      Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * Copyright (c) 2010      Oracle and/or its affiliates.  All rights reserved.
 * Copyright (c) 2014      Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orcm_config.h"
#include "orcm/constants.h"

#include <stdio.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNISTD_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif  /*  HAVE_STDLIB_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif  /* HAVE_SYS_STAT_H */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif  /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif  /* HAVE_SYS_WAIT_H */
#ifdef HAVE_STRING_H
#include <string.h>
#endif  /* HAVE_STRING_H */
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif  /* HAVE_DIRENT_H */

#include "opal/dss/dss.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/mca/event/event.h"
#include "opal/util/basename.h"
#include "opal/util/cmd_line.h"
#include "opal/util/output.h"
#include "opal/util/opal_environ.h"
#include "opal/util/show_help.h"
#include "opal/mca/base/base.h"
#include "opal/runtime/opal.h"
#if OPAL_ENABLE_FT_CR == 1
#include "opal/runtime/opal_cr.h"
#endif

#include "orte/runtime/orte_wait.h"
#include "orte/util/error_strings.h"
#include "orte/util/show_help.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/rml/rml.h"

#include "orcm/runtime/runtime.h"

#include "orcm/mca/scd/base/base.h"

/******************
 * Local Functions
 ******************/
static int orcm_octl_init(int argc, char *argv[]);
static int parse_args(int argc, char *argv[]);

/*****************************************
 * Global Vars for Command line Arguments
 *****************************************/
typedef struct {
    bool help;
    bool verbose;
    int output;
} orcm_octl_globals_t;

orcm_octl_globals_t orcm_octl_globals;

opal_cmd_line_init_t cmd_line_opts[] = {
    { NULL,
      'h', NULL, "help",
      0,
      &orcm_octl_globals.help, OPAL_CMD_LINE_TYPE_BOOL,
      "This help message" },

    { NULL,
      'v', NULL, "verbose",
      0,
      &orcm_octl_globals.verbose, OPAL_CMD_LINE_TYPE_BOOL,
      "Be Verbose" },

    /* End of list */
    { NULL,
      '\0', NULL, NULL,
      0,
      NULL, OPAL_CMD_LINE_TYPE_NULL,
      NULL }
};

int
main(int argc, char *argv[])
{
    orcm_scd_cmd_flag_t command;
    orcm_alloc_id_t id;
    opal_buffer_t *buf;
    orte_rml_recv_cb_t xfer;
    long session;
    int rc, n, result;

    /* initialize, parse command line, and setup frameworks */
    orcm_octl_init(argc, argv);

    errno = 0;
    if (0 == strncmp(argv[1], "cancel", 6)) {
        if (!argv[2]) {
            opal_output(1, "incorrect arguments to \"%s cancel\"\n", argv[0]);
            return ORTE_ERROR;
        }
        session = strtol(argv[2], NULL, 10);
        if (errno) {
            perror("OCTL ERROR: ");
            exit(1);
        }
        if (session > 0) {
            /* setup to receive the result */
            OBJ_CONSTRUCT(&xfer, orte_rml_recv_cb_t);
            xfer.active = true;
            orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD,
                                    ORCM_RML_TAG_SCD,
                                    ORTE_RML_NON_PERSISTENT,
                                    orte_rml_recv_callback, &xfer);

            /* send it to the scheduler */
            buf = OBJ_NEW(opal_buffer_t);

            command = ORCM_SESSION_CANCEL_COMMAND;
            /* pack the cancel command flag */
            if (OPAL_SUCCESS != (rc = opal_dss.pack(buf, &command,1, ORCM_SCD_CMD_T))) {
                ORTE_ERROR_LOG(rc);
                OBJ_RELEASE(buf);
                OBJ_DESTRUCT(&xfer);
                return rc;
            }

            id = (orcm_alloc_id_t)session;
            /* pack the session id */
            if (OPAL_SUCCESS != (rc = opal_dss.pack(buf, &id, 1, ORCM_ALLOC_ID_T))) {
                ORTE_ERROR_LOG(rc);
                OBJ_RELEASE(buf);
                OBJ_DESTRUCT(&xfer);
                return rc;
            }
            if (ORTE_SUCCESS != (rc = orte_rml.send_buffer_nb(ORTE_PROC_MY_SCHEDULER, buf,
                                                              ORCM_RML_TAG_SCD,
                                                              orte_rml_send_callback, NULL))) {
                ORTE_ERROR_LOG(rc);
                OBJ_RELEASE(buf);
                OBJ_DESTRUCT(&xfer);
                return rc;
            }
            /* get result */
            n=1;
            ORTE_WAIT_FOR_COMPLETION(xfer.active);
            if (OPAL_SUCCESS != (rc = opal_dss.unpack(&xfer.data, &result, &n, OPAL_INT))) {
                ORTE_ERROR_LOG(rc);
                OBJ_DESTRUCT(&xfer);
                return rc;
            }
            if (0 == result) {
                opal_output(0, "Success\n");
            } else {
                opal_output(0, "Failure\n");
            }
        } else {
            opal_output(1, "Invalid SESSION ID\n");
            return ORTE_ERROR;
        }
    } else {
        opal_output(1, "Unknown octl command: %s\n", argv[1]);
            return ORTE_ERROR;
    }

    if (ORTE_SUCCESS != orcm_finalize()) {
        fprintf(stderr, "Failed orcm_finalize\n");
        exit(1);
    }

    return ORTE_SUCCESS;
}

static int parse_args(int argc, char *argv[]) 
{
    int ret;
    opal_cmd_line_t cmd_line;
    orcm_octl_globals_t tmp = { false,    /* help */
                                false,    /* verbose */
                                -1};      /* output */

    orcm_octl_globals = tmp;
    char *args = NULL;
    char *str = NULL;

    /* Parse the command line options */
    opal_cmd_line_create(&cmd_line, cmd_line_opts);
    
    mca_base_cmd_line_setup(&cmd_line);
    ret = opal_cmd_line_parse(&cmd_line, true, argc, argv);
    
    if (OPAL_SUCCESS != ret) {
        if (OPAL_ERR_SILENT != ret) {
            fprintf(stderr, "%s: command line error (%s)\n", argv[0],
                    opal_strerror(ret));
        }
        return ret;
    }

    /**
     * Now start parsing our specific arguments
     */
    if (orcm_octl_globals.help) {
        args = opal_cmd_line_get_usage_msg(&cmd_line);
        str = opal_show_help_string("help-octl.txt", "usage", true,
                                    args);
        if (NULL != str) {
            printf("%s", str);
            free(str);
        }
        free(args);
        /* If we show the help message, that should be all we do */
        exit(0);
    }

    /*
     * Since this process can now handle MCA/GMCA parameters, make sure to
     * process them.
     */
    mca_base_cmd_line_process_args(&cmd_line, &environ, &environ);

    return ORTE_SUCCESS;
}

static int orcm_octl_init(int argc, char *argv[]) 
{
    int ret;

    /*
     * Make sure to init util before parse_args
     * to ensure installdirs is setup properly
     * before calling mca_base_open();
     */
    if( ORTE_SUCCESS != (ret = opal_init_util(&argc, &argv)) ) {
        return ret;
    }

    /*
     * Parse Command Line Arguments
     */
    if (ORTE_SUCCESS != (ret = parse_args(argc, argv))) {
        return ret;
    }

    /*
     * Setup OPAL Output handle from the verbose argument
     */
    if( orcm_octl_globals.verbose ) {
        orcm_octl_globals.output = opal_output_open(NULL);
        opal_output_set_verbosity(orcm_octl_globals.output, 10);
    } else {
        orcm_octl_globals.output = 0; /* Default=STDERR */
    }

    ret = orcm_init(ORCM_TOOL);

    return ret;
}
