#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/select.h>

#include "definition.h"

int main(int argc, char const *argv[])
{
	int a=0, b=0, c=0, i, job_pools, readfd, writefd, ret;
	char *w="-w", *r="-r", *l="-l", *n="-n", *fifo_READ, *fifo_WRITE, *path;
	struct timeval timeout;
	fd_set selectWriteFds;

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
	/* FIFO CREATION */
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
	//sleep(10);
	//while(1)
	//{
		//FD_ZERO(&selectWriteFds);
		//FD_SET(readfd, &selectWriteFds);

		//timeout.tv_sec = 15;
		//timeout.tv_usec = 0;
		//ret = select(readfd+1, &selectWriteFds, NULL, NULL, &timeout);

		if( (writefd = open(fifo_WRITE, O_WRONLY | O_NONBLOCK)) < 0)
		{
			perror("coord: can't open write FIFO");
			exit(EXIT_FAILURE);
		}
		//break;
	//}
	close(readfd);
	close(writefd);
	free(fifo_READ);
	free(fifo_WRITE);
	free(path);
	exit(0);
}