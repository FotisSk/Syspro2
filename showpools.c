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
#include "showpools.h"

void showpools_coord(int readfd, int writefd, char *buffer, poolInfo *coordStorageArray, int nextAvailablePos, int numOfPools)
{
	int status, i, j, k, l;
	char messageFromConsole[buf_SIZE], messageToConsole[buf_SIZE], messageFromPool[buf_SIZE];
	char buf_PRINTEND[] = "PRINTEND", buf_OK[] = "OK";
	char *split;

	memset(messageFromConsole, 0, buf_SIZE);
	memset(messageToConsole, 0, buf_SIZE);
	memset(messageFromPool, 0, buf_SIZE);

	sprintf(messageToConsole, "Pool & NumOfJobs:");
	write(writefd, messageToConsole, buf_SIZE);
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
	memset(messageToConsole, 0, buf_SIZE);

	for(i=0; i<numOfPools; i++)	//gia kathe ena apo ta iparxonta pools
	{
		if(coordStorageArray[i].pool_STATUS == 0) 	//an to pool einai active
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
						break;
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
										break;
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

				/* elegxos an exoun grapsei kati ta pools */
				for(k=0; k<nextAvailablePos; k++)
				{
					if(k != i)	//diavazei apo ola ta pools ektos apo to 1 pou perimenoume pio pano.
					{
						if(coordStorageArray[k].pool_STATUS == 0)	//if pool is active
						{
							if( (read(coordStorageArray[k].in, messageFromPool, buf_SIZE)) > 0)
							{
								split = strtok(messageFromPool, "_\n");
								if(strcmp(split, "I") == 0)	//plirofories termatismou pool
								{
									write(coordStorageArray[k].out, buf_OK, 3);	//dose tin adeia stin pool na termatisei
									printf("(coord B: showpools) Termination Info. Reading from pool%d\n", k+1);

									//efoson mpikame edo simainei oti to pool exei xtipisei exit() i tha xtipisei para poli sidoma, ara...
									while(1)
									{
										if(waitpid(coordStorageArray[k].pool_PID, &status, WNOHANG) > 0)
										{
											coordStorageArray[k].pool_STATUS = 1; 	//0:active, 1:finished
											printf("(coord B: showpools) pool%d (%d) has finished\n", coordStorageArray[k].pool_NUM, coordStorageArray[k].pool_PID);
												
											close(coordStorageArray[k].in);
											close(coordStorageArray[k].out);
											break;
										}
									}

									split = strtok(NULL, "_\n");
									l = 0;
									while(split) //i while(j < maxJobsInPool)
									{ 
										coordStorageArray[k].jobInfoArray[l].job_PID = atoi(split);
										split = strtok(NULL, "_\n");
										coordStorageArray[k].jobInfoArray[l].job_NUM = atoi(split);
										split = strtok(NULL, "_\n");
										coordStorageArray[k].jobInfoArray[l].job_STATUS = atoi(split); //prepei na einai 1:finished
										split = strtok(NULL, "_\n");
										coordStorageArray[k].jobInfoArray[l].startTimeInSeconds = atoi(split);

										coordStorageArray[k].nextAvailablePos++;

										split = strtok(NULL, "_\n");
										l++;
									}
									
								}
								else 	//mono gia error check!! den prepei na mpainei edo
								{
									printf("(coord) Just a message: %s\n", messageFromPool);
									write(writefd, messageFromPool, buf_SIZE);
								}
								memset(messageFromPool, 0, buf_SIZE);

							}
						}
					}
				}
				memset(messageFromPool, 0, buf_SIZE);	
			}
		}
	}
	write(writefd, buf_PRINTEND, 9);	//write ston console to PRINTEND gia na stamatisei na perimenei kai alla prints
}

void showpools_pool(int readfd_pool, int writefd_pool, jobInfo *poolStorageArray, int nextAvailablePos_pool)
{
	int i, jobsBeingExecuted, pool_PID;
	char messageFromCoord[buf_SIZE], messageToCoord[buf_SIZE];
	char buf_DONE[] = "DONE";

	memset(messageToCoord, 0, buf_SIZE);
	memset(messageFromCoord, 0, buf_SIZE);

	pool_PID = getpid();

	jobsBeingExecuted = 0;
	printf("nextAvailablePos_pool: %d\n", nextAvailablePos_pool);
	for(i=0; i<nextAvailablePos_pool; i++)
	{
		if( (poolStorageArray[i].job_STATUS == 0) || (poolStorageArray[i].job_STATUS == 2) )	//0:active, 1:finished, 2:suspended (theoro os 'ipo ektelesi' tis active kai suspended)
		{
			printf("poolStorageArray[%d].job_STATUS: %d\n", i, poolStorageArray[i].job_STATUS);
			jobsBeingExecuted++;
		}
	}
	sprintf(messageToCoord, "%d %d", pool_PID, jobsBeingExecuted);
	write(writefd_pool, messageToCoord, buf_SIZE);
	while(1)
	{
		if(read(readfd_pool, messageFromCoord, buf_SIZE) > 0)
		{
			printf("messageFromCoord (showpools): %s\n", messageFromCoord);
			if(strcmp(messageFromCoord, "OK") == 0)
			{
				memset(messageFromCoord, 0, buf_SIZE);
				break;
			}
			else
			{
				printf("ERROR: 1\n");
				//memset(messageFromCoord, 0, buf_SIZE);
			}
		}
	}
	write(writefd_pool, buf_DONE, 5);
}