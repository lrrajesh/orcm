# -*- shell-script -*-
#
# Copyright (c) 2013-2014 Intel, Inc. All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

AC_DEFUN([ORCM_CONFIG_FILES],[
    AC_CONFIG_FILES([
    orcm/Makefile
    orcm/etc/Makefile
    orcm/include/Makefile

    orcm/tools/octl/Makefile
    orcm/tools/orcmctrld/Makefile
    orcm/tools/orcmd/Makefile
    orcm/tools/ops/Makefile
    orcm/tools/orcm-info/Makefile
    orcm/tools/orcmsched/Makefile
    orcm/tools/otop/Makefile
    orcm/tools/orun/Makefile
    orcm/tools/osub/Makefile
    orcm/tools/oqueue/Makefile
    orcm/tools/orcmsd/Makefile
    orcm/tools/wrappers/Makefile
    orcm/tools/wrappers/orcm.pc
    orcm/tools/wrappers/orcmcc-wrapper-data.txt
    orcm/tools/orcm-emulator/Makefile
    orcm/tools/opwrvirus/Makefile
    orcm/tools/otestcmd/Makefile
    ])
])
