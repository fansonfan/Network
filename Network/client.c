
#include "client.h"

int main(int argc, char *argv[])
{
    if(argc != 4){
    	printf("This program requires IP Address and Port number and username as 3 input arguments. Exiting...\n");
    	exit(1);
    }
    struct	sockaddr_in	server;
	struct	hostent		*host;

    host = gethostbyname(argv[1]);
    uint16_t port = atoi(argv[2]);
	
	InitializeSignalHandlers();
	timer.tv_sec = 5;
	timer.tv_usec = 0;

	if (host == NULL) {
		perror ("Client: cannot get host description");
		shutdownClient(1);
	}

	socketFD = socket(AF_INET, SOCK_STREAM, 0);

	if (socketFD < 0) {
		perror ("Client: cannot open socket");
		shutdownClient(1);
	}

	bzero (&server, sizeof (server));
	bcopy (host->h_addr, & (server.sin_addr), host->h_length);
	server.sin_family = host->h_addrtype;
	server.sin_port = htons(port);

	if (connect (socketFD, (struct sockaddr*) & server, sizeof (server))) {
		perror ("Client: cannot connect to server");
		shutdownClient(1);
	}
	
	
	handShake();

	//确认开始一个单独的线程进行侦听
	pthread_t thread;
	int ret = pthread_create(&thread, NULL, &recieveHandler, 0);
	if(ret){
		printf("Error Creating Thread %d", ret);
		shutdownClient(1);
	}

	sendUsername(argv[3]);
	chat();	
}


void InitializeSignalHandlers(){

	currentSigHandler.sa_handler = shutdownClient;
	sigemptyset(&currentSigHandler.sa_mask);
	currentSigHandler.sa_flags = 0;
	sigaction(SIGINT, &currentSigHandler, &priorSigHandler);
}


void shutdownClient(int signal){

	
	userLink * currentLink = firstLink;
	userLink * nextLink;

	for(;;){
		if(currentLink == NULL){
			break;
		}

		nextLink = currentLink->next;

		free(currentLink->user);
		free(currentLink);

		currentLink = nextLink;
	}

	printf("Exiting. Goodbye\n");

	
	sigaction(SIGINT, &priorSigHandler, 0);
	close(socketFD);
	exit(1);
}


void * recieveHandler(void * unUsed){
	
	
	uint16_t code;
	unsigned char buffUsername[BufferSize];
	unsigned char buffMessage[BufferSize];
	memset(buffUsername, '\0', BufferSize);
	memset(buffMessage, '\0', BufferSize);
	while(1){
		
		recievedBytes(socketFD, buffMessage, 1);
		code = (uint16_t)buffMessage[0];
		
		
		memset(buffMessage, '\0', BufferSize);

		
		if(code == 0x00){
				
			
			unsigned char stringSizeChar;
			recievedBytes(socketFD, &stringSizeChar, 1);

			
			uint16_t stringSizeInt = (uint16_t)stringSizeChar;
			recievedBytes(socketFD, buffUsername, stringSizeInt);

			recievedBytes(socketFD, buffMessage, 2);
			
			uint16_t networkSize = buffMessage[1] << 8 | buffMessage[0];
			uint16_t sizeMessage = ntohs(networkSize);
		
			
			memset(buffMessage, '\0', BufferSize);

			recievedBytes(socketFD, buffMessage, sizeMessage);
			
			
			printf("%s : %s\n", buffUsername, buffMessage);
			
			memset(buffMessage, '\0', BufferSize);
			memset(buffUsername, '\0', BufferSize);
		}

		
		else if(code == 0x01){
			
			readAndAddUserName();
		}

		
		else if(code == 0x02){
			
			readAndRemoveUserName();
		}

		
		else{
			printf("Server sent uknown code: %u\n", code);
			shutdownClient(0);
		}
	}
}


void handShake(){
	unsigned char confirmation_one_byte;
	unsigned char confirmation_two_byte;
	
	
	if (setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&timer, sizeof(timer)) < 0){
	 	perror("setsockopt failed\n");
	 	exit(1);
	}
	
	recievedBytes(socketFD, &confirmation_one_byte, sizeof(confirmation_one_byte));
	recievedBytes(socketFD, &confirmation_two_byte, sizeof(confirmation_two_byte));
	
	uint16_t first_confirmation = (uint16_t)confirmation_one_byte;
	uint16_t second_confirmation = (uint16_t)confirmation_two_byte;
	
	if( first_confirmation != 0xCF || second_confirmation != 0xA7){
		perror("Client: Did not recieve correct server authentication");
		exit(1);
	}

	uint16_t numberOfUsersNetwork;
	recievedBytes(socketFD, (unsigned char *)&numberOfUsersNetwork, sizeof(numberOfUsersNetwork));

	
	uint16_t intUsers  = ntohs(numberOfUsersNetwork);
	
	readMultipleUsernames(intUsers);

	
	timer.tv_sec = 0;
	if (setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&timer, sizeof(timer)) < 0){	
	 	perror("setsockopt failed\n");
	 	exit(1);
	}
}


void readMultipleUsernames(uint16_t numberOfUsers){
	
	int i;
	for( i = 0;  i < numberOfUsers ; i++ ){
		readAndAddUserName();
	}
}


void readAndAddUserName(){
		
	unsigned char buff[BufferSize];
	memset(buff, '\0', BufferSize);
	
	recievedBytes(socketFD, buff, 1);

	uint16_t size = (uint16_t)buff[0];
	memset(buff, '\0', BufferSize);

	recievedBytes(socketFD, buff, size);
	
	userAdded(buff, size);
}


void readAndRemoveUserName(){

	unsigned char buff[BufferSize];
	memset(buff, '\0', BufferSize);
	
	recievedBytes(socketFD, buff, 1);

	uint16_t size = (uint16_t)buff[0];
	memset(buff, '\0', BufferSize);

	recievedBytes(socketFD, buff, size);
	
	userRemoved(buff, size);
}


uint16_t lengthOfString(unsigned char* userName){
	
	int i;
	for(i = 0; i < BufferSize; i++){
		
		if(userName[i] == '\0' || userName[i] == '\n' )
			break;
	}

	return i;
}


void sendUsername(unsigned char* username){
	//Put in the length at the start
	uint16_t sizeUser = lengthOfString(username);
	unsigned char user = (unsigned char) sizeUser;
	
	sendBytes(socketFD, &user, 1);
	sendBytes(socketFD, username, sizeUser);
}


void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes){

	int numberToRead = numBytes;
	unsigned char * spotInBuffer = buff;
	while(numberToRead > 0){
		
		int readBytes = recv(sock, spotInBuffer, numberToRead, 0);
		if(readBytes <= 0){
			printf("Server has hung up shutting down...\n");
			shutdownClient(0);			
		}

		
		spotInBuffer += readBytes;
		numberToRead -= readBytes;
	}
}


void sendBytes(int sock, unsigned char* buff, uint16_t numBytes){

	int numberToWrite = numBytes;
	unsigned char* spotInBuffer = buff;

 	while(numberToWrite > 0){
		int sentBytes = send(sock, spotInBuffer, numberToWrite, 0);

		if(sentBytes <= 0){
			printf("Server has hung up shutting down...\n");
			shutdownClient(0);			
		}

		
		spotInBuffer += sentBytes;
		numberToWrite -= sentBytes;
	}
}


void chat(){

	for(;;){

		struct timeval tv;
    	fd_set fds;

    	tv.tv_sec = 20;
    	tv.tv_usec = 0;

   		FD_ZERO(&fds);
   		
    	FD_SET(STDIN_FILENO, &fds);

		if(select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) <= 0){
			
			sendKeepAliveMessage();
			continue;
		}

		unsigned char buff[BufferSize];
		fgets(buff, BufferSize, stdin);

		if(strncmp(buff, ".usernames", 10) == 0){
			sendKeepAliveMessage();
			printUsernames();
			continue;
		}
		
		uint16_t size = lengthOfString(buff);
		uint16_t networkSize = htons(size);

		sendBytes(socketFD, (unsigned char *)&networkSize, 2);
		sendBytes(socketFD, buff, size);

	
		memset(buff, '\0', BufferSize);
	}
}


void userAdded(char* username, int size){
	
	user * User = (user *)malloc(sizeof(user));
	User->len = size;
	memcpy(&(User->username), username, size); 
	User->username[size] = '\0';

	userLink* link = malloc(sizeof(userLink));
	link->user = User;
	link->next = NULL;

	pthread_rwlock_wrlock(&lock);

	if(firstLink == NULL){
		
		firstLink = link;	
	}

	else{
		
		userLink* currentLink = firstLink;
		while(currentLink->next != NULL){
			currentLink = currentLink->next;
		}
		currentLink->next = link;
	}

	printf(" %s has been added\n", link->user->username);

	pthread_rwlock_unlock(&lock);
}


void userRemoved(char* userName, int size){

	pthread_rwlock_wrlock(&lock);

	userLink* priorLink = firstLink;
	userLink* currentLink = firstLink;
	for(;;){
		
		if(currentLink == NULL){
			printf("Unable to find username to remove\n");
			break;
		}

		
		if(strncmp(currentLink->user->username, userName, size) == 0){
			userLink* removedLink = currentLink;
			printf(" %s has been removed\n", removedLink->user->username);
			
			if(priorLink != currentLink){
				
				priorLink->next = currentLink->next;

			}
			
			else{
				
				firstLink = currentLink->next;
			}

			free(removedLink->user);
			free(removedLink);
			break;
		}

		priorLink = currentLink;
		currentLink = currentLink->next;
	}

	pthread_rwlock_unlock(&lock);
}


void printUsernames(){
	
	pthread_rwlock_rdlock(&lock);
	
	userLink * currentLink = firstLink;
	while(currentLink != NULL){
	
		printf("%s\n", currentLink->user->username);
		currentLink = currentLink->next;
	}
	pthread_rwlock_unlock(&lock);
}


void sendKeepAliveMessage(){

	uint16_t zeroCode = 0;
	uint16_t networkZeroCode = htons(zeroCode);

	sendBytes(socketFD, (unsigned char *)&networkZeroCode, 2);
}





