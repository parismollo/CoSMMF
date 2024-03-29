CC=gcc 
CFLAGS=-I./include

run_test:
	make internal_tests
	./internal_tests

internal_tests: src/internal.c tests/internal_tests.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f internal_tests
	rm -f files/*
	rm -rf logs/*
	

