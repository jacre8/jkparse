prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

BINDIR = bin
VPATH = $(BINDIR)

CC ?= gcc
CFLAGS ?= -s -O2
CFLAGS += -Wall -Wshadow -Wimplicit -Wextra -Winline -Wundef -Wmissing-declarations \
-Wstrict-prototypes -Wmissing-prototypes -Wno-unused-parameter -Wtrampolines

jkparse : jkparse.c | $(BINDIR)
	$(CC) $(CFLAGS) $(JKPARSE_FLAGS) $(shell if [ -n "$(USE_SHELL_PRINTF)" ];then \
			echo -D USE_SHELL_PRINTF=\\\"$(USE_SHELL_PRINTF)\\\" -D \
				SHELL_BASENAME=\\\"$(notdir $(USE_SHELL_PRINTF))\\\"; \
		elif [ -n "$(PRINTF_EXECUTABLE)" ];then \
			echo -D PRINTF_EXECUTABLE=\\\"$(PRINTF_EXECUTABLE)\\\"; \
		elif envPrintf=$$(which printf) && \
		$$envPrintf %q a >/dev/null 2>&1;then \
			echo -D \'PRINTF_EXECUTABLE="\"$$envPrintf\""\'; \
		elif \
			if [ -f /bin/ksh ];then \
				shellExec=/bin/ksh; \
			elif [ -f /bin/zsh ];then \
				shellExec=/bin/zsh; \
			elif [ -f /bin/bash ];then \
				shellExec=/bin/bash; \
			else \
				false; \
			fi; \
		then \
			echo -D USE_SHELL_PRINTF=\\\"$$shellExec\\\" \
				-D SHELL_BASENAME=\\\"$$(basename "$$shellExec")\\\"; \
		else \
			echo USABLE PRINTF NOT FOUND!  Compiling with the default location >&2; \
		fi; ) -o $(BINDIR)/jkparse $^ -ljson-c

install : $(DESTDIR)$(bindir)/jkparse

.PHONY : clean
clean :
	rm -f $(BINDIR)/jkparse

$(DESTDIR)$(bindir)/jkparse : jkparse | $(DESTDIR)$(bindir)
	install -o root -g root -m 0755 bin/jkparse "$(DESTDIR)$(bindir)"

$(DESTDIR)$(bindir) : | $(DESTDIR)$(exec_prefix)

$(DESTDIR)$(exec_prefix) : | $(DESTDIR)

#  Directory targets.
$(BINDIR) $(DESTDIR) $(DESTDIR)$(exec_prefix) $(DESTDIR)$(bindir) :
	@if ! [ -d "$@" ] && ! mkdir -p "$@"; then \
		echo Error. Unable to create the output directory: "$@"; \
		exit 1; \
	fi
