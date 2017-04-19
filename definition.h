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
#define PERMS 0666

typedef struct coordToPool
{
	int poolPid;
	int poolNum;
	int in;	//read
	int out; //write
}coordToPool;

#endif