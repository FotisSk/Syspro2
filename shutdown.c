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

void shutdown_coord(int readfd, int writefd, poolInfo *coordStorageArray, int poolCounter)
{
	int i, j, poolsFinished, status;
	char messageFromPool[buf_SIZE], messageToConsole[buf_SIZE], messageFromConsole[buf_SIZE];
	char buf_OK[] = "OK", buf_PRINTEND[] = "PRINTEND";
	char *split;

	memset(messageFromPool, 0, buf_SIZE);
	memset(messageToConsole, 0, buf_SIZE);
	memset(messageFromConsole, 0, buf_SIZE);

	poolsFinished = 0;

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
					printf("(coord) diavase apo pool minima: %s\n", messageFromPool);
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
								printf("pools finished: %d\n", poolsFinished);
								printf("(coord shut) pool%d (%d) has finished\n", coordStorageArray[i].pool_NUM, coordStorageArray[i].pool_PID);
								close(coordStorageArray[i].in);
								close(coordStorageArray[i].out);
								break;
							}
						}
						split = strtok(messageFromPool, "_\n");
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
	sprintf(messageToConsole, "Served jobs, were still in progress");
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
