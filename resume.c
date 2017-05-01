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
#include "resume.h"

void resume_coord(int readfd, int writefd, char *buffer, poolInfo *coordStorageArray, int nextAvailablePos, int jobID, int poolPos)
{
	int status, i, j;
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

			/* elegxos an exoun grapsei kati ta pools */
			for(i=0; i<nextAvailablePos; i++)
			{
				if(i != poolPos)	//diavazei apo ola ta pools ektos apo to 1 pou perimenoume pio pano.
				{
					if(coordStorageArray[i].pool_STATUS == 0)	//if pool is active
					{
						if( (read(coordStorageArray[i].in, messageFromPool, buf_SIZE)) > 0)
						{
							split = strtok(messageFromPool, "_\n");
							if(strcmp(split, "I") == 0)	//plirofories termatismou pool
							{
								write(coordStorageArray[i].out, buf_OK, 3);	//dose tin adeia stin pool na termatisei
								printf("(coord B: resume) Termination Info. Reading from pool%d\n", i+1);

								//efoson mpikame edo simainei oti to pool exei xtipisei exit() i tha xtipisei para poli sidoma, ara...
								while(1)
								{
									if(waitpid(coordStorageArray[i].pool_PID, &status, WNOHANG) > 0)
									{
										coordStorageArray[i].pool_STATUS = 1; 	//0:active, 1:finished
										printf("(coord B: resume) pool%d (%d) has finished\n", coordStorageArray[i].pool_NUM, coordStorageArray[i].pool_PID);
											
										close(coordStorageArray[i].in);
										close(coordStorageArray[i].out);
										break;
									}
								}

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
	else if(coordStorageArray[poolPos].pool_STATUS == 1)
	{
		sprintf(messageToConsole, "resume failed: Job %d is finished (Pool %d is finished)", jobID, poolPos+1);
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
		write(writefd, buf_PRINTEND, 9);
	}
}

int resume_pool(int readfd_pool, int writefd_pool, jobInfo *poolStorageArray, int jobID_pool, int posInPoolStorage)
{
	int status;
	char messageFromCoord[buf_SIZE], messageToCoord[buf_SIZE];
	char buf_DONE[] = "DONE";
	time_t currentTime;
	struct tm myTime;

	memset(messageToCoord, 0, buf_SIZE);
	memset(messageFromCoord, 0, buf_SIZE);

	if(poolStorageArray[posInPoolStorage].job_STATUS == 2)
	{
		kill(poolStorageArray[posInPoolStorage].job_PID, SIGCONT);
		while(1)
		{
			if( waitpid(poolStorageArray[posInPoolStorage].job_PID, &status, WNOHANG | WUNTRACED | WCONTINUED) > 0)
			{
				if( WIFCONTINUED(status) )
				{
					poolStorageArray[posInPoolStorage].job_STATUS = 0;
					printf("(pool) job%d (%d): resumed\n", poolStorageArray[posInPoolStorage].job_NUM, poolStorageArray[posInPoolStorage].job_PID);
					break;
				}
				else if( WIFEXITED(status) )	//terminated normally
				{
					//finishedJobs++;
					poolStorageArray[posInPoolStorage].job_STATUS = 1;
					printf("(pool) job%d (%d): finished (normally) in resume\n", poolStorageArray[posInPoolStorage].job_NUM, poolStorageArray[posInPoolStorage].job_PID);
					sprintf(messageToCoord, "resume failed: job%d (%d) is finished", poolStorageArray[posInPoolStorage].job_NUM, poolStorageArray[posInPoolStorage].job_PID);
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
					return 1;
				}
			}
		}

		currentTime = time(NULL);
		myTime = *localtime(&currentTime);
		poolStorageArray[posInPoolStorage].cont = (myTime.tm_hour*3600) + (myTime.tm_min*60) + myTime.tm_sec; //i ora pou egine cont

		sprintf(messageToCoord, "Sent resume signal to JobID  %d", poolStorageArray[posInPoolStorage].job_NUM);
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
		return 0;
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
		return 0;
	}
	else if(poolStorageArray[posInPoolStorage].job_STATUS == 0)
	{
		sprintf(messageToCoord, "JobID  %d is already active", poolStorageArray[posInPoolStorage].job_NUM);
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
		return 0;
	}

}