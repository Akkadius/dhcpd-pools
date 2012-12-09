EXTRA_DIST = doxy.conf.in introduction.dox

if ENABLE_DOXYGEN

doxyfile.stamp:
	$(DOXYGEN) doxy.conf
	date > doxyfile.stamp

CLEANFILES = doxyfile.stamp

all-local: doxyfile.stamp

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	-rm -rf html

endif
