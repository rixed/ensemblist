NAME=ensemblist
CC=gcc
ifdef DEBUG
DATADIR=/home/rixed/src/ensemblist/datas
COMPILE_FLAGS=-Wall -fno-builtin -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wredundant-decls -O0 -g `libmikmod-config --cflags` -DDATADIR=$(DATADIR)
LINK_FLAGS=-g -lm -L /usr/X11R6/lib -lXmu -lGL -lglut -lGLU -lpng `libmikmod-config --libs` -lefence -lpthread -lz
else
DATADIR=$(DESTDIR)/usr/share/$(NAME)
COMPILE_FLAGS=-Wall -O3 -fomit-frame-pointer `libmikmod-config --cflags` -DNDEBUG -DDATADIR=$(DATADIR)
LINK_FLAGS=-lm -L /usr/X11R6/lib -lXmu -lGL -lglut -lGLU -lpng `libmikmod-config --libs` -lpthread -lz
endif
APPLE_FRAMEWORKS=-framework GLUT -framework Cocoa -framework OpenGL
#uncomment the following if you want to compile on MacOS/X
#LINK_FLAGS=$(LINK_FLAGS) $(APPLE_FRAMEWORKS)
CFILES=$(wildcard *.c)
OFILES=$(patsubst %.c,%.o,$(CFILES))

SUFFIXES=.c .h .ok .o
COL=[3;36m
NORM=[0m

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
	@install -d $(DATADIR) $(DESTDIR)/usr/games
	@install -m755 $(NAME) $(DESTDIR)/usr/games
	@find datas/ -\( -name CVS -prune -\) -o -type f -exec install -m644 \{\} $(DATADIR) \;

deb:
	dpkg-buildpackage -rfakeroot

include depends
