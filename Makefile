mybin: test.c
	gcc test.c -o mybin -std=c99

.PHONY:clean
clean:
	rm -f mybin
