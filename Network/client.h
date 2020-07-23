#ifndef CLIENT
#define CLIENT

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

#define  MaxUsernameLength 20
#define  MaxUsernames 256
#define  BufferSize 256

typedef struct users{
	int len;
	unsigned char username[MaxUsernameLength];
} user;

typedef struct link{
	user* user;
	struct link* next;
} userLink;


void * recieveHandler(void * unUsed);


void chat();

void InitializeSignalHandlers();


void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes);
void sendBytes(int sock, unsigned char* buff, uint16_t numBytes);

void handShake();
void sendUsername(unsigned char * username);
void printUsernames();

void readMultipleUsernames(uint16_t numberOfUsers);
void readAndAddUserName();
void readAndRemoveUserName();


void userAdded(char* username, int size);
void userRemoved(char* userName, int size);

void sendKeepAliveMessage();


uint16_t lengthOfString(unsigned char* userName);

void shutdownClient(int signal);



user* listUsers[MaxUsernameLength];

struct timeval timer;
struct sigaction priorSigHandler;
struct sigaction currentSigHandler;

int socketFD;

pthread_rwlock_t lock =  PTHREAD_RWLOCK_INITIALIZER;

 userLink* firstLink;

#endif 