#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include "definition.h"

int main(int argc, char const *argv[])
{
	int i, a=0, b=0, c=0, readfd, writefd, n, bytesRead;
	char fileBuf[fileBuf_SIZE], userBuf[userBuf_SIZE], buf_reply[3];
	char *w="-w", *r="-r", *o="-o", *fifo_READ, *fifo_WRITE, *fileName;
	FILE *fp;

	if(argc == 7)	//diavasma apo arxeio
	{
		for(i=1; i<7; i=i+2)
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
			else if(strcmp(argv[i], o) == 0)
			{
				c = 1;
				fileName = malloc( (strlen(argv[i+1])+1) * sizeof(char) );
				strcpy(fileName, argv[i+1]);
			}
			else
			{
				if(a == 1)
					free(fifo_READ);
				if(b == 1)
					free(fifo_WRITE);
				if(c == 1)
					free(fileName);

				printf("(console) acceptable flags: -w -r -o\n");
				exit(1);
			}
		}
	}
	//printf("console ~ jms_out: %s, jms_in: %s, file name: %s\n", fifo_READ, fifo_WRITE, fileName);


	/* ________OPEN FIFOs________ */
	if( (readfd = open(fifo_READ, O_RDONLY | O_NONBLOCK)) < 0)
	{
		perror("console: can't open read FIFO");
		exit(EXIT_FAILURE);
	}
	if( (writefd = open(fifo_WRITE, O_WRONLY | O_NONBLOCK)) < 0)
	{
		perror("console: can't open write FIFO");
		exit(EXIT_FAILURE);
	}
	
	/* ________OPEN FILE________ */
	fp = fopen(fileName, "r");
	if(fp)
	{
		while( fgets(fileBuf, fileBuf_SIZE, fp) )
		{
			/* ________WRITE TO FIFO________ */
			n = strlen(fileBuf);
			if(fileBuf[n-1] == '\n')	//exclude '\n'
				n--;
			write(writefd, fileBuf, n);
			memset(fileBuf, 0, fileBuf_SIZE);

			while(1)
			{
				if( (bytesRead = read(readfd, buf_reply, 3)) > 0)
				{
					memset(buf_reply, 0, 3);
					break;
				}
				else
				{
					//printf("bytesRead: %d\n", bytesRead);
				}
			}

		}
	}
	//NA MI MPEI AN EXEI DOTHEI SHUTDOWN
	//if( den exei dothei edoli shutdown, tote... )
	printf("You are in control\n");
	printf("> ");
	while( fgets(userBuf, userBuf_SIZE, stdin) != NULL )
	{
		/* ________WRITE TO FIFO________ */
		n = strlen(userBuf);
		if(userBuf[n-1] == '\n')	//exclude '\n'
			n--;
		write(writefd, userBuf, n);
		memset(userBuf, 0, userBuf_SIZE);

		while(1)
		{
			if( (bytesRead = read(readfd, buf_reply, 3)) > 0)
			{
				memset(buf_reply, 0, 3);
				break;
			}
			else
			{
				//printf("bytesRead: %d\n", bytesRead);
			}
		}
		printf("> ");
	}
	/**********************************************************************************************/

	close(readfd);
	close(writefd);
	fclose(fp);
	free(fifo_READ);
	free(fifo_WRITE);
	free(fileName);
	exit(0);
}