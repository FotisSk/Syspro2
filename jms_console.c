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
	int i, a=0, b=0, c=0, readfd, writefd, n, bytesRead, thereIsAFile, terminate;
	char fileBuf[fileBuf_SIZE], userBuf[userBuf_SIZE], messageFromCoord[buf_SIZE], buf_OK[] = "OK";
	char *w="-w", *r="-r", *o="-o", *fifo_READ, *fifo_WRITE, *fileName, *split;
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
				thereIsAFile = 1;
			}
			else
			{
				if(a == 1)
					free(fifo_READ);
				if(b == 1)
					free(fifo_WRITE);
				if(c == 1)
					free(fileName);

				printf("(console) acceptable flags: -w -r [-o]\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	else if(argc == 5) //no file given
	{
		thereIsAFile = 0;

		for(i=1; i<5; i=i+2)
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
			else
			{
				if(a == 1)
					free(fifo_READ);
				if(b == 1)
					free(fifo_WRITE);

				printf("(console) acceptable flags: -w -r [-o]\n");
				exit(EXIT_FAILURE);
			}
		}
	}	

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

	terminate = 0;
	memset(messageFromCoord, 0, buf_SIZE);

	if(thereIsAFile == 1)
	{
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

				split = strtok(fileBuf, " \n");
				if( (strcmp(split, SHUTDOWN) == 0) )
					terminate = 1;
				memset(fileBuf, 0, fileBuf_SIZE);

				while(1)
				{
					if( (bytesRead = read(readfd, messageFromCoord, buf_SIZE)) > 0)
					{
						if(strcmp(messageFromCoord, "PRINTEND") == 0)
						{
							printf("\n");
							memset(messageFromCoord, 0, buf_SIZE);
							break;
						}
						else
						{
							write(writefd, buf_OK, 3);
							printf("%s\n", messageFromCoord);
							memset(messageFromCoord, 0, buf_SIZE);
						}
					}
				}
				if(terminate == 1)
				{
					printf("Console terminated\n");
					break;
				}
			}
		}
		fclose(fp);
		free(fileName);
	}

	if(terminate == 0)
	{
		printf("You are in control\n");
		printf("> ");
		while( fgets(userBuf, userBuf_SIZE, stdin) != NULL )
		{
			/* ________WRITE TO FIFO________ */
			n = strlen(userBuf);
			if(userBuf[n-1] == '\n')	//exclude '\n'
				n--;
			write(writefd, userBuf, n);

			split = strtok(userBuf, " \n");
			if( (strcmp(split, SHUTDOWN) == 0) )
				terminate = 1;
			memset(userBuf, 0, userBuf_SIZE);

			while(1)
			{
				if( (bytesRead = read(readfd, messageFromCoord, buf_SIZE)) > 0)
				{
					if(strcmp(messageFromCoord, "PRINTEND") == 0)
					{
						printf("\n");
						memset(messageFromCoord, 0, buf_SIZE);
						break;
					}
					else
					{
						write(writefd, buf_OK, 3);
						printf("%s\n", messageFromCoord);
						memset(messageFromCoord, 0, buf_SIZE);		
					}
				}
			}
			if(terminate == 1)
			{
				printf("Console terminated\n");
				break;
			}
			else
				printf("> ");
		}
	}
	/**********************************************************************************************/

	close(readfd);
	close(writefd);
	free(fifo_READ);
	free(fifo_WRITE);
	exit(EXIT_SUCCESS);
}