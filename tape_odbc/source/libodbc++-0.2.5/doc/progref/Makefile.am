#  This file is part of libodbc++.
#  
#  Copyright (C) 1999-2000 Manush Dodunekov <manush@stendahls.net>
#   
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#   
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#  
#  You should have received a copy of the GNU Library General Public License
#  along with this library; see the file COPYING.  If not, write to
#  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#  Boston, MA 02111-1307, USA.

progref_dir=@docdir@/@PACKAGE@-@VERSION@/progref

stamp=progref-stamp

progref_dist_files=$(srcdir)/*.html $(srcdir)/*.css $(srcdir)/*.gif $(srcdir)/*.png footer
progref_local_files=*.html *.css *.gif *.png

incdir=$(top_srcdir)/include/odbc++
stripdir=$(top_srcdir)/include/

docheaders=\
$(incdir)/callablestatement.h \
$(incdir)/connection.h \
$(incdir)/databasemetadata.h \
$(incdir)/drivermanager.h \
$(incdir)/errorhandler.h \
$(incdir)/preparedstatement.h \
$(incdir)/resultset.h \
$(incdir)/resultsetmetadata.h \
$(incdir)/statement.h \
$(incdir)/types.h

docdeps=$(srcdir)/Doxyfile $(srcdir)/footer $(srcdir)/doxygen.css.in

all: $(stamp)

$(stamp): $(docheaders) $(docdeps)
	if test "x$(doxygen)" != "x"; then \
		rm -f $(progref_local_files); \
		PACKAGE=$(PACKAGE) VERSION=$(VERSION) \
		FOOTER=$(srcdir)/footer SOURCES="$(incdir)" \
		STRIPDIR=`cd $(stripdir) && pwd`/ \
		$(doxygen) $(srcdir)/Doxyfile && \
		cat $(srcdir)/doxygen.css.in >> doxygen.css; \
	fi
	echo timestamp > $(stamp)


dist-hook: $(stamp)
	for i in $(progref_dist_files) $(srcdir)/$(stamp); do \
		cp $$i $(distdir)/$$i; \
	done


# Docs are always present in srcdir in a distribution, so we grab them from there
# To rebuild those, one must use maintainer mode and srcdir=builddir
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(progref_dir); \
	for i in $(progref_dist_files); do \
		if test -r "$$i"; then \
			fn=`basename $$i` \
			$(INSTALL_DATA) $$i $(DESTDIR)$(progref_dir)/$$fn; \
		fi; \
	done

uninstall-local:
	if test -d $(DESTDIR)$(progref_dir); then \
		rm -f $(DESTDIR)$(progref_dir)/*; \
		rmdir $(DESTDIR)$(progref_dir) || true; \
	fi


maintainer-clean-local:
	rm -f $(stamp) $(progref_local_files)


EXTRA_DIST=Doxyfile footer doxygen.css.in
