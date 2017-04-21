#ifndef DEFINITION__H__
#define DEFINITION__H__

#define SUBMIT "submit"
#define STATUS "status"
#define STATUSALL "status-all"
#define SHOWACTIVE "show-active"
#define SHOWPOOLS "show-pools"
#define SHOWFINISHED "show-finished"
#define SUSPEND "suspend"
#define RESUME "resume"
#define SHUTDOWN "shutdown"

#define fileBuf_SIZE 1024
#define buf_SIZE 1024
#define PERMS 0755

typedef struct coordToPool
{
	int pool_PID;
	int pool_NUM;
	int in;	//read
	int out; //write
}coordToPool;

typedef struct jobInfo
{
	int job_PID;
	int job_NUM;
	int job_STATUS; //0:active, 1:finished, 3:suspended
	int startTimeInSeconds;
}jobInfo;

#endif