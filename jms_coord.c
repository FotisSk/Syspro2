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
	int a=0, b=0, c=0, i, maxJobsInPool, readfd, writefd, bytesRead, status, exit_status, jobCounter, 
		poolCounter, jobID, nextAvailablePos, size, poolNum, tempReadFd_coord, tempWriteFd_coord, tempReadFd_pool, tempWriteFd_pool;

	char *w="-w", *r="-r", *l="-l", *n="-n", *fifo_READ, *fifo_WRITE, *path, *split, **next;
	char buf[buf_SIZE], poolName_in[15], poolName_out[15], poolBuf[buf_SIZE], buf_OK[] = "OK";
	coordToPool *coordStorageArray;
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
				maxJobsInPool = atoi(argv[i+1]);
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
	nextAvailablePos = 0;
	size = 10;
	coordStorageArray = malloc(size * sizeof(coordToPool));
	
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

	char *arguments[1024];
	jobCounter = 0;
	poolCounter = 0;
	jobID = 0;
	while(1)
	{	
		if( (bytesRead = read(readfd, buf, buf_SIZE)) > 0)
		{
			write(writefd, buf_OK, 3); //eidopoihse to allo akro oti diavastike auto pou esteile


			split = strtok(buf, " \n");
			if(strcmp(split, SUBMIT) == 0)
			{
				jobCounter++;
				jobID++;

				if(poolCounter*maxJobsInPool >= jobCounter)	//de xreiazetai neo pool
				{
					//poolNum = (jobCounter-1) / maxJobsInPool;
					//sprintf(poolName_out, "pool%d_out", poolNum);
					//write(, buff, buf_SIZE);
				}
				else 	//xreiazetai neo pool
				{
					poolCounter++;
				
					/* ________FIFO CREATION________ */
					sprintf(poolName_in, "pool%d_in", poolCounter);
					if( mkfifo(poolName_in, PERMS) < 0 && errno != EEXIST)
					{
						perror("can't create FIFO (read)");
						exit(EXIT_FAILURE);
					}

					sprintf(poolName_out, "pool%d_out", poolCounter);
					if( mkfifo(poolName_out, PERMS) < 0 && errno != EEXIST )
					{
						perror("can't create FIFO (write)");
						exit(EXIT_FAILURE);
					}

					printf("created pool%d FIFOs\n", poolCounter);

					pid = fork();
					if(pid > 0)	//coord (father)
					{
						/* OPEN FIFOs */
						if( (tempReadFd_coord = open(poolName_in, O_RDONLY | O_NONBLOCK)) < 0)
						{
							perror("(coord) can't open read FIFO");
							exit(EXIT_FAILURE);
						}

						while(1)
						{
							if((bytesRead = read(tempReadFd_coord, buf, buf_SIZE)) == -1)
							{
								if( (tempWriteFd_coord = open(poolName_out, O_WRONLY | O_NONBLOCK)) < 0)
								{
									perror("(coord) can't open write FIFO");
									exit(EXIT_FAILURE);
								}
								printf("bytesRead: %d, writefd: %d\n",bytesRead, tempWriteFd_coord );
								break;
							}
						}
						printf("opened pool%d FIFOs\n", poolCounter);

						if(nextAvailablePos >= size)
						{
							printf("realloc\n");
							size = 2*size;
							coordStorageArray = realloc(coordStorageArray, size);
						}
						coordStorageArray[nextAvailablePos].poolPid = pid;
						coordStorageArray[nextAvailablePos].poolNum = poolCounter;
						coordStorageArray[nextAvailablePos].in = tempReadFd_coord;
						coordStorageArray[nextAvailablePos].out = tempWriteFd_coord;
						nextAvailablePos++;

						//memset(poolName_in, 0, 15);
						//memset(poolName_out, 0, 15);
					}
					else if(pid == 0)	//pool (child)
					{
						/* OPEN FIFOs */
						if( (tempReadFd_pool = open(poolName_out, O_RDONLY | O_NONBLOCK)) < 0)
						{
							perror("coord: can't open read FIFO");
							exit(EXIT_FAILURE);
						}
						if( (tempWriteFd_pool = open(poolName_in, O_WRONLY | O_NONBLOCK)) < 0)
						{
							perror("coord: can't open read FIFO");
							exit(EXIT_FAILURE);
						}

						/* Creating argument array */
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
						if(pid == 0)	//job (child)
						{
							//thelei mkdir, create 2 arxeia mesa kai anakatefthinsi se auta
							//...

							execvp(arguments[0], arguments);

						}
						else if(pid > 0) 	//pool (father)
						{
							printf("I am pool process %d with child %d\n", getpid(), pid);

							sleep(1); //testing only. na figei
							waitpid(-1, &status, WNOHANG);

							if(WIFEXITED(status)) //an einai != 0 diladi true
							{
								exit_status = WEXITSTATUS(status);
								printf("exit status from %d was %d\n", pid, exit_status);
							}
						}
						else 
						{
							perror("(pool) could not fork");
							exit(EXIT_FAILURE);
						}

					}
					else 
					{
						perror("(coord) could not fork");
						exit(EXIT_FAILURE);
					}

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

			sleep(1); //testing only. na figei
			waitpid(-1, &status, WNOHANG);

			break;
		}
		else if(bytesRead < 0)
		{
			//printf("n: %d\n", bytesRead);
			memset(buf, 0, buf_SIZE);

		}
		
		//elegxos an termatise kapoio paidi-pool
		printf("elegxos lul\n");
		for(i=0; i<nextAvailablePos; i++)
		{
			printf("i: %d\n", i);
			if(waitpid(coordStorageArray[i].poolPid, &status, WNOHANG) > 0)	//tote auto to paidi-pool termatise
			{
				//do stuff
				//...
				printf("pool (child) with pid: %d has terminated\n", coordStorageArray[i].poolPid);
					
				close(coordStorageArray[i].in);
				close(coordStorageArray[i].out);
			}
		}
	}

	close(readfd);
	close(writefd);
	free(fifo_READ);
	free(fifo_WRITE);
	free(path);
	exit(0);
}