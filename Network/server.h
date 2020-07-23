#ifndef SERVER
#define SERVER

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>


#define MAXSIZE 20
#define BUFFSIZE 255



pthread_rwlock_t lock=PTHREAD_RWLOCK_INITIALIZER;
typedef struct users{
	int fd;
	uint16_t length;
	unsigned char usernamestr [MAXSIZE];
} user;

typedef struct link{
	user* user;
	struct link* next;
} userLink;


void * handShake(void*  u);
int IsUserNameUnique(unsigned char *name ,int  size);
void sendCurrentUserNames(user * targetUser);
void getUsername(user* currentUser);
void recievedBytes(user * currentUser, unsigned char* buff, uint16_t numBytes);
void removeUser(user* u);
void addUser(user* u);
void chat(user* u);
void sendChatMessage(char* p,  uint16_t size, user * sender);
void sendJoinMessage(char* p, uint16_t size);
void sendExitMessage(char* p, uint16_t size);
void sendBytes(user * currentUser, unsigned char* buff, uint16_t numBytes);
void sendJoin(user * currentUser);
void sendExit(user * currentUser);
void writeToLogJoin(user *u);
void writeToLogExit(user *u);
void exitAll();

struct sigaction priorSigHandler;
struct sigaction currentSigHandler;

uint16_t numberofclients;
int fd;
int sock;
pid_t sid;
userLink* firstLink;
FILE *fp; 

#endif