microshell: microshell.c
	gcc -Wall -ansi -o microshell microshell.c -lreadline

clean:
	rm -f *.o microshell
