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
#include "shutdown.h"

void shutdown_coord(int readfd, int writefd, poolInfo *coordStorageArray, int poolCounter, int jobCounter)
{
	int i, j, poolsFinished, status, inProgress;
	char messageFromPool[buf_SIZE], messageToConsole[buf_SIZE], messageFromConsole[buf_SIZE];
	char buf_OK[] = "OK", buf_PRINTEND[] = "PRINTEND";
	char *split;

	memset(messageFromPool, 0, buf_SIZE);
	memset(messageToConsole, 0, buf_SIZE);
	memset(messageFromConsole, 0, buf_SIZE);

	poolsFinished = 0;
	inProgress = 0;

	for(i=0; i<poolCounter; i++)
	{
		if(coordStorageArray[i].pool_STATUS == 0) //an to pool einai energo
		{
			if(kill(coordStorageArray[i].pool_PID, SIGTERM) == -1)
				perror("SIGTERM for pool");
		}
		else
			poolsFinished++;	
	}


	while(poolsFinished != poolCounter)
	{
		for(i=0; i<poolCounter; i++)
		{
			if(coordStorageArray[i].pool_STATUS == 0)
			{
				if(read(coordStorageArray[i].in, messageFromPool, buf_SIZE) > 0)
				{
					printf("(coord shut) diavase apo pool minima: %s\n", messageFromPool);
					if(strchr(messageFromPool, '_')) 	//tote einai minima termatismou
					{
						write(coordStorageArray[i].out, buf_OK, 3);	//dose tin adeia stin pool na termatisei
						printf("(coord shut) Termination Info. Reading from pool%d\n", i+1);
						//efoson mpikame edo simainei oti to pool exei xtipisei exit() i tha xtipisei para poli sidoma, ara...
						while(1)
						{
							if(waitpid(coordStorageArray[i].pool_PID, &status, WNOHANG) > 0)
							{
								coordStorageArray[i].pool_STATUS = 1; 	//0:active, 1:finished
								poolsFinished++;
								printf("(coord shut) pool%d (%d) has finished\n", coordStorageArray[i].pool_NUM, coordStorageArray[i].pool_PID);
								printf("(coord shut) pools finished: %d\n", poolsFinished);
								close(coordStorageArray[i].in);
								close(coordStorageArray[i].out);
								break;
							}
						}
						split = strtok(messageFromPool, "_\n");
						split = strtok(NULL, "_\n");
						inProgress = atoi(split);
						split = strtok(NULL, "_\n");
						j = 0;
						while(split) //i while(j < maxJobsInPool)
						{ 
							coordStorageArray[i].jobInfoArray[j].job_PID = atoi(split);
							split = strtok(NULL, "_\n");
							coordStorageArray[i].jobInfoArray[j].job_NUM = atoi(split);
							split = strtok(NULL, "_\n");
							coordStorageArray[i].jobInfoArray[j].job_STATUS = atoi(split); //prepei na einai 1:finished
							split = strtok(NULL, "_\n");
							coordStorageArray[i].jobInfoArray[j].startTimeInSeconds = atoi(split);
							coordStorageArray[i].nextAvailablePos++;
							split = strtok(NULL, "_\n");
							j++;
						}
						memset(messageFromPool, 0, buf_SIZE);
					}
				}
				
			}
		}
	}
	sprintf(messageToConsole, "Served %d jobs, %d were still in progress", jobCounter, inProgress);
	write(writefd, messageToConsole, buf_SIZE);
	while(1)
	{
		if(read(readfd, messageFromConsole, buf_SIZE) > 0)	//perimenei to OK tou console oti diavastike
		{
			if(strcmp(messageFromConsole, "OK") == 0)
				break;
			
		}
	}
	write(writefd, buf_PRINTEND, 9);
}


void shutdown_pool(int readfd_pool, int writefd_pool, jobInfo *poolStorageArray, int nextAvailablePos_pool, int poolCounter, int finishedJobs, int maxJobsInPool)
{
	int i, status, pid, inProgress;
	char message[buf_SIZE], messageToCoord[buf_SIZE], messageFromCoord[buf_SIZE];

	memset(message, 0, buf_SIZE);
	memset(messageToCoord, 0, buf_SIZE);
	memset(messageFromCoord, 0, buf_SIZE);

	inProgress = 0;
	pid = getpid();

	for(i=0; i<nextAvailablePos_pool; i++)
	{
		if(poolStorageArray[i].job_STATUS == 0) 	//an einai active
		{
			inProgress++;
			if(kill(poolStorageArray[i].job_PID, SIGTERM) == -1)
			{
				perror("SIGTERM for job");
			}
			while(1)	//sigoureuomaste oti pige to SIGTERM sto process, pianoume kai tin periptosi na oloklirothike fisiologika ligo prin ftasei to SIGTERM
			{
				if(waitpid(poolStorageArray[i].job_PID, &status, WNOHANG | WCONTINUED) > 0)
				{
					if( WIFSIGNALED(status) )	//terminated by a signal
					{
						finishedJobs++;
						poolStorageArray[i].job_STATUS = 1;
						printf("(pool%d) job%d (%d): terminated (signal) in shutdown\n", poolCounter, poolStorageArray[i].job_NUM, poolStorageArray[i].job_PID);
					}
					else if( WIFEXITED(status) )	//terminated normally
					{
						finishedJobs++;
						poolStorageArray[i].job_STATUS = 1;
						printf("(pool%d) job%d (%d): finished (normally) in shutdown\n", poolCounter, poolStorageArray[i].job_NUM, poolStorageArray[i].job_PID);
					}
				}
			}

		}
		else if(poolStorageArray[i].job_STATUS == 2) //an einai suspended
		{
			inProgress++;
			if(kill(poolStorageArray[i].job_PID, SIGCONT) == -1)	//prota tin kano resume
			{
				perror("SIGCONT for job");
			}
			while(1)	//sigoureuomaste oti pige to SIGCONT sto process, pianoume kai tin periptosi na pige to resume kai na termatise oriaka
			{
				if(waitpid(poolStorageArray[i].job_PID, &status, WNOHANG | WCONTINUED) > 0)
				{
					if( WIFCONTINUED(status) )
					{
						poolStorageArray[i].job_STATUS = 0;
						printf("(pool%d shutdown) job%d (%d): resumed\n", poolCounter, poolStorageArray[i].job_NUM, poolStorageArray[i].job_PID);
						
						if(kill(poolStorageArray[i].job_PID, SIGTERM) == -1)	//kai meta tou dinoume SIGTERM sima
						{
							perror("SIGTERM for job");
						}
						while(1)	//sigoureuomaste oti pige to SIGTERM sto process, pianoume kai tin periptosi na oloklirothike fisiologika ligo prin ftasei to SIGTERM
						{
							if(waitpid(poolStorageArray[i].job_PID, &status, WNOHANG | WUNTRACED) > 0)
							{
								if( WIFSIGNALED(status) )	//terminated by a signal
								{
									finishedJobs++;
									poolStorageArray[i].job_STATUS = 1;
									printf("(pool%d) job%d (%d): terminated (signal) in shutdown\n", poolCounter, poolStorageArray[i].job_NUM, poolStorageArray[i].job_PID);
									break;
								}
								else if( WIFEXITED(status) )	//terminated normally
								{
									finishedJobs++;
									poolStorageArray[i].job_STATUS = 1;
									printf("(pool%d) job%d (%d): finished (normally) in shutdown\n", poolCounter, poolStorageArray[i].job_NUM, poolStorageArray[i].job_PID);
									break;
								}
							}
						}
						break;
					}
					else if( WIFEXITED(status) )	//terminated normally
					{
						finishedJobs++;
						poolStorageArray[i].job_STATUS = 1;
						printf("(pool%d shutdown) job%d (%d): finished (normally) in shutdown\n", poolCounter, poolStorageArray[i].job_NUM, poolStorageArray[i].job_PID);
						break;
					}
				}
			}
		}
	}//end for (tora exoume steilei kill se ola ta jobs kai exoume tis katastaseis olon ton jobs ananeomenes)

	if(finishedJobs == maxJobsInPool)
	{
		printf("(pool%d) %d: all of its in-progress-jobs were forced to terminate\n", poolCounter, pid); //den eimai sigouros gia to poolCounter an einai sosto, thelei check!!!!
		sprintf(message, "I_%d", inProgress);
		strcat(messageToCoord, message);

		for(i=0; i<maxJobsInPool; i++)
		{
			sprintf(message, "_%d_%d_%d_%d", poolStorageArray[i].job_PID, poolStorageArray[i].job_NUM, poolStorageArray[i].job_STATUS, poolStorageArray[i].startTimeInSeconds);
			strcat(messageToCoord, message);
		}
								
		printf("(pool%d) %s\n", poolCounter, messageToCoord);
		write(writefd_pool, messageToCoord, buf_SIZE);

		//perimenei ton coord na tou dosei to ok gia na kanei exit
		while(1)
		{
			if(read(readfd_pool, messageFromCoord, buf_SIZE) > 0)
			{
				if(strcmp(messageFromCoord, "OK") == 0)
				{
					printf("(pool%d) Time to exit.\n", poolCounter);
					break;
				}
				else
					memset(messageFromCoord, 0, buf_SIZE);
			}
		}						
	}
	else if(finishedJobs < maxJobsInPool)
	{
		printf("(pool%d) %d: forced shutdown (finishedJobs < maxJobsInPool)\n", poolCounter, pid); //den eimai sigouros gia to poolCounter an einai sosto, thelei check!!!!
		sprintf(message, "I_%d", inProgress);
		strcat(messageToCoord, message);

		for(i=0; i<nextAvailablePos_pool; i++)
		{
			sprintf(message, "_%d_%d_%d_%d", poolStorageArray[i].job_PID, poolStorageArray[i].job_NUM, poolStorageArray[i].job_STATUS, poolStorageArray[i].startTimeInSeconds);
			strcat(messageToCoord, message);
		}
									
		printf("(pool%d) %s\n", poolCounter, messageToCoord);
		write(writefd_pool, messageToCoord, buf_SIZE);
		//perimenei ton coord na tou dosei to ok gia na kanei exit
		while(1)
		{
			if(read(readfd_pool, messageFromCoord, buf_SIZE) > 0)
			{
				if(strcmp(messageFromCoord, "OK") == 0)
				{
					printf("(pool%d) Time to exit.\n", poolCounter);
					break;
				}
				else
					memset(messageFromCoord, 0, buf_SIZE);
			}
		}
	}
}