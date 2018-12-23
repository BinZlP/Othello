#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


#define MAX_CLIENT_NUM 2
#define MAX_WAITING_QUEUE 2


typedef enum __reqtype{
	REQUEST_CONNECT=1,
	REQUEST_DISCONNECT,
	REQUEST_SIGNUP,
	REQUEST_SIGNIN,
	REQUEST_SIGNOUT,
	REQUEST_WITHDRAWL,
	REQUEST_WAITING,
	REQUEST_SIGNUP_ERROR,
	REQUEST_SIGNIN_ERROR,
	REQUEST_WITHDRAWL_ERROR,
	REQUEST_GAME,
	REQUEST_INVALID_POS,
	REQUEST_GAME_EXIT,
	REQUEST_OK,
	REQUEST_REGAME
}RequestType;


typedef struct __user{
    int win;
    int lose;

    char id[50];
    char pw[50];
}User;


typedef struct __game{
	int x;
	int y;
}GamePos;

typedef struct __reqmsg{
	RequestType type;
	User info;
	GamePos game;
}RequestMessage;

typedef struct __gameinfo{
	char gameBoard[6][6];
	char p1name[50];
	char p2name[50];
	int p1count;
	int p2count;
	int turn;
	int isFinished;
}GameInfo;


int listen_socket=-1;
int client_socket[MAX_CLIENT_NUM]={-1,};
int client_num=0;
int waiting_client=0;
User gameplayer[2]={0,};
char loggedin_user[50]={0,};

// *** SELECT() RELATED FUNCTIONS ***
int getMaxFd();
int addClient();
int removeClient();

// *** SERVER FUNCTIONS ***
void signIn(int client_fd, RequestMessage* msg);
void signUp(int client_fd, RequestMessage* msg);
void signOut(RequestMessage* msg);
void userWithdrawl(int client_fd, RequestMessage* msg);
int inGame(int client_fd1, int client_fd2);
void waitAndStart(int client_fd, RequestMessage* msg);
void userInfoModify(User* player, int isWin);
int checkGameOver(char m, GameInfo* info);
int checkValid(int x, int y, char m, int direction, GameInfo* info);
void moveToDirection(int* x, int* y, int direction);
void flip(int x, int y, char m, int direction, GameInfo* info);
void validSoFlip(int* isValid, int x, int y, char m, int direction, GameInfo* info);
int setMarker(int x, int y, char m, GameInfo* info);


// *** SIGNAL HANDLER ***
void signalHandler(int signum){
	if(signum==SIGINT||signum==SIGKILL){
		for(int i = 0; i < client_num; ++i) { close(client_socket[i]); }
		close(listen_socket);
		exit(-1);
	}
}


int main(int argc, const char* argv[]){
	int i=0, j=0;
	struct sockaddr_in server_addr={0x00,};
	fd_set readfds;
	RequestMessage msg={0,};


	if(argc<2){
		printf("Usage: %s [PORT]\n",argv[0]);
		exit(0);
	}

	listen_socket=socket(PF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = PF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(atoi(argv[1]));

	bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
	listen(listen_socket, MAX_WAITING_QUEUE);

	while(1){
		FD_ZERO(&readfds);
		FD_SET(listen_socket, &readfds);
		for(int i=0;i<client_num;i++) FD_SET(client_socket[i],&readfds);

		select(getMaxFd(),&readfds,NULL,NULL,NULL);

		if(FD_ISSET(listen_socket, &readfds)) 
			if(addClient()==-1) printf("addClient() error!\n");

		for(int i=0; i<client_num; i++){
			if(FD_ISSET(client_socket[i],&readfds)){ // If there's any request from client
				memset(&msg,0x00,sizeof(msg));
				recv(client_socket[i],&msg,sizeof(msg),0);
				switch(msg.type){
					case REQUEST_SIGNIN: signIn(client_socket[i],&msg); break;
					case REQUEST_SIGNUP: signUp(client_socket[i],&msg); break;
					case REQUEST_WITHDRAWL: userWithdrawl(client_socket[i],&msg); break;
					case REQUEST_WAITING: waitAndStart(client_socket[i],&msg); break;
					case REQUEST_DISCONNECT: removeClient(i); break;
					case REQUEST_SIGNOUT: signOut(&msg); break;
				}
			}
		}
	}

	return 0;
}

// *** CLIENT MANAGEMENT FUNCTIONS ***
int getMaxFd(void) {
	int i = 0;
	int max = listen_socket;
	for(i = 0; i < client_num; ++i) {
	if (max < client_socket[i]) 
		{ max = client_socket[i]; }
	}
	return max + 1;
}

int addClient(void) {
	int client = 0;
	int clientAddrSize = 0;
	struct sockaddr_in clientAddr = { 0x00, };
	RequestMessage msg = { 0x00, };
	client = accept(listen_socket, (struct sockaddr *)&clientAddr,&clientAddrSize);
	if (client_num == MAX_CLIENT_NUM) { return -1; }
	client_socket[client_num] = client;
	++client_num;
	//printf("New client connected.\n");
	printf("Concurrent user: %d\n",client_num);
	return 0;
}

int removeClient(int n) {
	close(client_socket[n]);
	//printf("Client disconnected.\n");
	
	if (n != client_num - 1) {
		client_socket[n] = client_socket[client_num - 1];
		client_socket[client_num - 1] = -1;
	}
	--client_num;

	printf("Concurrent user: %d\n",client_num);
	return 0;
}


// *** SERVER FUNCTIONS ***

void signIn(int client_fd, RequestMessage* msg){
	RequestMessage res={0,};
    
	if(!strcmp(msg->info.id,loggedin_user)){
		res.type=REQUEST_SIGNIN_ERROR;
		printf("SIGN IN ERROR: %s\n", msg->info.id);
		send(client_fd,&res,sizeof(res),0);
		return;
	}
	
	int fd;
    User* cmp=(User*)malloc(sizeof(User));
	memset(cmp,0x00,sizeof(User));
    fd=open("user.txt",O_RDONLY);
    while(read(fd,cmp,sizeof(User))>0){
        if(!strcmp(msg->info.id,cmp->id))
            if(!strcmp(msg->info.pw,cmp->pw)){
				res.type=REQUEST_OK;
                strcpy(res.info.id,cmp->id);
                res.info.win=cmp->win;
                res.info.lose=cmp->lose;
				send(client_fd,&res,sizeof(res),0);
                strcpy(loggedin_user,cmp->id);

				printf("SIGN IN: %s\n",res.info.id);
				
				free(cmp);
				close(fd);
                return;
            }
        memset(cmp,0x00,sizeof(User));
    }
	res.type=REQUEST_SIGNIN_ERROR;
	printf("SIGN IN ERROR: %s\n", msg->info.id);
	send(client_fd,&res,sizeof(res),0);
	close(fd);
    free(cmp);
    return;
}

void signUp(int client_fd, RequestMessage* msg){
	int fd; 
    int rsize=0;
    User ins;
    User* comp=(User*)malloc(sizeof(User));
	RequestMessage res={0,};

    memset(comp,0x00,sizeof(User));
    fd=open("user.txt",O_CREAT|O_RDWR,0666);
    while((rsize=read(fd,comp,sizeof(User)))>0){
        if(!strcmp(msg->info.id,comp->id)){ // If requested id is existing in DB
			res.type=REQUEST_SIGNUP_ERROR;
			strcpy(res.info.id,msg->info.id);
			send(client_fd, &res, sizeof(res), 0);
			printf("SIGN UP ERROR: %s\n",msg->info.id);
			return;
		}
        memset(comp,0x00,sizeof(User));
    }

    memset(&ins.id,0x00,50);
    memset(&ins.pw,0x00,50);
    strcpy(ins.id,msg->info.id);
    strcpy(ins.pw,msg->info.pw);
    ins.win=0;
    ins.lose=0;

    lseek(fd,0,SEEK_END);
    if(write(fd,&ins,sizeof(User))<0){
        perror("write()");
        exit(0);
    }

	res.type=REQUEST_OK;
	strcpy(res.info.id,msg->info.id);
	send(client_fd, &res, sizeof(res), 0);
	printf("SIGN UP: %s\n",res.info.id);

    close(fd);
    free(comp);
    return;
}

void signOut(RequestMessage* req){
	if(!strcmp(req->info.id,loggedin_user)){
		memset(loggedin_user,0x00,50);
	}
	printf("SIGN OUT: %s\n",req->info.id);
}

void userWithdrawl(int client_fd, RequestMessage* msg){
	int fd;
    User* cmp=(User*)malloc(sizeof(User));
	RequestMessage res={0,};
    memset(cmp,0x00,sizeof(User));
    fd=open("user.txt",O_RDWR);
    while(read(fd,cmp,sizeof(User))>0){
        if(!strcmp(msg->info.id,cmp->id))
            if(!strcmp(msg->info.pw,cmp->pw)){
                memset(cmp,0x00,sizeof(User));
                lseek(fd,-sizeof(User),SEEK_CUR);
                write(fd,cmp,sizeof(User));

				res.type=REQUEST_OK;
				send(client_fd, &res, sizeof(res), 0);
				printf("WITHDRAWL: %s\n",msg->info.id);

                free(cmp);
				close(fd);
                return;
            }
        memset(cmp,0x00,sizeof(User));
    }

	res.type=REQUEST_WITHDRAWL_ERROR;
	send(client_fd, &res, sizeof(res), 0);
	printf("WITHDRAWL ERROR: %s\n",msg->info.id);

	close(fd);
    free(cmp);
    return;
}


// *** GAME ***
void waitAndStart(int client_fd, RequestMessage* msg){
	if(waiting_client==0) {
		waiting_client=client_fd;
		memcpy(&gameplayer[0],&msg->info,sizeof(User));
		printf("WAITING: %s\n",msg->info.id);
		return;
	}
	else{
		RequestMessage gamestart1={0,};
		RequestMessage gamestart2={0,};
		int gameResult=0;

		memcpy(&gameplayer[1],&msg->info,sizeof(User));

		gamestart1.type=REQUEST_GAME;
		gamestart2.type=REQUEST_WAITING;
		memcpy(&gamestart1.info,&gameplayer[1],sizeof(User));
		memcpy(&gamestart2.info,&gameplayer[0],sizeof(User));

		send(waiting_client,&gamestart1,sizeof(gamestart1),0);
		send(client_fd,&gamestart2,sizeof(gamestart2),0);
		printf("GAME START: %s vs %s\n",gameplayer[0].id,gameplayer[1].id);
		gameResult=inGame(waiting_client,client_fd);

		if(gameResult==3) return;
		userInfoModify(&gameplayer[0], gameResult==1? 1 : 0);
		userInfoModify(&gameplayer[1], gameResult==1? 0 : 1);

		waiting_client=0;
		return;
	}
}

int inGame(int client_fd1, int client_fd2){
	GameInfo info={0,};
	RequestMessage rcv={0,};

	int gameExitByClient=0;
	int isRegame=0;

	strcpy(info.p1name,gameplayer[0].id);
	strcpy(info.p2name,gameplayer[1].id);

	do{ // Loop if regame selected by client
	
	// If game has ended, not ended with draw and restarted
	if(info.isFinished&&(info.isFinished!=3&&isRegame)){
		userInfoModify(&gameplayer[0], info.isFinished==1?1:0);
		userInfoModify(&gameplayer[1], info.isFinished==1?0:1);
	}

	// Game Info. message setting
	for(int i=0;i<6;i++)
		for(int j=0;j<6;j++)
			info.gameBoard[i][j]=' ';
	info.gameBoard[2][2]='O';
	info.gameBoard[2][3]='X';
	info.gameBoard[3][2]='X';
	info.gameBoard[3][3]='O';
	info.p1count=2;
	info.p2count=2;
	info.turn=1;
	info.isFinished=0;
	isRegame=0;

	send(client_fd1, &info, sizeof(info), 0);
	send(client_fd2, &info, sizeof(info), 0);

	// Game Management - Client started game 
	while(!gameExitByClient){
		//printf("Player 1: %d\t Player 2: %d\n",p1count,p2count);
		while(1){
			//printf("Player 1's turn.\n");
			if(checkGameOver('O', &info)){
				//printf("Player 1 doesn't have any valid mark place.\n");
				info.turn=2;
				break;
			}

			recv(client_fd1, &rcv, sizeof(rcv), 0);
			if(rcv.type==REQUEST_GAME_EXIT){
				gameExitByClient=1;
				break;
			}else if(rcv.type==REQUEST_REGAME){
				isRegame=1;
				break;
			}
			if(rcv.game.x==-1&&rcv.game.y==-1) {
				info.turn=2;
				send(client_fd1, &info, sizeof(info), 0);
				break;
			}
			if(setMarker(rcv.game.y,rcv.game.x,'O', &info)) {
				printf("%s: (%d, %d)\n",info.p1name,rcv.game.x+1, rcv.game.y+1);
				info.turn=2;
				send(client_fd1, &info, sizeof(info), 0);
				break;
			}
			else {
				//printf("Invalid position. Try again!\n");
				send(client_fd1, &info, sizeof(info), 0);
			}
		}

		if(gameExitByClient){
			info.isFinished=gameExitByClient==1?2:1;
			break;
		}else if(isRegame){
			break;
		}
		if(checkGameOver(0, &info)) {
			if(info.p1count>info.p2count) info.isFinished=1;
			else if(info.p2count>info.p1count) info.isFinished=2;
			else info.isFinished=3; // Draw
			break;
		}
		send(client_fd1, &info, sizeof(info), 0);
		send(client_fd2, &info, sizeof(info), 0);

		while(1){
			//printf("Player 2's turn.\n");
			if(checkGameOver('X', &info)){
				//printf("Player 2 doesn't have any valid mark place.\n");
				info.turn=1;
				break;
			}
			
			recv(client_fd2, &rcv, sizeof(rcv), 0);
			if(rcv.type==REQUEST_GAME_EXIT){
				gameExitByClient=2;
				break;
			}else if(rcv.type==REQUEST_REGAME){
				isRegame=1;
				break;
			}
			if(rcv.game.x==-1&&rcv.game.y==-1) {
				info.turn=1;
				send(client_fd2, &info, sizeof(info), 0);
				break;
			}
			if(setMarker(rcv.game.y, rcv.game.x, 'X', &info)) {
				info.turn=1;
				printf("%s: (%d, %d)\n",info.p2name,rcv.game.x+1, rcv.game.y+1);
				send(client_fd2, &info, sizeof(info), 0);
				break;
			}
			else {
				//printf("Invalid position. Try again!\n");
				send(client_fd2, &info, sizeof(info), 0);
			}
		}


		if(gameExitByClient){
			info.isFinished=gameExitByClient==1?2:1;
			break;
		}else if(isRegame){
			break;
		}
		if(checkGameOver(0, &info)) {
			if(info.p1count>info.p2count) info.isFinished=1;
			else if(info.p2count>info.p1count) info.isFinished=2;
			else info.isFinished=3;
			break;
		}

		send(client_fd1, &info, sizeof(info), 0);
		send(client_fd2, &info, sizeof(info), 0);
	}

	if(isRegame) continue;

	send(client_fd1, &info, sizeof(info), 0);
	send(client_fd2, &info, sizeof(info), 0);

	if(gameExitByClient){
		int winner_fd=gameExitByClient==1?client_fd2:client_fd1;
		rcv.type=REQUEST_GAME_EXIT;
		send(winner_fd,&rcv,sizeof(rcv),0);
		printf("WINNER: %s\n",gameExitByClient==1?info.p2name:info.p1name);
		return gameExitByClient==1?2:1;
	}

	if(info.isFinished==3){// if draw
		printf("DRAW \n");
		recv(client_fd1, &rcv, sizeof(rcv), 0);
		isRegame= rcv.type==REQUEST_GAME ? 1 : 0;
		rcv.type= rcv.type==REQUEST_GAME ? REQUEST_GAME : REQUEST_GAME_EXIT ;
		send(client_fd1, &rcv, sizeof(rcv), 0);
		send(client_fd2, &rcv, sizeof(rcv), 0);
	}else{
		int loser_fd = info.p1count>info.p2count ? 
			client_fd2 : client_fd1;

		printf("WINNER: %s\n", info.p1count>info.p2count ? 
			info.p1name : info.p2name);

		
		recv(loser_fd, &rcv, sizeof(rcv), 0);
		isRegame= rcv.type==REQUEST_GAME ? 1 : 0;
		rcv.type= rcv.type==REQUEST_GAME ? REQUEST_GAME : REQUEST_GAME_EXIT ;
		send(client_fd1, &rcv, sizeof(rcv), 0);
		send(client_fd2, &rcv, sizeof(rcv), 0);
	}

	}while(isRegame);
	return info.isFinished;
}

int checkGameOver(char m, GameInfo* info){
	char op=m=='O'?'X':'O';
	op=m==0?' ':op;

	if((info->p1count+info->p2count)==36) return 1; // If game board is full
	else if(info->p1count==0||info->p2count==0) return 1; // If one side lost all markers
	else{
		for(int i=0;i<6;i++){
			for(int j=0;j<6;j++){
				if(info->gameBoard[i][j]=='O'||info->gameBoard[i][j]=='X') continue;
				else{
					for(int d=1;d<11;d++){
						if(d==3||d==7) continue;
						int x=i, y=j;
						moveToDirection(&x,&y,d);
						if(m==0){
							if(checkValid(x,y,'O',d,info)||checkValid(x,y,'X',d,info)) return 0;
						}else if(checkValid(x,y,m,d,info)) return 0; // If there're any valid position
					}
				}
			}
		}
	}
	return 0;
}

void moveToDirection(int* x, int* y, int direction){
	switch(direction){
		case 1: (*x)--; break;
		case 2: (*x)++; break;
		case 4: (*y)--; break;
		case 8: (*y)++; break;

		case 5: (*x)--; (*y)--; break;
		case 9: (*x)--; (*y)++; break;
		case 6: (*x)++; (*y)--; break;
		case 10: (*x)++; (*y)++; break;
	}
}

int checkValid(int x, int y, char m, int direction, GameInfo* info) {
	// (x, y) is opponent's position near by input

	// UP=1 DOWN=2 LEFT=4 RIGHT=8
	// UPLEFT=5, UPRIGHT=9, DOWNLEFT=6, DOWNRIGHT=10 
	char op = m=='O' ? 'X' : 'O';
	moveToDirection(&x,&y,direction); // Change x and y by direction
	if(x>5||x<0||y>5||y<0) return 0;

	if(info->gameBoard[x][y]==m) return 1;
	else if(info->gameBoard[x][y]==op) return checkValid(x,y,m,direction,info);
	else return 0; 
}

void flip(int x, int y, char m, int direction, GameInfo* info){
	if(info->gameBoard[x][y]==m) return; // If my marker
	else if(x<0||x>5||y<0||y>5) return;
	else{
		info->gameBoard[x][y]=m;
		switch(m){
			case 'O': info->p1count++; info->p2count--; break;
			case 'X': info->p2count++; info->p1count--; break;
		}
		moveToDirection(&x,&y,direction);
		flip(x,y,m,direction,info);
	}
}

void validSoFlip(int* isValid, int x, int y, char m, int direction, GameInfo* info){
	if(x<0||x>5||y<0||y>5) return;
	(*isValid)++; // Set ok flag
	if(info->gameBoard[x][y]!=m){
		info->gameBoard[x][y]=m;
		if(m=='O') info->p1count++;
		else info->p2count++;
	} 
	moveToDirection(&x,&y,direction);
	flip(x,y,m,direction,info);
}

int setMarker(int x, int y, char m, GameInfo* info) {
	// Opponent's marker
	char op = m=='O' ? 'X' : 'O';
	int isValid=0;

	if(info->gameBoard[x][y]!=' ') return 0;

	// UP=1 DOWN=2 LEFT=4 RIGHT=8
	// UPLEFT=5, UPRIGHT=9, DOWNLEFT=6, DOWNRIGHT=10 

	// Check all direction of request
	for(int i=-1;i<2;i+=2){
		if(info->gameBoard[x][y+i]==op)
			if(checkValid(x,y+i,m,i<0?4:8,info))
				validSoFlip(&isValid,x,y,m,i<0?4:8,info);
	}

	for(int ud=-1;ud<2;ud+=2){
		if(info->gameBoard[x+ud][y]==op) {
			if(checkValid(x+ud,y,m,ud<0?1:2,info)){
				validSoFlip(&isValid,x,y,m,ud<0?1:2,info);
			}
		}

		for(int lr=-1;lr<2;lr+=2){
			if(info->gameBoard[x+ud][y+lr]==op){
				int d=ud<0?1:2; // Up or Down
				d+=lr<0?4:8; // Left or Right
				if(checkValid(x+ud,y+lr,m,d,info)){ // If valid
					/*
					isValid++; // Set ok flag
					gameBoard[x][y]=m;
					if(m=='O') p1count++;
					else p2count++;
					flip(x+ud,y+lr,m,d); // Flip all opponent's markers
					*/
					validSoFlip(&isValid,x,y,m,d,info);
				}
			}
		}
	}

	if(isValid) return 1;
	else return 0;
}

void userInfoModify(User* player, int isWin){
	int fd;
    User* cmp=(User*)malloc(sizeof(User));
	RequestMessage res={0,};
    memset(cmp,0x00,sizeof(User));
    fd=open("user.txt",O_RDWR);
    while(read(fd,cmp,sizeof(User))>0){
        if(!strcmp(player->id,cmp->id)){
			if(isWin) cmp->win++;
			else cmp->lose++;
			lseek(fd,-sizeof(User),SEEK_CUR);
			write(fd,cmp,sizeof(User));
			break;
		}
        memset(cmp,0x00,sizeof(User));
    }
	free(cmp);
	close(fd);
	return;
}