include make.rules

all: $(NAME)

$(NAME): $(OFILES)
	@echo '$(COL)$@$(NORM)'
	$(CC) $(COMPILE_FLAGS) $(LINK_FLAGS) $^ -o $@
ifndef DEBUG
	strip $(NAME)
endif

.c.o:
	@echo '$(COL)$@$(NORM)'
	$(CC) $(COMPILE_FLAGS) -c $<

clean:
	@echo '$(COL)$@$(NORM)'
	@rm -f *.o depends $(NAME)

menu_digits.h: txt2digit.pl menu.txt
	@echo '$(COL)$@$(NORM)'
	@./txt2digit.pl < menu.txt > menu_digits.h

depends: $(wildcard *.c) $(wildcard *.h) menu_digits.h
	@echo '$(COL)$@$(NORM)'
	@if [ "$(CFILES)" ]; then \
		$(CC) -M $(CFILES) > $@ ; \
	else \
		touch $@ ; \
	fi

install: $(NAME)
	@echo '$(COL)$@$(NORM)'
	@install -d $(DESTDIR)/usr/games
	@install -m755 $(NAME) $(DESTDIR)/usr/games
	@install -m644 $(NAME)_32x32.xpm $(DESTDIR)/usr/share/pixmap

deb:
	@# -i to filter out CVS and such from source
	dpkg-buildpackage -rfakeroot -i

include depends
