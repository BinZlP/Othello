#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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



// *** LOGIN SESSION INFORMATION ***
char login_id[50];
int login_win=0;
int login_lose=0;
int isLogin=0;

// *** CONNECTION GLOBALS ***
char* SERVER_IP;
char* SERVER_PORT;
int server_fd=0;

// *** CONNECTION FUNCTIONS ***
int connectInit();

int connectInit(){
	struct sockaddr_in server_addr={0x00,};
	server_fd=socket(PF_INET, SOCK_STREAM, 0);

	
	server_addr.sin_family=PF_INET;
	server_addr.sin_addr.s_addr=inet_addr(SERVER_IP);
	server_addr.sin_port=htons(atoi(SERVER_PORT));

	if(connect(server_fd,(struct sockaddr*)&server_addr, sizeof(server_addr))){
        perror("connect()");
        return -1;
    }

    return 0;
}


// *** GAME FUNCTIONS ***
int mainMenu();
int signin(WINDOW* window1, WINDOW* window2);
void signup(WINDOW* window1, WINDOW* window2);
void mainAfterLogin(WINDOW* window1, WINDOW* window2);
void logOut();
int userStat(WINDOW* window1, WINDOW* window2);
void Withdrawl(WINDOW* window1, WINDOW* window2);
int Game();
int setMark(int cursor, int myTurn);
void moveToDirection(int* x, int* y, int direction);
int checkValid(int x, int y, char m, int direction, char gameBoard[6][6]);
int checkGameOver(char m, char gameBoard[6][6]);
int insertUser(User* user);
int loginUser(User* user);
int deleteUser(User* user);
void disconnect();

void printMenu(WINDOW* window, char* menus[], int cursor, int menu_number);
void printGameMenu(WINDOW* window, int menuCursor, GameInfo* game);
void printGameBoard(WINDOW* window, char board[6][6], int cursor);

// Print top windows
void printMainW1(WINDOW* window1){
    wclear(window1);
    wmove(window1, 6, 26);
    wprintw(window1, "System Software Practice");
    
    wmove(window1, 8, 36);
    wattron(window1, A_DIM);
    wprintw(window1, "OTHELLO");
    wattroff(window1, A_DIM);

    wmove(window1, 13, 66);
    wprintw(window1, "2017203044");

    wmove(window1, 15, 66);
    wprintw(window1, "Hyungseok Han");
    wrefresh(window1);
}
void printSignU1(WINDOW* window1){
    wclear(window1);
    wmove(window1, 6, 36);
    wattron(window1, A_DIM);
    wprintw(window1, "SIGN UP");
    wattroff(window1, A_DIM);

    wmove(window1, 8, 31);
    wprintw(window1, "ID : ");

    wmove(window1, 10, 25);
    wprintw(window1, "PASSWORD : ");
    wrefresh(window1);
}

void printSignI1(WINDOW* window1){
    wclear(window1);
    wmove(window1, 6, 36);
    wattron(window1, A_DIM);
    wprintw(window1, "SIGN IN");
    wattroff(window1, A_DIM);

    wmove(window1, 8, 31);
    wprintw(window1, "ID : ");

    wmove(window1, 10, 25);
    wprintw(window1, "PASSWORD : ");
    wrefresh(window1);
}
void printMainAfterLogin1(WINDOW* window1){
    wclear(window1);
    wmove(window1, 6, 40-(12+strlen(login_id))/2);
    wprintw(window1, "PLAYER ID : %s", login_id);
    wrefresh(window1);
}
void printWithdrawl1(WINDOW* window1){
    wclear(window1);
    wmove(window1, 6, 36);
    wattron(window1, A_DIM);
    wprintw(window1, "WITHDRAWL");
    wattroff(window1, A_DIM);

    wmove(window1, 8, 24);
    wprintw(window1, "PLAYER ID : %s",login_id);

    wmove(window1, 10, 25);
    wprintw(window1, "PASSWORD : ");
    wrefresh(window1);
}
void printPlayerInfo1(WINDOW* window1, RequestMessage* msg, int player_num){
    wclear(window1);

    double op_winrate = msg->info.win+msg->info.lose==0 ?
        0 : msg->info.win/(double)(msg->info.win+msg->info.lose);
    double my_winrate = login_win+login_lose==0 ?
        0 : login_win/(double)(login_win+login_lose);
    
    if(player_num==1){
        wmove(window1, 6, 20-(12+strlen(login_id))/2);
        wprintw(window1, "PLAYER ID : %s", login_id);

        wmove(window1, 8, 15);
        wattron(window1, A_DIM);
        wprintw(window1, "STATISTICS");
        wattroff(window1, A_DIM);

        wmove(window1, 10, 6);
        wprintw(window1, "WIN : %d / LOSE : %d (%.3lf%%)", login_win, login_lose, my_winrate);

        wmove(window1, 6, 60-(12+strlen(msg->info.id))/2);
        wprintw(window1, "PLAYER ID : %s", msg->info.id);

        wmove(window1, 8, 55);
        wattron(window1, A_DIM);
        wprintw(window1, "STATISTICS");
        wattroff(window1, A_DIM);

        wmove(window1, 10, 46);
        wprintw(window1, "WIN : %d / LOSE : %d (%.3lf%%)",msg->info.win, msg->info.lose, op_winrate);
    }else{
        wmove(window1, 6, 20-(12+strlen(msg->info.id))/2);
        wprintw(window1, "PLAYER ID : %s",  msg->info.id);

        wmove(window1, 8, 15);
        wattron(window1, A_DIM);
        wprintw(window1, "STATISTICS");
        wattroff(window1, A_DIM);

        wmove(window1, 10, 6);
        wprintw(window1, "WIN : %d / LOSE : %d (%.3lf%%)", msg->info.win, msg->info.lose, op_winrate);

        wmove(window1, 6, 60-(12+strlen(login_id))/2);
        wprintw(window1, "PLAYER ID : %s", login_id);

        wmove(window1, 8, 55);
        wattron(window1, A_DIM);
        wprintw(window1, "STATISTICS");
        wattroff(window1, A_DIM);

        wmove(window1, 10, 46);
        wprintw(window1, "WIN : %d / LOSE : %d (%.3lf%%)", login_win, login_lose, my_winrate);
    }

    wrefresh(window1);
}




// Start point of program
int main(int argc, char const *argv[]) { 
    if(argc<3){
    	printf("Usage: ./Client [SERVER_IP] [SERVER_PORT]");
    	exit(0);
    }

    memset(login_id,0x00,50);
    SERVER_IP=argv[1];
    SERVER_PORT=argv[2];
    if(connectInit()==-1){
    	printf("Connection error!\n");
    	exit(0);
    }

    int exitcode=mainMenu();
    return exitcode;
}


// *** MAIN MENU ***

int mainMenu(){
    WINDOW *window1;
    WINDOW *window2;

    char* menus[]={
        "SIGN IN",
        "SIGN UP",
        "EXIT"
    };
    int mainCursor=0, menu_num=3, isNotExit=1, loginres=0;
    int ch;

    initscr();

    if (has_colors() == FALSE) {
        puts("Terminal does not support colors!"); 
        endwin();
        return 0;
    } else {
        start_color();
        init_pair(1, COLOR_BLUE, COLOR_WHITE);
        init_pair(2, COLOR_WHITE, COLOR_BLUE);
    }
    refresh();

    window1 = newwin(18, 80, 0, 0);
    window2 = newwin(6, 80, 18, 0);

    wbkgd(window1, COLOR_PAIR(1));
    wbkgd(window2, COLOR_PAIR(2));

    signal(SIGINT,disconnect);
    signal(SIGKILL,disconnect);

    // Print Main 
    printMainW1(window1);
    wrefresh(window2);

    noecho(); // No print
    cbreak(); // Pass on everything
    curs_set(0); // No cursor

    printMenu(window2,menus,mainCursor,menu_num);
    //nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    while(isNotExit){
        ch=getch();

        switch(ch){
        case KEY_LEFT:
            if(mainCursor!=0) mainCursor--;
            break;
        case KEY_RIGHT:
            if(mainCursor!=2) mainCursor++;
            break;
        case '\n':
            switch(mainCursor){
            case 0:
                loginres=signin(window1, window2);
                if(loginres){
                    mainAfterLogin(window1,window2);
                }
                break;
            case 1:
                signup(window1, window2);
                break;
            case 2:
                isNotExit=0; break;
            }
            break;
        default: break;
        }
        
        printMainW1(window1);
        printMenu(window2,menus,mainCursor,menu_num);
    }

    disconnect(0);
    endwin();

    return 1; 
}

void disconnect(int signum){
    RequestMessage msg={0,};
    msg.type=REQUEST_DISCONNECT;
    send(server_fd,&msg,sizeof(msg),0);
    if(signum==SIGINT||signum==SIGKILL) exit(0);
    return;
}

void printMenu(WINDOW* window, char* menus[], int cursor, int button_num){
    wclear(window);

    int x=10, y=3;
    if(button_num==1) x=39;

    for(int i = 0; i < button_num; ++i){   
        wmove(window,y,x+(80/(button_num))*i);
        if(cursor == i) {
            wattron(window, A_REVERSE); 
            wprintw(window, "%s", menus[i]);
            wattroff(window, A_REVERSE);
        }
        else
            wprintw(window, "%s", menus[i]);
    }
    wrefresh(window);
}


// *** SIGN IN ***

int signin(WINDOW* window1, WINDOW* window2){
    char* menus[]={
        "SIGN IN",
        "BACK"
    };

    char id[50];
    char pw[50];
    User* usr=(User*)malloc(sizeof(User));

    int menu_num=2, isNotBack=1, res=1, mainCursor=0;
    int ch;

    // Print frame
    wclear(window1);
    wclear(window2);
    wrefresh(window2);

    noecho(); // No print

    printMenu(window2,menus,mainCursor,menu_num);
    //nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    while(isNotBack){
        curs_set(1); // cursor visible

        printSignI1(window1);
        memset(id,0x00,sizeof(id)/sizeof(char));
        memset(pw,0x00,sizeof(pw)/sizeof(char));
        memset(usr,0x00,sizeof(User));
        int isEnter=0, i=0;
        mainCursor=0;
        res=1;

        wmove(window1,8,36);
        //echo();
        while(!isEnter){
            ch=wgetch(window1);
            switch(ch){
            case 8:
            case 127:
                if(i>0) {
                    id[--i]=0;
                    wmove(window1,8,36+i);
                    wechochar(window1, ' ');
                }
                break;
            case '\n':
                isEnter=1; break;
            default:
                id[i++]=ch; break;
            }
            wmove(window1,8,36);
            for(int j=0;j<i;j++)
                wechochar(window1, id[j]);
        }

        isEnter=0;
        i=0;
        wmove(window1,10,36);
        noecho();
        while(!isEnter){
            ch=wgetch(window1);
            switch(ch){
                case 8: // Backspace
                case 127:
                    if(i>0){
                        pw[--i]=0; 
                        wmove(window1,10,36+i);
                        wechochar(window1,' ');
                        wmove(window1,10,36+i);
                    }
                    break;
                case '\n':
                    isEnter=1; break;
                default:
                    pw[i++]=ch; 
                    wmove(window1,10,36+i-1);
                    wechochar(window1,'*');
                    break;
            }
        }

        strcpy(usr->id,id);
        strcpy(usr->pw,pw);

        curs_set(0); // Cursor invisible

        while(isNotBack&&res){
            ch=getch();
            switch(ch){
            case KEY_LEFT:
                if(mainCursor!=0) mainCursor--;
                break;
            case KEY_RIGHT:
                if(mainCursor!=1) mainCursor++;
                break;
            case '\n':
                switch(mainCursor){
                case 0:
                    res=loginUser(usr);
                    
                    if(res){
                        strcpy(login_id,usr->id);
                        isLogin=1;
                        free(usr);
                        return 1;
                    }else{
                        wmove(window2,5,0);
                        wprintw(window2,">>> Check your ID and PASSWORD. (Press any key...)",id);
                        wrefresh(window2);
                        getch();
                    }
                    break;
                case 1:
                    isNotBack=0;
                    break;
                }
                break;

            default: break;
            }
            printMenu(window2,menus,mainCursor,menu_num);
        }
        
    }
    free(usr);
    return 0;
}


// *** SIGN UP ***

void signup(WINDOW* window1, WINDOW* window2){

    char* menus[]={
        "SIGN UP",
        "BACK"
    };
    char id[50];
    char pw[50];
    User* usr=(User*)malloc(sizeof(User));

    int menu_num=2, isNotBack=1, res=1, mainCursor=0;
    int ch;

    // Print frame
    wrefresh(window2);

    noecho(); // No print
    cbreak();


    printMenu(window2,menus,mainCursor,menu_num);
    //nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    while(isNotBack){
        curs_set(1); // cursor visible

        printSignU1(window1);
        memset(id,0x00,sizeof(id)/sizeof(char));
        memset(pw,0x00,sizeof(pw)/sizeof(char));
        memset(usr,0x00,sizeof(User));
        int isEnter=0, i=0;
        mainCursor=0;
        res=1;

        wmove(window1,8,36);
        //echo();
        while(!isEnter){
            ch=wgetch(window1);
            switch(ch){
            case 8:
            case 127:
                if(i>0) {
                    id[--i]=0;
                    wmove(window1,8,36+i);
                    wechochar(window1, ' ');
                }
                break;
            case '\n':
                isEnter=1; break;
            default:
                id[i++]=ch; break;
            }
            wmove(window1,8,36);
            for(int j=0;j<i;j++)
                wechochar(window1, id[j]);
        }

        isEnter=0;
        i=0;
        wmove(window1,10,36);
        noecho();
        while(!isEnter){
            ch=wgetch(window1);
            switch(ch){
                case 8: // Backspace
                case 127:
                    if(i>0){
                        pw[--i]=0; 
                        wmove(window1,10,36+i);
                        wechochar(window1,' ');
                        wmove(window1,10,36+i);
                    }
                    break;
                case '\n':
                    isEnter=1; break;
                default:
                    pw[i++]=ch;
                    wmove(window1,10,36+i-1);
                    wechochar(window1,'*');
                    break;
            }
        }

        strcpy(usr->id,id);
        strcpy(usr->pw,pw);

        curs_set(0); // Cursor invisible

        while(isNotBack&&res){
            ch=getch();
            switch(ch){
            case KEY_LEFT:
                if(mainCursor!=0) mainCursor--;
                break;
            case KEY_RIGHT:
                if(mainCursor!=1) mainCursor++;
                break;
            case '\n':
                switch(mainCursor){
                case 0:
                    res=insertUser(usr);
                    
                    if(res){
                        wmove(window2,5,0);
                        wprintw(window2,">>> Welcome to OTHELLO World! (Press any key...)");
                        wrefresh(window2);
                        getch();
                        free(usr);
                        //delwin(window1);
                        //delwin(window2);
                        return;
                    }else{
                        wmove(window2,5,0);
                        wprintw(window2,">>> %s has already exist in DB! (Press any key...)",id);
                        wrefresh(window2);
                        getch();
                    }
                    break;
                case 1:
                    isNotBack=0;
                    break;
                }
                break;

            default: break;
            }
            printMenu(window2,menus,mainCursor,menu_num);
        }
        
    }
    free(usr);
    //delwin(window1);
    //delwin(window2);
    return; 
}

int insertUser(User* user){
    RequestMessage req={0,};
    RequestMessage res={0,};

    req.type=REQUEST_SIGNUP;
    strcpy(req.info.id,user->id);
    strcpy(req.info.pw,user->pw);

    send(server_fd,&req,sizeof(req),0);
    do{
        recv(server_fd,&res,sizeof(res),0);
    }while(res.type!=REQUEST_OK&&res.type!=REQUEST_SIGNUP_ERROR);
    
    
    if(res.type==REQUEST_OK)
        return 1;
    else return 0;
}

int loginUser(User* user){
    RequestMessage req={0,};
    RequestMessage res={0,};

    req.type=REQUEST_SIGNIN;
    strcpy(req.info.id,user->id);
    strcpy(req.info.pw,user->pw);

    send(server_fd,&req,sizeof(req),0);
    do{
        recv(server_fd,&res,sizeof(res),0);
    }while(res.type!=REQUEST_OK&&res.type!=REQUEST_SIGNIN_ERROR);
    
    if(res.type==REQUEST_OK){
        login_win=res.info.win;
        login_lose=res.info.lose;
        return 1;
    }else {
        return 0;
    }
}

void mainAfterLogin(WINDOW* window1, WINDOW* window2){
    char* menus[]={
        "PLAY",
        "SIGN OUT",
        "WITHDRAWL"
    };
    int mainCursor=0, menu_num=3, isNotExit=1;
    int ch, gameRes=-1;

    // Print Main 
    printMainAfterLogin1(window1);

    noecho(); // No print
    cbreak(); 
    curs_set(0); // No cursor

    printMenu(window2,menus,mainCursor,menu_num);
    keypad(stdscr, TRUE);

    while(isNotExit&&isLogin){
        printMainAfterLogin1(window1);
        ch=getch();

        switch(ch){
        case KEY_LEFT:
            if(mainCursor!=0) mainCursor--;
            break;
        case KEY_RIGHT:
            if(mainCursor!=2) mainCursor++;
            break;
        case '\n':
            switch(mainCursor){
            case 0:
                gameRes=userStat(window1, window2);
                if(gameRes==1) login_win++;
                else if(gameRes==0) login_lose++;
                break;
            case 1:
                logOut(); return;
                break;
            case 2:
                Withdrawl(window1,window2);
                break;
            }
            break;
        default: break;
        }
        
        printMenu(window2,menus,mainCursor,menu_num);
    }
}

void logOut(){
    RequestMessage msg={0,};
    msg.type=REQUEST_SIGNOUT;
    strcpy(msg.info.id,login_id);
    send(server_fd,&msg,sizeof(msg),0);
    memset(login_id,0x00,50);
    return;
}

int userStat(WINDOW* window1, WINDOW* window2){
    char* menus[]={
        "Waiting..."
    };
    char* menusMatch[]={
        "OK"
    };

    int menu_num=1, mainCursor=0;

    // Print frame
    wclear(window1);
    wclear(window2);
    wrefresh(window2);


    int ch=10;
    int game_result=0;

    RequestMessage req={0,};
    RequestMessage res={0,};

    req.type=REQUEST_WAITING;
    strcpy(req.info.id,login_id);
    req.info.win=login_win;
    req.info.lose=login_lose;

    printPlayerInfo1(window1, &res, 1);

    printMenu(window2,menus,-1,menu_num);
    send(server_fd,&req,sizeof(req),0);
    do{
        recv(server_fd,&res,sizeof(res),0);
    }while(res.type!=REQUEST_GAME&&res.type!=REQUEST_WAITING);
    
    if(res.type==REQUEST_GAME)
        printPlayerInfo1(window1, &res, 1);
    else
        printPlayerInfo1(window1, &res, 2);
    printMenu(window2,menusMatch,mainCursor,menu_num);

    while(1){
        ch=(int)getch();
        if(ch=='\n'){
            game_result=Game();
            return game_result;
        }
    }
}

void Withdrawl(WINDOW* window1, WINDOW* window2){
    char* menus[]={
        "WITHDRAWL",
        "BACK"
    };

    char pw[50];
    User* usr=(User*)malloc(sizeof(User));

    int menu_num=2, isNotBack=1, res=1, mainCursor=0;
    int ch;

    // Print frame
    wclear(window1);
    wclear(window2);
    wrefresh(window2);

    noecho(); // No print

    printMenu(window2,menus,mainCursor,menu_num);
    //nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    while(isNotBack){
        curs_set(1); // cursor visible

        printWithdrawl1(window1);
        memset(pw,0x00,sizeof(pw)/sizeof(char));
        memset(usr,0x00,sizeof(User));
        int isEnter=0, i=0;
        mainCursor=0;
        res=1;

        wmove(window1,10,36);
        noecho();
        while(!isEnter){
            ch=wgetch(window1);
            switch(ch){
                case 8: // Backspace
                case 127:
                    if(i>0){
                        pw[--i]=0; 
                        wmove(window1,10,36+i);
                        wechochar(window1,' ');
                        wmove(window1,10,36+i);
                    }
                    break;
                case '\n':
                    isEnter=1; break;
                default:
                    pw[i++]=ch; 
                    wmove(window1,10,36+i-1);
                    wechochar(window1,'*');
                    break;
            }
        }

        strcpy(usr->id,login_id);
        strcpy(usr->pw,pw);

        curs_set(0); // Cursor invisible

        while(isNotBack&&res){
            ch=getch();
            switch(ch){
            case KEY_LEFT:
                if(mainCursor!=0) mainCursor--;
                break;
            case KEY_RIGHT:
                if(mainCursor!=1) mainCursor++;
                break;
            case '\n':
                switch(mainCursor){
                case 0:
                    res=deleteUser(usr);
                    
                    if(res){
                        isLogin=0;
                        free(usr);
                        return;
                    }else{
                        wmove(window2,5,0);
                        wprintw(window2,">>> Check your PASSWORD. (Press any key...)");
                        wrefresh(window2);
                        getch();
                    }
                    break;
                case 1:
                    isNotBack=0;
                    break;
                }
                break;

            default: break;
            }
            printMenu(window2,menus,mainCursor,menu_num);
        }
        
    }
    free(usr);
    return;
}

int deleteUser(User* user){
    RequestMessage req={0,};
    RequestMessage res={0,};

    req.type=REQUEST_WITHDRAWL;
    strcpy(req.info.id,user->id);
    strcpy(req.info.pw,user->pw);

    send(server_fd, &req, sizeof(req), 0);
    do{
        recv(server_fd, &res, sizeof(res), 0);
    }while(res.type!=REQUEST_OK&&res.type!=REQUEST_WITHDRAWL_ERROR);

    if(res.type==REQUEST_OK)
        return 1;
    else return 0;
}

int Game(){
    WINDOW *window1;
    WINDOW *window2;

    GameInfo game={0,};

    int cursor=14;
    int ch;
    int isCursorOnGame=1, menuCursor=0;
    int myTurn=0, isRegame=0;

    RequestMessage game_res={0,};

    refresh();

    window1 = newwin(24, 56, 0, 0);
    window2 = newwin(24, 80, 0, 56);

    wbkgd(window1, COLOR_PAIR(1));
    wbkgd(window2, COLOR_PAIR(2));

    // Game start
    do{
        if(game.isFinished!=3&&game.isFinished!=0){
            if(game.isFinished==myTurn) login_win++;
            else login_lose++;
        } 

        cursor=14;
        isCursorOnGame=1;
        menuCursor=0;
        isRegame=0;

        // Server sent initialized game info.
        recv(server_fd,&game,sizeof(game),0);
        if(!strcmp(game.p1name,login_id)) myTurn=1;
        else myTurn=2;

        while(!game.isFinished){
            printGameBoard(window1,game.gameBoard,cursor);
            printGameMenu(window2,-1,&game);
            if(myTurn==game.turn){
                int isMarked=0;
                while(!isMarked){
                    ch=(int)getch();
                    if(isCursorOnGame){
                        switch(ch){
                            case KEY_UP:
                                if(cursor/6!=0) cursor-=6; break;
                            case KEY_DOWN:
                                if(cursor/6!=5) cursor+=6; break;
                            case KEY_RIGHT:
                                if(cursor%6!=5) cursor++; break;
                            case KEY_LEFT:
                                if(cursor%6!=0) cursor--; break;
                            case 'n':
                                isCursorOnGame=0;
                                menuCursor=0;
                                break;
                            case 'r':
                                isCursorOnGame=0;
                                menuCursor=1;
                                break;
                            case 'x':
                                isCursorOnGame=0; 
                                menuCursor=2;
                                break;
                            case '\n':
                                isMarked=setMark(cursor, myTurn);
                                if(!isMarked) continue; // If invalid position, enter again
                                break;
                        }
                        printGameBoard(window1, game.gameBoard, cursor);
                        if(isCursorOnGame)
                            printGameMenu(window2,-1,&game);
                        else printGameMenu(window2,menuCursor,&game);
                    }else{
                        switch(ch){
                        case '\n':
                            switch(menuCursor){
                            case 0:
                                setMark(-1, myTurn);
                                isMarked=1;
                                break;
                            case 1:
                                game_res.type=REQUEST_REGAME;
                                send(server_fd,&game_res,sizeof(game_res),0);
                                isMarked=1;
                                break;
                            case 2:
                                game_res.type=REQUEST_GAME_EXIT;
                                send(server_fd, &game_res, sizeof(game_res), 0);
                                recv(server_fd, &game_res, sizeof(game_res), 0);
                                return 0;
                            }
                        case 'g':
                            isCursorOnGame=1; break;
                        case KEY_UP:
                            if(menuCursor!=0) menuCursor--; break;
                        case KEY_DOWN:
                            if(menuCursor!=2) menuCursor++; break;
                        default: break;
                        }
                        if(!isCursorOnGame)
                            printGameMenu(window2,menuCursor,&game);
                        else printGameMenu(window2,-1,&game);
                    }
                }
            }
            recv(server_fd,&game,sizeof(game),0);
        }

        printGameBoard(window1, game.gameBoard, -1);
        menuCursor=1;
        if(game.isFinished!=myTurn||(game.isFinished==3&&myTurn==1)){ // If lost game or draw&&p1
            int reqNotSent=1;
            while(reqNotSent){
                printGameMenu(window2, menuCursor, &game);
                ch=(int)getch();
                switch(ch){
                case KEY_UP:
                    if(menuCursor==2) menuCursor--; break;
                case KEY_DOWN:
                    if(menuCursor==1) menuCursor++; break;
                case '\n':
                    if(menuCursor==2){
                        game_res.type=REQUEST_GAME_EXIT;
                    }
                    else{
                        game_res.type=REQUEST_GAME;
                        isRegame=1;
                    }
                    send(server_fd,&game_res,sizeof(game_res),0);
                    recv(server_fd,&game_res,sizeof(game_res),0);
                    reqNotSent=0;
                    break;
                }
            }
        }else{ // If won game or draw&&p2
            printGameMenu(window2, -1, &game);
            if(game_res.type!=REQUEST_GAME_EXIT) // If another player exitted during game
                recv(server_fd,&game_res,sizeof(game_res),0);
            else{ // If opponent exited during game
                delwin(window1);
                delwin(window2);
                return 1;
            }
            if(game_res.type==REQUEST_GAME) isRegame=1;
            else if(game_res.type==REQUEST_GAME_EXIT) return 1;
        }

        
    }while(isRegame); // Loop if regame selected


    delwin(window1);
    delwin(window2);
    if(game.isFinished==3) return 3;
    return game.isFinished==myTurn? 1 : 0;
}

int setMark(int cursor, int myTurn){
    RequestMessage req={0,};
    GameInfo res={0,};

    req.type=REQUEST_GAME;
    if(cursor==-1){
        req.game.x=-1;
        req.game.y=-1;
    }else{
        req.game.x=cursor%6;
        req.game.y=cursor/6;
    }

    send(server_fd, &req, sizeof(req), 0);
    recv(server_fd, &res, sizeof(res), 0);

    if(res.turn==myTurn) return 0;
    else return 1;
}

void printGameMenu(WINDOW* window, int menuCursor, GameInfo* game){
    wclear(window);

    int x=6, y=16;

    wmove(window, 9, 2);
    if(game->turn==1){
        wattron(window, A_REVERSE);
        wprintw(window, "%s(O) : %d", game->p1name, game->p1count);
        wattroff(window, A_REVERSE);
        wmove(window, 10, 2);
        wprintw(window, "%s(X) : %d", game->p2name, game->p2count);
    }else{
        wprintw(window, "%s(O) : %d", game->p1name, game->p1count);
        wmove(window, 10, 2);
        wattron(window, A_REVERSE);
        wprintw(window, "%s(X) : %d", game->p2name, game->p2count);
        wattroff(window, A_REVERSE);
    }

    if(game->isFinished==0){
        // Menu of not ended game
        wmove(window, 13, 6);
        if(menuCursor==0) wattron(window, A_REVERSE);
        wattron(window, A_UNDERLINE);
        wprintw(window, "N");
        wattroff(window, A_UNDERLINE);
        wprintw(window, "EXT TURN");
        if(menuCursor==0) wattroff(window, A_REVERSE);
    }else{
        wmove(window, 13, 6);
        if(game->isFinished==3) wprintw(window,"DRAW");
        else wprintw(window,"%s Win!",game->isFinished==1?game->p1name:game->p2name);
    }

    wmove(window, 15, 6);
        if(menuCursor==1) wattron(window, A_REVERSE);
        wattron(window, A_UNDERLINE);
        wprintw(window, "R");
        wattroff(window, A_UNDERLINE);
        wprintw(window, "EGAME");
        if(menuCursor==1) wattroff(window, A_REVERSE);

        wmove(window, 17, 6);
        if(menuCursor==2) wattron(window, A_REVERSE);
        wprintw(window, "E");
        wattron(window, A_UNDERLINE);
        wprintw(window, "X");
        wattroff(window, A_UNDERLINE);
        wprintw(window, "IT");
        if(menuCursor==2) wattroff(window, A_REVERSE);

        wrefresh(window);
    
}

void printGameBoard(WINDOW* window, char board[6][6], int cursor){
    wclear(window);

    //wmove(window,6,16);
    for(int i=0;i<6;i++){
        //wmove(window,6+2*i,16);
        for(int k=0;k<25;k++){
            wmove(window,6+i*2,16+k);
            if(k%4==0) wechochar(window,'+');
            else wechochar(window,'-');
        }
        for(int j=0;j<6;j++){
            wmove(window,6+i*2+1,16+j*4);
            wechochar(window,'|');
            wmove(window,6+i*2+1,16+j*4+1);
            if(cursor==i*6+j){
                wattron(window, A_REVERSE);
                wprintw(window," %c ",board[i][j]);
                wattroff(window, A_REVERSE);
            }
            else wprintw(window," %c ",board[i][j]);
        }
        wmove(window,6+i*2+1,40);
        wechochar(window,'|');
    }
    for(int k=0;k<25;k++){
        wmove(window,18,16+k);
        if(k%4==0) wechochar(window,'+');
        else wechochar(window,'-');
    }

    wrefresh(window);
}


/*
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

int checkValid(int x, int y, char m, int direction, char gameBoard[6][6]){
	// (x, y) is opponent's position near by input

	// UP=1 DOWN=2 LEFT=4 RIGHT=8
	// UPLEFT=5, UPRIGHT=9, DOWNLEFT=6, DOWNRIGHT=10 
	char op = m=='O' ? 'X' : 'O';
	moveToDirection(&x,&y,direction); // Change x and y by direction
    if(x>5||x<0||y>5||y<0) return 0;

	if(gameBoard[x][y]==m) return 1;
	else if(gameBoard[x][y]==op) return checkValid(x,y,m,direction,gameBoard);
	else return 0; 
}

int checkGameOver(char m, char gameBoard[6][6]){
	char op=m=='O'?'X':'O';
	op=m==0?' ':op;

	if((p1count+p2count)==36) return 0; // If game board is full
	else if(p1count==0||p2count==0) return 0; // If one side lost all markers
	else{
		for(int i=0;i<6;i++){
			for(int j=0;j<6;j++){
				if(gameBoard[i][j]==' '||gameBoard[i][j]==op) continue;
				else{
					for(int d=1;d<11;d++){
						if(d==3||d==7) continue;
						if(checkValid(i,j,gameBoard[i][j],d,gameBoard)) return 0; // If there's any valid position
					}
				}
			}
		}
	}
	return 1;
}
*/