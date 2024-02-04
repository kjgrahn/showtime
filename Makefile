SHELL=/bin/bash

CXXFLAGS=-W -Wall -pedantic -std=c++14 -g -Os
CPPFLAGS=
ARFLAGS=rTP

.PHONY: all
all: test/test
all: libshowtime.a

libshowtime.a: showtime.o
	$(AR) $(ARFLAGS) $@ $^

# tests

.PHONY:  checkv
check: test/test
	./test/test
checkv: test/test
	valgrind -q ./test/test -v

test/test: test/test.o libshowtime.a test/libtest.a
	$(CXX) $(CXXFLAGS) -o $@ test/test.o -Ltest/ -ltest -L. -lshowtime

test/test.cc: test/libtest.a
	orchis -o $@ $^

test/libtest.a: test/showtime.o
	$(AR) $(ARFLAGS) $@ $^

test/%.o: CPPFLAGS+=-I.

# other

.PHONY: clean
clean:
	$(RM) *.o lib*.a
	$(RM) test/*.o test/lib*.a
	$(RM) test/test test/test.cc
	$(RM) -r dep
	$(RM) TAGS

love:
	@echo "not war?"

# DO NOT DELETE

$(shell mkdir -p dep/test)
DEPFLAGS=-MT $@ -MMD -MP -MF dep/$*.Td
COMPILE.cc=$(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

%.o: %.cc
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	@mv dep/$*.{Td,d}

dep/%.d: ;
dep/test/%.d: ;
-include dep/*.d
-include dep/test/*.d
