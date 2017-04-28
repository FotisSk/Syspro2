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
#include "suspend.h"

void suspend_coord(int readfd, int writefd, char *buffer, poolInfo *coordStorageArray, int jobID, int poolPos)
{
	int status, j;
	char messageFromConsole[buf_SIZE], messageToConsole[buf_SIZE], messageFromPool[buf_SIZE];
	char buf_PRINTEND[] = "PRINTEND", buf_OK[] = "OK";
	char *split;

	memset(messageFromConsole, 0, buf_SIZE);
	memset(messageToConsole, 0, buf_SIZE);
	memset(messageFromPool, 0, buf_SIZE);

	//prota apo ola koita an exei idi termatisei to pool sto opoio anikei to job
	if(coordStorageArray[poolPos].pool_STATUS == 0)
	{
		write(coordStorageArray[poolPos].out, buffer, buf_SIZE);	//write sto katallilo pool
		while(1)
		{
			if(read(coordStorageArray[poolPos].in, messageFromPool, buf_SIZE) > 0)	//an diavaseis minima apo to pool
			{
				printf("(coord) diavase apo pool minima: %s\n", messageFromPool);
				if(strchr(messageFromPool, '_')) 	//tote einai minima termatismou
				{
					write(coordStorageArray[poolPos].out, buf_OK, 3);	//dose tin adeia stin pool na termatisei
					printf("(coord A) Termination Info. Reading from pool%d\n", poolPos+1);
					//efoson mpikame edo simainei oti to pool exei xtipisei exit() i tha xtipisei para poli sidoma, ara...
					while(1)
					{
						if(waitpid(coordStorageArray[poolPos].pool_PID, &status, WNOHANG) > 0)
						{
							coordStorageArray[poolPos].pool_STATUS = 1; 	//0:active, 1:finished
							printf("(coord A) pool%d (%d) has finished\n", coordStorageArray[poolPos].pool_NUM, coordStorageArray[poolPos].pool_PID);
								
							close(coordStorageArray[poolPos].in);
							close(coordStorageArray[poolPos].out);
							break;
						}
					}
					split = strtok(messageFromPool, "_\n");
					split = strtok(NULL, "_\n");
					j = 0;
					while(split) //i while(j < maxJobsInPool)
					{ 
						coordStorageArray[poolPos].jobInfoArray[j].job_PID = atoi(split);
						split = strtok(NULL, "_\n");
						coordStorageArray[poolPos].jobInfoArray[j].job_NUM = atoi(split);
						split = strtok(NULL, "_\n");
						coordStorageArray[poolPos].jobInfoArray[j].job_STATUS = atoi(split); //prepei na einai 1:finished
						split = strtok(NULL, "_\n");
						coordStorageArray[poolPos].jobInfoArray[j].startTimeInSeconds = atoi(split);
						coordStorageArray[poolPos].nextAvailablePos++;
						split = strtok(NULL, "_\n");
						j++;
					}
					sprintf(messageToConsole, "Job %d is finished", jobID);
					write(writefd, messageToConsole, buf_SIZE);
					while(1)
					{
						if(read(readfd, messageFromConsole, buf_SIZE) > 0)	//perimenei to OK tou console oti diavastike
						{
							if(strcmp(messageFromConsole, "OK") == 0)
							{
								memset(messageFromConsole, 0, buf_SIZE);
								break;
							}
						}
					}
					write(writefd, buf_PRINTEND, 9);
					return;
				}
				else //aplo minima ektiposis
				{
					if(strcmp(messageFromPool, "DONE") != 0)
					{
						write(coordStorageArray[poolPos].out, buf_OK, 3);
						write(writefd, messageFromPool, buf_SIZE);
						while(1)
						{
							if(read(readfd, messageFromConsole, buf_SIZE) > 0)
							{
								if(strcmp(messageFromConsole, "OK") == 0)
								{
									memset(messageFromConsole, 0, buf_SIZE);
									break;
								}
							}
						}
				    }
					else
					{
						write(writefd, buf_PRINTEND, 9);
						return;
					}	
				}
			}
			memset(messageFromPool, 0, buf_SIZE);
		}
		
	}
}

void suspend_pool(int readfd_pool, int writefd_pool, jobInfo *poolStorageArray, int jobID_pool, int posInPoolStorage)
{
	char messageFromCoord[buf_SIZE], messageToCoord[buf_SIZE];
	char buf_DONE[] = "DONE";
	time_t currentTime;
	struct tm myTime;

	memset(messageToCoord, 0, buf_SIZE);
	memset(messageFromCoord, 0, buf_SIZE);

	if(poolStorageArray[posInPoolStorage].job_STATUS == 0)
	{
		kill(poolStorageArray[posInPoolStorage].job_PID, SIGSTOP);
		sleep(1);
		currentTime = time(NULL);
		myTime = *localtime(&currentTime);
		sprintf(messageToCoord, "Sent suspend signal to JobID  %d", poolStorageArray[posInPoolStorage].job_NUM);
		write(writefd_pool, messageToCoord, buf_SIZE);
		while(1)
		{
			if(read(readfd_pool, messageFromCoord, buf_SIZE) > 0)
			{
				printf("messageFromCoord (status): %s\n", messageFromCoord);
				if(strcmp(messageFromCoord, "OK") == 0)
					break;
				else
				{
					printf("ERROR: 1\n");
					//memset(messageFromCoord, 0, buf_SIZE);
				}
			}
		}
		write(writefd_pool, buf_DONE, 5);
		return;
	}
	else if(poolStorageArray[posInPoolStorage].job_STATUS == 1)
	{
		sprintf(messageToCoord, "JobID  %d is finished", poolStorageArray[posInPoolStorage].job_NUM);
		write(writefd_pool, messageToCoord, buf_SIZE);
		while(1)
		{
			if(read(readfd_pool, messageFromCoord, buf_SIZE) > 0)
			{
				printf("messageFromCoord (status): %s\n", messageFromCoord);
				if(strcmp(messageFromCoord, "OK") == 0)
					break;
				else
				{
					printf("ERROR: 1\n");
					//memset(messageFromCoord, 0, buf_SIZE);
				}
			}
		}
		write(writefd_pool, buf_DONE, 5);
		return;
	}
	else if(poolStorageArray[posInPoolStorage].job_STATUS == 2)
	{
		sprintf(messageToCoord, "JobID  %d is already suspended", poolStorageArray[posInPoolStorage].job_NUM);
		write(writefd_pool, messageToCoord, buf_SIZE);
		while(1)
		{
			if(read(readfd_pool, messageFromCoord, buf_SIZE) > 0)
			{
				printf("messageFromCoord (status): %s\n", messageFromCoord);
				if(strcmp(messageFromCoord, "OK") == 0)
					break;
				else
				{
					printf("ERROR: 1\n");
					//memset(messageFromCoord, 0, buf_SIZE);
				}
			}
		}
		write(writefd_pool, buf_DONE, 5);
		return;
	}

}