include ../make.rules

all:

install:
	@echo '$(COL)$@$(NORM)'
	@install -d $(DATADIR)
	@find . -\( -name CVS -prune -\) -o -type f -exec install -m644 \{\} $(DATADIR) \;

clean:

