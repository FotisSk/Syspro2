#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <sys/wait.h>


#include "definition.h"

int main(int argc, char const *argv[])
{
	int a=0, b=0, c=0, i, job_pools, readfd, writefd, bytesRead, status, exit_status;
	char *w="-w", *r="-r", *l="-l", *n="-n", *fifo_READ, *fifo_WRITE, *path, *split, **next;
	char buf[buf_SIZE], buf_OK[] = "OK";
	pid_t pid;

	memset(buf, 0, buf_SIZE);
	if(argc == 9)
	{
		for(i=1; i<9; i=i+2)
		{
			if(strcmp(argv[i], r) == 0)
			{
				a = 1;
				fifo_READ = malloc( (strlen(argv[i+1])+1) * sizeof(char) );
				strcpy(fifo_READ, argv[i+1]);
			}
			else if(strcmp(argv[i], w) == 0)
			{
				b = 1;
				fifo_WRITE = malloc( (strlen(argv[i+1])+1) * sizeof(char) );
				strcpy(fifo_WRITE, argv[i+1]);
			}
			else if(strcmp(argv[i], l) == 0)
			{
				c = 1;
				path = malloc( (strlen(argv[i+1])+1) * sizeof(char) );
				strcpy(path, argv[i+1]);
			}
			else if(strcmp(argv[i], n) == 0)
			{
				job_pools = atoi(argv[i+1]);
			}
			else
			{
				if(a == 1)
					free(fifo_READ);
				if(b == 1)
					free(fifo_WRITE);
				if(c == 1)
					free(path);

				printf("(coord) acceptable flags: -w -r -l -n\n");
				exit(1);
			}
		}
	}

	/* ________FIFO CREATION________ */
	if( mkfifo(fifo_READ, PERMS) < 0 && errno != EEXIST)
	{
		perror("can't create FIFO (read)");
		exit(EXIT_FAILURE);
	}
	if( mkfifo(fifo_WRITE, PERMS) < 0 && errno != EEXIST )
	{
		perror("can't create FIFO (write)");
		exit(EXIT_FAILURE);
	}

	/* OPEN FIFOs */
	if( (readfd = open(fifo_READ, O_RDONLY | O_NONBLOCK)) < 0)
	{
		perror("coord: can't open read FIFO");
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		if((bytesRead = read(readfd, buf, buf_SIZE)) == -1)
		{
			if( (writefd = open(fifo_WRITE, O_WRONLY | O_NONBLOCK)) < 0)
			{
				perror("coord: can't open write FIFO");
				exit(EXIT_FAILURE);
			}
			printf("bytesRead: %d, writefd: %d\n",bytesRead, writefd );
			break;
		}
	}

	/* ________READ FROM FIFOs________ */

	char *arguments[1024];
	while(1)
	{	
		if( (bytesRead = read(readfd, buf, buf_SIZE)) > 0)
		{
			write(writefd, buf_OK, 3); //eidopoihse to allo akro oti diavastike auto pou esteile

			split = strtok(buf, " \n");
			if(strcmp(split, SUBMIT) == 0)
			{
				next = arguments;
				split = strtok(NULL, " \n");
				while(split)
				{
					*next++ = split;
					//printf("%s\n",split);
					split = strtok(NULL, " \n");
				}
				*next = NULL;
				printf("Checking:\n");
				for(next = arguments; *next != 0; next++)
					puts(*next);

				pid = fork();
				if(pid == 0)	//child
					execvp(arguments[0], arguments);
				else if(pid > 0) 	//father
				{
					printf("I am parent process %d with child %d\n", getpid(), pid);

					waitpid(-1, &status, WNOHANG);

					if(WIFEXITED(status)) //an einai != 0 diladi true
					{
						exit_status = WEXITSTATUS(status);
						printf("exit status from %d was %d\n", pid, exit_status);
					}
				}
				else 	//fork failed
				{
					perror("could not fork");
					exit(EXIT_FAILURE);
				}
			}
			else if(strcmp(split, STATUS) == 0)
			{

			}
			else if(strcmp(split, STATUSALL) == 0)
			{

			}
			else if(strcmp(split, SHOWACTIVE) == 0)
			{
				printf("show active\n");
			}
			else if(strcmp(split, SHOWPOOLS) == 0)
			{

			}
			else if(strcmp(split, SHOWFINISHED) == 0)
			{

			}
			else if(strcmp(split, SUSPEND) == 0)
			{

			}
			else if(strcmp(split, RESUME) == 0)
			{

			}
			else if(strcmp(split, SHUTDOWN) == 0)
			{

			}
			else
			{
				printf("Wrong command given\n");
			}

			//printf("%s\n", buf);
			memset(buf, 0, buf_SIZE);
			memset(arguments, 0, 1024);
			printf("\n");
		}
		else if( bytesRead == 0)
		{
			printf("End Of File\n");
			memset(buf, 0, buf_SIZE);
			break;
		}
		else if(bytesRead < 0)
		{
			//printf("n: %d\n", bytesRead);
			memset(buf, 0, buf_SIZE);

		}
	}

	close(readfd);
	close(writefd);
	free(fifo_READ);
	free(fifo_WRITE);
	free(path);
	exit(0);
}