include ../make.rules

all:

install: $(NAME)
	@echo '$(COL)$@$(NORM)'
	@install -d $(DATADIR)
	@find . -\( -name CVS -prune -\) -o -type f -exec install -m644 \{\} $(DATADIR) \;

deb:
	dpkg-buildpackage -rfakeroot

