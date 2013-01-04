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

instdoc_dir=@docdir@/@PACKAGE@-@VERSION@/INSTALL

instdoc_name=INSTALL

SUBDIRS=progref

all-local: $(instdoc_name)

$(instdoc_name): $(instdoc_name).sgml
	test -z "$(db2html)" && mkdir $@ || $(db2html) $<

maintainer-clean-local:
	rm -rf $(instdoc_name)

clean-local:
	rm -rf $(instdoc_name).junk DBTOHTML_OUTPUT_DIR*

dist-hook: all-local
	cp -r $(srcdir)/$(instdoc_name) $(distdir)

install-data-local:
	test -z "$(db2html)" || ( \
		$(mkinstalldirs) $(DESTDIR)$(instdoc_dir) $(DESTDIR)$(instdoc_dir)/stylesheet-images; \
		for i in $(srcdir)/$(instdoc_name)/*; do \
			if test -e $$i; then \
				fn=`basename $$i`; \
				$(INSTALL_DATA) $$i $(DESTDIR)$(instdoc_dir)/$$fn; \
			fi; \
		done; \
		for i in $(srcdir)/$(instdoc_name)/stylesheet-images/*; do \
			if test -e $$i; then \
				fn=`basename $$i`; \
				$(INSTALL_DATA) $$i $(DESTDIR)$(instdoc_dir)/stylesheet-images/$$fn; \
			fi; \
		done; \
	)

uninstall-local:
	if test -d $(DESTDIR)$(instdoc_dir); then \
		rm -rf $(DESTDIR)$(instdoc_dir)/*; \
		rmdir $(DESTDIR)$(instdoc_dir) || true; \
	fi

EXTRA_DIST=$(instdoc_name).sgml
