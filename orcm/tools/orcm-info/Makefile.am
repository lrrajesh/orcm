#
# Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2009 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2010-2012 Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2008      Sun Microsystems, Inc.  All rights reserved.
# Copyright (c) 2014      Intel, Inc. All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

AM_CFLAGS = \
            -DOPAL_CONFIGURE_USER="\"@OPAL_CONFIGURE_USER@\"" \
            -DOPAL_CONFIGURE_HOST="\"@OPAL_CONFIGURE_HOST@\"" \
            -DOPAL_CONFIGURE_DATE="\"@OPAL_CONFIGURE_DATE@\"" \
            -DOPAL_BUILD_USER="\"$$USER\"" \
            -DOPAL_BUILD_HOST="\"`hostname`\"" \
            -DOPAL_BUILD_DATE="\"`date`\"" \
            -DOPAL_BUILD_CFLAGS="\"@CFLAGS@\"" \
            -DOPAL_BUILD_CPPFLAGS="\"@CPPFLAGS@\"" \
            -DOPAL_BUILD_CXXFLAGS="\"@CXXFLAGS@\"" \
            -DOPAL_BUILD_CXXCPPFLAGS="\"@CXXCPPFLAGS@\"" \
            -DOPAL_BUILD_FFLAGS="\"@FFLAGS@\"" \
            -DOPAL_BUILD_FCFLAGS="\"@FCFLAGS@\"" \
            -DOPAL_BUILD_LDFLAGS="\"@LDFLAGS@\"" \
            -DOPAL_BUILD_LIBS="\"@LIBS@\"" \
            -DOPAL_CC_ABSOLUTE="\"@OPAL_CC_ABSOLUTE@\"" \
            -DOPAL_CXX_ABSOLUTE="\"@OPAL_CXX_ABSOLUTE@\""
 
include $(top_srcdir)/Makefile.ompi-rules

man_pages = orcm-info.1
EXTRA_DIST = $(man_pages:.1=.1in)

if OPAL_INSTALL_BINARIES

bin_PROGRAMS = orcm-info

dist_orcmdata_DATA = help-orcm-info.txt

nodist_man_MANS = $(man_pages)

# Ensure that the man pages are rebuilt if the opal_config.h file
# changes; a "good enough" way to know if configure was run again (and
# therefore the release date or version may have changed)
$(nodist_man_MANS): $(top_builddir)/opal/include/opal_config.h

endif

orcm_info_SOURCES = \
        orcm-info.h \
        orcm-info.c \
        output.c \
        param.c \
        components.c \
        version.c

orcm_info_LDADD = $(top_builddir)/orcm/liborcm.la $(top_builddir)/orte/libopen-rte.la $(top_builddir)/opal/libopen-pal.la

clean-local:
	test -z "$(OPAL_CXX_TEMPLATE_REPOSITORY)" || rm -rf $(OPAL_CXX_TEMPLATE_REPOSITORY)

distclean-local:
	rm -f $(man_pages)
