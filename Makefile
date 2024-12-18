CPPFLAGS += -I.
CFLAGS += -Wall -W -Wextra -O3 -g
# RUNNER:=valgrind

.PHONY: all compiletests runtests clean

all: compiletests

check: runtests

runtests: test/printf test/sprintf
	set -x ; for prg in $^ ; do $(RUNNER) $$prg || exit $$? ; done

compiletests: tinyprintf_minimal.o tinyprintf_only_tfp_printf.o tinyprintf_only_tfp_sprintf.o

tinyprintf_minimal.o: tinyprintf.c
	$(COMPILE.c) \
	  -DTINYPRINTF_DEFINE_TFP_PRINTF=0 \
	  -DTINYPRINTF_DEFINE_TFP_SPRINTF=0 \
	  -DTINYPRINTF_OVERRIDE_LIBC=0 \
	  -o $@ $<
tinyprintf_only_tfp_printf.o: tinyprintf.c
	$(COMPILE.c) \
	  -DTINYPRINTF_DEFINE_TFP_PRINTF=1 \
	  -DTINYPRINTF_DEFINE_TFP_SPRINTF=0 \
	  -DTINYPRINTF_OVERRIDE_LIBC=0 \
	  -o $@ $<
tinyprintf_only_tfp_sprintf.o: tinyprintf.c
	$(COMPILE.c) \
	  -DTINYPRINTF_DEFINE_TFP_PRINTF=0 \
	  -DTINYPRINTF_DEFINE_TFP_SPRINTF=1 \
	  -DTINYPRINTF_OVERRIDE_LIBC=0 \
	  -o $@ $<

test/printf: test/printf.o tinyprintf.o
	$(LINK.c) -o $@ $^

test/sprintf: test/sprintf.o tinyprintf.o
	$(LINK.c) -o $@ $^

clean:
	$(RM) *.o test/*.o *~ test/*~ test/printf test/sprintf
