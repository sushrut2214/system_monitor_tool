CC = gcc
CFLAGS = -Wall -g -Werror

## mySystemStats: build the mySystemStats executable
mySystemStats: mySystemStats.c stats_functions.c
	$(CC) $(CFLAGS) -o $@ $^ -lm

## clean: remove the mySystemStats executable and object files
.PHONY: clean
clean:
	rm -f mySystemStats *.o

## help: display this help message
.PHONY: help
help: makefile
	@echo "Available targets:"
	@sed -n 's/^##//p' $<