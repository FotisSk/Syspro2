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
#include <time.h>

#include "definition.h"
#include "status.h"
#include "statusall.h"
#include "showactive.h"
#include "showfinished.h"

int main(int argc, char const *argv[])
{
	int a=0, b=0, c=0, i, j, maxJobsInPool, readfd, writefd, bytesRead, status, exit_status, jobCounter, oldSize, timeDuration,
		poolCounter, jobID, nextAvailablePos, nextAvailablePos_pool, size, poolNum, tempReadFd_coord, tempWriteFd_coord, tempReadFd_pool,
		 tempWriteFd_pool, jobPID, stderrToFile, stdoutToFile, finishedJobs, jobID2, poolPos, secondsActive, jobID_pool, posInPoolStorage;

	char *w="-w", *r="-r", *l="-l", *n="-n", *fifo_READ, *fifo_WRITE, *path, *split, *split2, *split3, **next;
	char buf[buf_SIZE], copyBuf[buf_SIZE], copyBuf2[buf_SIZE], copyBuf_pool[buf_SIZE], message[buf_SIZE],
			messageToCoord[buf_SIZE], messageFromPool[buf_SIZE], messageFromConsole[buf_SIZE], messageFromCoord[buf_SIZE], messageToPool[buf_SIZE], messageToConsole[buf_SIZE],
				poolName_in[15], poolName_out[15], dirName[50], jobPath[100], poolBuf[buf_SIZE], buf_reply[3], buf_OK[] = "OK", buf_PRINTEND[] = "PRINTEND", buf_DONE[] = "DONE";

	poolInfo *coordStorageArray;
	jobInfo *poolStorageArray;
	pid_t pid, retVal;
	mode_t fdmode;
	time_t currentTime;
	struct tm myTime;

	memset(buf, 0, buf_SIZE);
	memset(copyBuf, 0, buf_SIZE);
	memset(copyBuf_pool, 0, buf_SIZE);
	
	memset(messageFromPool, 0, buf_SIZE); //einai tou coord
	memset(messageToPool, 0, buf_SIZE);
	memset(messageFromConsole, 0, buf_SIZE); //einai tou coord
	memset(messageToConsole, 0, buf_SIZE); //einai tou coord
	memset(messageFromCoord, 0, buf_SIZE); //einai tou pool
	memset(messageToCoord, 0, buf_SIZE); //einai tou pool
	

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
	size = 2;
	coordStorageArray = malloc(size * sizeof(poolInfo));
	
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
			//printf("bytesRead: %d, writefd: %d\n",bytesRead, writefd );
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
			printf("\n");
			printf("(coord) buf: %s\n",buf);
			
			//write(writefd, buf_OK, 3); //eidopoihse to allo akro oti diavastike auto pou esteile

			strcpy(copyBuf, buf);
			split = strtok(buf, " \n");
			if(strcmp(split, SUBMIT) == 0)
			{
				jobCounter++;
				jobID++;

				if(poolCounter*maxJobsInPool >= jobCounter)	//de xreiazetai neo pool
				{
					poolNum = (jobCounter-1) / maxJobsInPool;	//poolNum einai to pos ston pinaka me ta poolInfo
					write(coordStorageArray[poolNum].out, copyBuf, buf_SIZE);	//grafo sto pool tin entoli (submit)
					while(1)
					{
						if( (bytesRead = read(coordStorageArray[poolNum].in, messageFromPool, buf_SIZE)) > 0)	//perimenei message apo to pool
						{
							
							write(writefd, messageFromPool, buf_SIZE);
							while(1)
							{
								if(read(readfd, messageFromConsole, buf_SIZE) > 0)
								{
									if(strcmp(messageFromConsole, "OK") == 0)
										break;
								}
							}
							write(writefd, buf_PRINTEND, 9);
							memset(messageFromPool, 0, buf_SIZE);
							memset(messageFromConsole, 0, buf_SIZE);
							break;
						}
					}
					memset(buf, 0, buf_SIZE);
					memset(copyBuf, 0, buf_SIZE);
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

					printf("(coord) created pool%d FIFOs\n", poolCounter);

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
								//printf("bytesRead: %d, writefd: %d\n",bytesRead, tempWriteFd_coord );
								break;
							}
						}
						printf("(coord) opened pool%d FIFOs\n", poolCounter);

						if(nextAvailablePos >= size)
						{
							printf("realloc...\n");
							oldSize = size;
							size = 2*size;
							coordStorageArray = realloc(coordStorageArray, size * sizeof(poolInfo));

							/* initilization to the new half of the array */
							for(i=oldSize; i<size; i++)
							{
								coordStorageArray[nextAvailablePos].pool_PID = -1;
								coordStorageArray[nextAvailablePos].pool_NUM = -1;
								coordStorageArray[nextAvailablePos].pool_STATUS = -1;
								coordStorageArray[nextAvailablePos].in = -1;
								coordStorageArray[nextAvailablePos].out = -1;
								
							}
						}
						coordStorageArray[nextAvailablePos].pool_PID = pid;
						coordStorageArray[nextAvailablePos].pool_NUM = poolCounter;
						coordStorageArray[nextAvailablePos].pool_STATUS = 0;	//0:active
						coordStorageArray[nextAvailablePos].in = tempReadFd_coord;
						coordStorageArray[nextAvailablePos].out = tempWriteFd_coord;
						coordStorageArray[nextAvailablePos].jobInfoArray = malloc(maxJobsInPool * sizeof(jobInfo));
						coordStorageArray[nextAvailablePos].nextAvailablePos = 0;
						
						nextAvailablePos++;

					
						write(tempWriteFd_coord, copyBuf, buf_SIZE);
						while(1)
						{
							if( (bytesRead = read(tempReadFd_coord, messageFromPool, buf_SIZE)) > 0)	//perimenei message apo to pool
							{
								
								write(writefd, messageFromPool, buf_SIZE);	//write ston console to message
								while(1)
								{
									if(read(readfd, messageFromConsole, buf_SIZE) > 0)	//perimenei to OK tou console oti diavastike
									{
										if(strcmp(messageFromConsole, "OK") == 0)
											break;
									}
								}
								write(writefd, buf_PRINTEND, 9);	//write ston console to PRINTEND gia na stamatisei na perimenei kai alla prints
								memset(messageFromPool, 0, buf_SIZE);
								memset(messageFromConsole, 0, buf_SIZE);
								break;
							}
						}
						memset(buf, 0, buf_SIZE);
						memset(copyBuf, 0, buf_SIZE);
						memset(poolName_in, 0, 15);
						memset(poolName_out, 0, 15);
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
						printf("(pool) NEW pool process: %d\n", getpid());
						finishedJobs = 0;
						poolStorageArray = malloc(maxJobsInPool * sizeof(jobInfo));
						for(i=0; i<maxJobsInPool; i++)
						{
							poolStorageArray[i].job_PID = -1;
							poolStorageArray[i].job_NUM = -1;
							poolStorageArray[i].job_STATUS = -1;
							poolStorageArray[i].startTimeInSeconds = -1;
						}
						nextAvailablePos_pool = 0;

						jobID--;




						/* Keep reading and checking */
						while(1)
						{
							if( (bytesRead = read(tempReadFd_pool, poolBuf, buf_SIZE)) > 0)
							{
								//write(tempWriteFd_pool, buf_OK, 3);		//pes ston coord oti diavastike
								printf("%d poolBuf: %s\n", poolCounter, poolBuf);
								strcpy(copyBuf_pool, poolBuf);
								split2 = strtok(copyBuf_pool, " \n");
								/*
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								DUMMY
								*/
								if(strcmp(split2, SUBMIT) == 0)
								{
									jobID++;
									/* Creating argument array */
									next = arguments;
									split2 = strtok(NULL, " \n");
									while(split2)
									{
										*next++ = split2;
										//printf("%s\n",split2);
										split2 = strtok(NULL, " \n");
									}
									*next = NULL;

									printf("(pool) Checking:\n");
									for(next = arguments; *next != 0; next++)
										puts(*next);

									currentTime = time(NULL);
									myTime = *localtime(&currentTime);

									//printf("sleeping for 1 second...\n");
									//sleep(4);


									pid = fork();
									if(pid > 0) 	//pool (father)
									{
										//printf("still in pool process: %d\n", getpid());

										poolStorageArray[nextAvailablePos_pool].job_PID = pid;
										poolStorageArray[nextAvailablePos_pool].job_NUM = jobID;
										poolStorageArray[nextAvailablePos_pool].job_STATUS = 0;  			//status -> 0:active, 1:finished, 2:suspended
										poolStorageArray[nextAvailablePos_pool].startTimeInSeconds = (myTime.tm_hour*3600) + (myTime.tm_min*60) + myTime.tm_sec;	//kratao tin ora pou ksekinise to job se seconds
										nextAvailablePos_pool++;

										memset(messageToCoord, 0, buf_SIZE);
										sprintf(messageToCoord, "JobID: %d, PID: %d", jobID, pid);
										write(tempWriteFd_pool, messageToCoord, buf_SIZE);
									}
									else if(pid == 0)	//job (child)
									{
										/* directory and file creation */
										jobPID = getpid();
										sprintf(dirName, "sdi1000155_%d_%d_%d%02d%02d_%02d%02d%02d", jobID, jobPID, myTime.tm_year+1900, myTime.tm_mon+1, myTime.tm_mday, myTime.tm_hour, myTime.tm_min, myTime.tm_sec);
										mkdir(dirName, PERMS);

										memset(jobPath, 0, 100);
										sprintf(jobPath, "%s/stdout_%d.txt", dirName, jobID);
										printf("(job) job%d Path: %s\n", jobID, jobPath);
										if ( (stdoutToFile = open(jobPath, O_CREAT | O_TRUNC | O_RDWR, PERMS)) == -1)
										{
											perror("creating out file");
											exit(EXIT_FAILURE);
										}

										memset(jobPath, 0, 100);
										sprintf(jobPath, "%s/stderr_%d.txt", dirName, jobID);
										if ( (stderrToFile = open(jobPath, O_CREAT | O_TRUNC | O_RDWR, PERMS)) == -1)
										{
											perror("creating err file");
											exit(EXIT_FAILURE);
										}

										/* redirection */
										dup2(stdoutToFile, 1);
										dup2(stderrToFile, 2);

										close(stdoutToFile);
										close(stderrToFile);

										if ( execvp(arguments[0], arguments) == -1 )
										{
											perror("failure to exec: ");
										}

									}
									else 
									{
										perror("(pool) could not fork");
										exit(EXIT_FAILURE);
									}
								}
								else if(strcmp(split2, STATUS) == 0)
								{
									split2 = strtok(NULL, " \n");
									jobID_pool = atoi(split2);
									posInPoolStorage = (jobID_pool-1) - ( maxJobsInPool* ((jobID_pool-1)/maxJobsInPool)); 

									status_pool(tempReadFd_pool, tempWriteFd_pool, poolStorageArray, jobID_pool, posInPoolStorage);

								}
								else if(strcmp(split2, STATUSALL) == 0)
								{
									split = strtok(NULL, " \n");
									if(split)	
										timeDuration = atoi(split);
									else 
										timeDuration = 500000; //dinoume epitides mia tixaia megali timi (sigoura megaliteri apo 86400)

									statusall_pool(tempReadFd_pool, tempWriteFd_pool, poolStorageArray, nextAvailablePos_pool, timeDuration);
								}
								else if(strcmp(split2, SHOWACTIVE) == 0)
								{
									showactive_pool(tempReadFd_pool, tempWriteFd_pool, poolStorageArray, nextAvailablePos_pool);
								}
								else if(strcmp(split2, SHOWFINISHED) == 0)
								{
									showfinished_pool(tempReadFd_pool, tempWriteFd_pool, poolStorageArray, nextAvailablePos_pool);
								}
								else if(strcmp(split2, SHOWPOOLS) == 0)
								{

								}
								else if(strcmp(split2, SUSPEND) == 0)
								{

								}
								else if(strcmp(split2, RESUME) == 0)
								{

								}
								else if(strcmp(split2, SHUTDOWN) == 0)
								{

								}
	 
							}
							else if(bytesRead == 0)		//eof
							{
								//printf("EOF\n");
								memset(poolBuf, 0, buf_SIZE);
							}
							else if(bytesRead < 0)
							{
								//printf("n: %d\n", bytesRead);
								memset(poolBuf, 0, buf_SIZE);

							}

							/* elegxos katastasis jobs */
							for(i=0; i<nextAvailablePos_pool; i++)
							{
								//printf("(pool -> job) i: %d\n",i );
								if(poolStorageArray[i].job_STATUS == 0)
								{
									retVal = waitpid(poolStorageArray[i].job_PID, &status, WNOHANG | WUNTRACED);
									if(retVal > 0)	//tote auto to paidi-job termatise
									{
										if(WIFEXITED(status))	//terminated normally
										{
											finishedJobs++;
											poolStorageArray[i].job_STATUS = 1;
											printf("(pool) job%d (%d): finished (normally)\n", poolStorageArray[i].job_NUM, poolStorageArray[i].job_PID);
										}
										else if(WIFSIGNALED(status))	//terminated by a signal
										{
											finishedJobs++;
											poolStorageArray[i].job_STATUS = 1;
											printf("(pool) job%d (%d): finished (signal)\n", poolStorageArray[i].job_NUM, poolStorageArray[i].job_PID);
										}
										else if(WIFSTOPPED(status))	//stopped (can be restarted)
										{
											poolStorageArray[i].job_STATUS = 2;
											printf("(pool) job%d (%d): suspended\n", poolStorageArray[i].job_NUM, poolStorageArray[i].job_PID);
										}
										
									}
									else if(retVal == -1)
									{
										perror("(pool) waitpid failed");
										exit(EXIT_FAILURE);
									}
								}
							}
							memset(poolBuf, 0, buf_SIZE);
							memset(copyBuf_pool, 0, buf_SIZE);
							memset(messageToCoord, 0, buf_SIZE);

							if(finishedJobs == maxJobsInPool)
							{
								printf("(pool) pool%d (%d): all of its jobs are finished\n", poolCounter, pid); //den eimai sigouros gia to poolCounter an einai sosto, thelei check!!!!
								sprintf(message, "I");
								strcat(messageToCoord, message);

								for(i=0; i<maxJobsInPool; i++)
								{
									sprintf(message, "_%d_%d_%d_%d", poolStorageArray[i].job_PID, poolStorageArray[i].job_NUM, poolStorageArray[i].job_STATUS, poolStorageArray[i].startTimeInSeconds);
									strcat(messageToCoord, message);
								}
								
								printf("(pool) %s\n", messageToCoord);
								write(tempWriteFd_pool, messageToCoord, buf_SIZE);

								//perimenei ton coord na tou dosei to ok gia na kanei exit
								while(1)
								{
									if(read(tempReadFd_pool, messageFromCoord, buf_SIZE) > 0)
									{
										if(strcmp(messageFromCoord, "OK") == 0)
										{
											printf("(pool) Time to exit.\n");
											break;
										}
										else
											memset(messageFromCoord, 0, buf_SIZE);
									}
								}
								for(i=0; i<nextAvailablePos; i++)
									free(coordStorageArray[i].jobInfoArray);
								free(coordStorageArray);
								
								free(poolStorageArray);
								free(fifo_READ);
								free(fifo_WRITE);
								free(path);
								close(tempReadFd_pool);
								close(tempWriteFd_pool);
								exit(EXIT_SUCCESS);
							}

						}//end_while
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
				split = strtok(NULL, " \n");
				jobID2 = atoi(split);
				poolPos = (jobID2 - 1) / maxJobsInPool;	//position in the coordStorageArray

				status_coord(readfd, writefd, copyBuf, coordStorageArray, jobID2, poolPos);
				memset(buf, 0, buf_SIZE);
				memset(copyBuf, 0, buf_SIZE);
			}
			else if(strcmp(split, STATUSALL) == 0)
			{
				split = strtok(NULL, " \n");
				if(split)	
					timeDuration = atoi(split);
				else 
					timeDuration = 500000; //dinoume epitides mia tixaia megali timi (sigoura megaliteri apo 86400)
			
				statusall_coord(readfd, writefd, copyBuf, coordStorageArray, poolCounter, timeDuration);
			}
			else if(strcmp(split, SHOWACTIVE) == 0)
			{
				showactive_coord(readfd, writefd, copyBuf, coordStorageArray, poolCounter);
			}
			else if(strcmp(split, SHOWPOOLS) == 0)
			{

			}
			else if(strcmp(split, SHOWFINISHED) == 0)
			{
				showfinished_coord(readfd, writefd, copyBuf, coordStorageArray, poolCounter);
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
			//printf("\n");
		}
		else if( bytesRead == 0)
		{
			//printf("End Of File. User in control.\n");
			memset(buf, 0, buf_SIZE);
			//break;
		}
		else if(bytesRead < 0)
		{
			//printf("n: %d\n", bytesRead);
			memset(buf, 0, buf_SIZE);

		}
		
		/* elegxos an exoun grapsei kati ta pools */
		for(i=0; i<nextAvailablePos; i++)
		{
			if(coordStorageArray[i].pool_STATUS == 0)	//if pool is active
			{
				if( (bytesRead = read(coordStorageArray[i].in, buf, buf_SIZE)) > 0)
				{
					
					strcpy(copyBuf2, buf);
					split3 = strtok(copyBuf2, "_\n");
					if(strcmp(split3, "I") == 0)	//plirofories termatismou pool
					{
						write(coordStorageArray[i].out, buf_OK, 3);	//dose tin adeia stin pool na termatisei
						printf("(coord B) Termination Info. Reading from pool%d\n", i+1);
						//efoson mpikame edo simainei oti to pool exei xtipisei exit() i tha xtipisei para poli sidoma, ara...
						while(1)
						{
							if(waitpid(coordStorageArray[i].pool_PID, &status, WNOHANG) > 0)
							{
								coordStorageArray[i].pool_STATUS = 1; 	//0:active, 1:finished
								printf("(coord B) pool%d (%d) has finished\n", coordStorageArray[i].pool_NUM, coordStorageArray[i].pool_PID);
									
								close(coordStorageArray[i].in);
								close(coordStorageArray[i].out);
								break;
							}
						}

						split3 = strtok(NULL, "_\n");
						j = 0;
						while(split3) //i while(j < maxJobsInPool)
						{ 
							coordStorageArray[i].jobInfoArray[j].job_PID = atoi(split3);
							split3 = strtok(NULL, "_\n");
							coordStorageArray[i].jobInfoArray[j].job_NUM = atoi(split3);
							split3 = strtok(NULL, "_\n");
							coordStorageArray[i].jobInfoArray[j].job_STATUS = atoi(split3); //prepei na einai 1:finished
							split3 = strtok(NULL, "_\n");
							coordStorageArray[i].jobInfoArray[j].startTimeInSeconds = atoi(split3);

							coordStorageArray[i].nextAvailablePos++;
							split3 = strtok(NULL, "_\n");
							j++;
						}
						//coordStorageArray[i].pool_STATUS = 1; 
						memset(copyBuf2, 0, buf_SIZE);
						/*
						if(poolCounter == 3)
						{
							for(i=0; i<3; i++)
							{
								printf("coordStorageArray[%d]\n", i);
								printf("	pool_PID: %d\n", coordStorageArray[i].pool_PID);
								printf("	pool_NUM: %d\n", coordStorageArray[i].pool_NUM);
								printf("	pool_STATUS: %d\n", coordStorageArray[i].pool_STATUS);
								printf("	nextAvailablePos: %d\n", coordStorageArray[i].nextAvailablePos);
								for(j=0; j<coordStorageArray[i].nextAvailablePos; j++ )
								{
									printf("*************************************\n");
									printf("		job_PID: %d\n", coordStorageArray[i].jobInfoArray[j].job_PID);
									printf("		job_NUM: %d\n", coordStorageArray[i].jobInfoArray[j].job_NUM);
									printf("		job_STATUS: %d\n", coordStorageArray[i].jobInfoArray[j].job_STATUS);
									printf("		startTimeInSeconds: %d\n", coordStorageArray[i].jobInfoArray[j].startTimeInSeconds);
								}
							}
						}*/
					}
					else 	//aplo minima ektiposis - denprepei kan na einai edo. gia error check mono!
					{
						printf("(coord) Just a message: %s\n", buf);
						write(writefd, buf, buf_SIZE);
					}
					
					memset(buf, 0, buf_SIZE);

				}
			}
		}
	}
	printf("sleeping...\n");
	sleep(20);


	close(readfd);
	close(writefd);
	free(fifo_READ);
	free(fifo_WRITE);
	free(path);
	exit(0);
}