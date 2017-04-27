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
#include "showfinished.h"

void showfinished_coord(int readfd, int writefd, char *buffer, poolInfo *coordStorageArray, int numOfPools)
{
	int status, i, j, k, l;
	char messageFromConsole[buf_SIZE], messageToConsole[buf_SIZE], messageFromPool[buf_SIZE];
	char buf_PRINTEND[] = "PRINTEND", buf_OK[] = "OK";
	char *split;

	memset(messageFromConsole, 0, buf_SIZE);
	memset(messageToConsole, 0, buf_SIZE);
	memset(messageFromPool, 0, buf_SIZE);

	for(i=0; i<numOfPools; i++)	//gia kathe ena apo ta iparxonta pools
	{
		//prota apo ola koita an exei idi termatisei to pool sto opoio anikei to job
		if(coordStorageArray[i].pool_STATUS == 1)
		{
			for(j=0; j<coordStorageArray[i].nextAvailablePos; j++) 	//tote gia kathe job pou iparxei mesa sto pool
			{
				sprintf(messageToConsole, "JobID %d Status: Finished", coordStorageArray[i].jobInfoArray[j].job_NUM);
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
				memset(messageToConsole, 0, buf_SIZE);
			}
		}
		else
		{
			write(coordStorageArray[i].out, buffer, buf_SIZE);	//write sto katallilo pool
			while(1)
			{
				if(read(coordStorageArray[i].in, messageFromPool, buf_SIZE) > 0)
				{
					printf("(coord) diavase apo pool minima: %s\n", messageFromPool);
					if(strchr(messageFromPool, '_'))
					{
						write(coordStorageArray[i].out, buf_OK, 3);	//dose tin adeia stin pool na termatisei
						printf("(coord A) Termination Info. Reading from pool%d\n", i+1);
						//efoson mpikame edo simainei oti to pool exei xtipisei exit() i tha xtipisei para poli sidoma, ara...
						while(1)
						{
							if(waitpid(coordStorageArray[i].pool_PID, &status, WNOHANG) > 0)
							{
								coordStorageArray[i].pool_STATUS = 1; 	//0:active, 1:finished
								printf("(coord A) pool%d (%d) has finished\n", coordStorageArray[i].pool_NUM, coordStorageArray[i].pool_PID);
									
								close(coordStorageArray[i].in);
								close(coordStorageArray[i].out);
								break;
							}
						}
						split = strtok(messageFromPool, "_\n");
						split = strtok(NULL, "_\n");
						k = 0;
						while(split) //i while(k < maxJobsInPool)
						{ 
							coordStorageArray[i].jobInfoArray[k].job_PID = atoi(split);
							split = strtok(NULL, "_\n");
							coordStorageArray[i].jobInfoArray[k].job_NUM = atoi(split);
							split = strtok(NULL, "_\n");
							coordStorageArray[i].jobInfoArray[k].job_STATUS = atoi(split); //prepei na einai 1:finished
							split = strtok(NULL, "_\n");
							coordStorageArray[i].jobInfoArray[k].startTimeInSeconds = atoi(split);
							coordStorageArray[i].nextAvailablePos++;
							split = strtok(NULL, "_\n");
							k++;
						}

						for(l=0; l<coordStorageArray[i].nextAvailablePos; l++)
						{
							sprintf(messageToConsole, "JobID %d Status: Finished", coordStorageArray[i].jobInfoArray[l].job_NUM);
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
						}
						memset(messageFromPool, 0, buf_SIZE);
						break; //pame sto epomeno i/pool

					}
					else 	//kanoniko minima gia ektiposi
					{
						if(strcmp(messageFromPool, "DONE") != 0)
						{
							write(coordStorageArray[i].out, buf_OK, 3);
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
							memset(messageFromPool, 0, buf_SIZE);
							break;
						}
					}
				}
				memset(messageFromPool, 0, buf_SIZE);
			}
		}
	}
	write(writefd, buf_PRINTEND, 9);	//write ston console to PRINTEND gia na stamatisei na perimenei kai alla prints

}

void showfinished_pool(int readfd_pool, int writefd_pool, jobInfo *poolStorageArray, int nextAvailablePos_pool)
{
	int i;
	char messageFromCoord[buf_SIZE], messageToCoord[buf_SIZE];
	char buf_DONE[] = "DONE";

	memset(messageToCoord, 0, buf_SIZE);
	memset(messageFromCoord, 0, buf_SIZE);

	for(i=0; i<nextAvailablePos_pool; i++)
	{
		if(poolStorageArray[i].job_STATUS == 1)
		{
			sprintf(messageToCoord, "JobID %d Status: Finished", poolStorageArray[i].job_NUM);
			write(writefd_pool, messageToCoord, buf_SIZE);
			while(1)
			{	
				if(read(readfd_pool, messageFromCoord, buf_SIZE) > 0)
				{
					printf("messageFromCoord (showfinished): %s\n", messageFromCoord);
					if(strcmp(messageFromCoord, "OK") == 0)
					{
						memset(messageFromCoord, 0, buf_SIZE);
						break;
					}
					else
					{
						printf("ERROR: 2\n");
						//memset(messageFromCoord, 0, buf_SIZE);
					}
				}
			}
		}
		memset(messageToCoord, 0, buf_SIZE);
	}
	write(writefd_pool, buf_DONE, 5);
}