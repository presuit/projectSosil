#include <stdio.h>
#include <termio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFSIZE 1024

int getch(void){
	int ch;
	struct termios buf, save;
	tcgetattr(0, &save);
	buf = save;
	buf.c_lflag &= ~(ICANON | ECHO);
	buf.c_cc[VMIN] = 1;
	buf.c_cc[VTIME] = 0;
	tcsetattr(0, TCSAFLUSH, &buf);
	ch = getchar();
	tcsetattr(0, TCSAFLUSH, &save);
	return ch;
}

char** strParse(char buf[]){	
	char** strToken = (char**)malloc(sizeof(char*) * BUFSIZE);
	char tmpBuf[BUFSIZE];
	int tmpSize = 0;
	int tokenSize = 0;
	memset(tmpBuf, 0, BUFSIZE);

	//tokenizer setting
	for(int i = 0; i < BUFSIZE; i++){
		strToken[i] = (char*)malloc(sizeof(char)* BUFSIZE);
		memset(strToken[i], 0, BUFSIZE);
	}

	//parsing
	for(int i = 0; i < strlen(buf); i++){
		if(buf[i] != ' ' ){
			tmpBuf[tmpSize++] = buf[i];
		}
		if(buf[i] == ' ' || i == strlen(buf) - 1){
			strcpy(strToken[tokenSize++], tmpBuf);
			memset(tmpBuf, 0, BUFSIZE);
			tmpSize = 0;
		}
	}
	
	for(int i = tokenSize; i < BUFSIZE; i++){
		free(strToken[i]);
		strToken[i] = NULL;
	}

	return strToken;
}

char** freeStrToken(char** strToken){
	for(int i = 0; i < BUFSIZE; i++){
		if(strToken[i] == NULL){
			break;
		}
		free(strToken[i]);
	}
	
	strToken = NULL;

	return strToken;
}

void forkPipe(char ** cmdStr, char ** pipeStr, int flag, int fileFd){
	pid_t childPid;
	int fd[2];
//	int flagPipe = 0, splitStrSize = 0;
//	char ** splitStr = (char**)malloc(sizeof(char*)* BUFSIZE);
/*
	for(int i = 0; i < BUFSIZE; i++){
		splitStr[i] = (char*)malloc(sizeof(char) * BUFSIZE);
		memset(splitStr[i], 0, BUFSIZE);
	}
*/
	pipe(fd);
	childPid = fork();

	if(childPid == -1){
		printf("error in forkPipe func\n");
		return;
	}
	/*
	for(int i = 0; i < BUFSIZE; i++){
		if(cmdStr[i] == NULL){
			break;
		}
		printf("<#cmdStr[%d]> %s\n", i, cmdStr[i]);
	}
	
	for(int i = 0; i < BUFSIZE; i++){
		if(pipeStr[i] == NULL){
			break;
		}
		printf("<#pipeStr[%d]> %s\n", i, pipeStr[i]);
	}
*/

	if(childPid == 0){
		//child
		dup2(fd[1], STDOUT_FILENO);
		close(fd[0]);
		close(fd[1]);
		execvp(cmdStr[0], &cmdStr[0]);
	}
	else {
		//parent
		wait(NULL);
/*
		for(int i = 0; i < BUFSIZE; i++){
			if(pipeStr[i] == NULL){
				break;
			}
			if(!strcmp(pipeStr[i], "|")){
				flagPipe = i + 1;
			}
		}
		if(flagPipe > 0){
			for(int i = flagPipe; i < BUFSIZE; i++){
				if(pipeStr[i] == NULL){
					break;
				}
				strcpy(splitStr[splitStrSize++], pipeStr[i]);
			}
			for(int i = splitStrSize; i < BUFSIZE; i++){
				free(splitStr[i]);
				splitStr[i] = NULL;
			}
			for(int i = flagPipe - 1; i < BUFSIZE; i++){
				free(pipeStr[i]);
				pipeStr[i] = NULL;
			}
		}
*/
		dup2(fd[0], STDIN_FILENO);
		close(fd[0]);
		close(fd[1]);
		if(flag == 1){
			dup2(fileFd, STDOUT_FILENO);
		}
		if(flag == 2){
			dup2(fileFd, STDIN_FILENO);
		}
		execvp(pipeStr[0], &pipeStr[0]);
	}
}

void exeCommand(char** strToken)
{
	pid_t childPid;
	int fd;
	int pipeFd[2];
	int pipeSize = 0, pipeOutSize = 0;
	int flagInput = 0, flagOutput = 0, flagPipe = 0, flagPipeOut = 0;
	char bufInput[BUFSIZE];
	char ** cmdStr = (char**)malloc(sizeof(char*) * BUFSIZE);
	char ** pipeStr = (char**)malloc(sizeof(char*) * BUFSIZE);
	char ** pipeStrOut = (char**)malloc(sizeof(char*) * BUFSIZE);
	memset(bufInput, 0, BUFSIZE);

	for(int i = 0; i < BUFSIZE; i++){
		cmdStr[i] = (char*)malloc(sizeof(char) * BUFSIZE);
		pipeStr[i] = (char*)malloc(sizeof(char) * BUFSIZE);
		pipeStrOut[i] = (char*)malloc(sizeof(char) * BUFSIZE);
		memset(cmdStr[i], 0, BUFSIZE);
		memset(pipeStr[i], 0, BUFSIZE);
		memset(pipeStrOut[i], 0, BUFSIZE);
	}

	for(int i = 0; i < BUFSIZE; i++){
		if(strToken[i] == NULL){
			break;
		}
		
		if( !strcmp(strToken[i], "<") ){
			flagInput = i + 1;
		}
		if( !strcmp(strToken[i], ">") ){
			flagOutput = i + 1;
		}
		if( !strcmp(strToken[i], "|") ){
			flagPipe = i + 1;
		}

	}

	if(flagInput != 0){
		fd = open( strToken[flagInput],  O_RDONLY );

		if(fd == -1){
			printf("wrong file format\n");
			sleep(2);
			return ;
		}

		for(int i = 0; i < flagInput -1; i++){
			strcpy(cmdStr[i], strToken[i]);
		}
		for(int i = flagInput -1; i < BUFSIZE; i++){
			free(cmdStr[i]);
			cmdStr[i] = NULL;
		}
	}

	if(flagOutput != 0){
		fd = open( strToken[flagOutput],  O_WRONLY | O_CREAT | O_TRUNC, 0644 );

		if(fd == -1){
			printf("wrong file format\n");
			sleep(2);
			return ;
		}

		for(int i = 0; i < flagOutput -1; i++){
			strcpy(cmdStr[i], strToken[i]);
		}
		for(int i = flagOutput -1; i < BUFSIZE; i++){
			free(cmdStr[i]);
			cmdStr[i] = NULL;
		}
	}

	if(flagPipe != 0){
		for(int i = 0; i < BUFSIZE; i++){
			if(cmdStr[i] == NULL){
				break;
			}
			memset(cmdStr[i], 0, BUFSIZE);
		}

		for(int i = 0; i < flagPipe - 1; i++){
			strcpy(cmdStr[i], strToken[i]);
		}
		for(int i = flagPipe - 1; i < BUFSIZE; i++){
			free(cmdStr[i]);
			cmdStr[i] = NULL;
		}
		
		for(int i = flagPipe; i < BUFSIZE; i++){
			if(strToken[i] == NULL){
				break;
			}
			strcpy(pipeStr[pipeSize++], strToken[i] );
		}
		for(int i = pipeSize; i < BUFSIZE; i++){
			free(pipeStr[i]);
			pipeStr[i] = NULL;
		}
	}

	/*
	for(int i = 0; i < BUFSIZE; i++){
		if(cmdStr[i] == NULL){
			break;
		}
		printf("<cmdStr[%d]> : %s\n", i, cmdStr[i]);
	}
	for(int i = 0; i < BUFSIZE; i++){
		if(pipeStr[i] == NULL){
			break;
		}
		printf("<pipeStr[%d]> : %s\n", i, pipeStr[i]);
	}
	*/

	//fork to exe command
		childPid = fork();
		
		if(childPid == -1){
			printf("fork error\n");
			return;
		}

		if(childPid == 0){
	//child
			if(flagPipe != 0){
				if(flagOutput != 0 || flagInput != 0){
					for(int i = 0; i < BUFSIZE; i++){
						if(pipeStr[i] == NULL){
							break;
						}
						if(!strcmp(pipeStr[i], ">")){
							flagPipeOut = i + 1;
						}
					}
					/*
					for(int i = flagPipeOut; i < BUFSIZE; i++){
						if(pipeStr[i] == NULL){
							break;
						}
						strcpy(pipeStrOut[pipeOutSize++], pipeStr[i]);
					}
					*/
					for(int i = flagPipeOut - 1; i < BUFSIZE; i++){
						free(pipeStr[i]);
						pipeStr[i] = NULL;
					}
					/*
					for(int i = pipeOutSize; i < BUFSIZE; i++){
						free(pipeStrOut[i]);
						pipeStrOut[i] = NULL;
					}
					*/
					if(flagOutput != 0){
						forkPipe(cmdStr, pipeStr, 1, fd);
					}
					else if(flagInput != 0){
						forkPipe(cmdStr, pipeStr, 2, fd);
					}
					
				}
				else {
					forkPipe(cmdStr, pipeStr, 0, 0);
				}
			}
			else if(flagInput != 0){
				dup2( fd, STDIN_FILENO );
				execvp(cmdStr[0], &cmdStr[0]);
			}
			else if(flagOutput != 0){
				dup2( fd, STDOUT_FILENO );
				execvp(cmdStr[0], &cmdStr[0]);
			}
			else {
				execvp(strToken[0], &strToken[0]);		
			}
		}
		else {
	//parent
			wait(NULL);
		}
}

char** getCurDir(char ** buf){
	DIR *dir;
	struct dirent *ent;
	char ** _buf = (char**)malloc(sizeof(char*) * BUFSIZE);
	int size = 0;

	for(int i = 0; i < BUFSIZE; i++){
		_buf[i] = (char*)malloc(sizeof(char) * BUFSIZE);
		memset(_buf[i], 0, BUFSIZE);
	}

	dir = opendir("./");

	if(dir == NULL){
		printf("directory open fail\n");
		exit(1);
	}

	while( (ent = readdir(dir)) != NULL ){
		strcpy( _buf[size++], ent->d_name );
	}
	closedir(dir);
	
	for(int i = size; i < BUFSIZE; i++){
		free(_buf[i]);
		_buf[i] = NULL;
	}

	buf = _buf;

	return buf;
	
}

void getArgBuf(char buf[], int* count, char argBuf[]){
	int i = *count - 1;
	int argSize = 0;
	
	while( buf[i] != ' ' ){
		--i;
	}
	++i;
	while(1){
		argBuf[argSize++] = buf[i++];
		if( i >= strlen(buf)){
			break;
		}
		
	}
}

void deleteUi(char buf[], int *count){
	if(*count > 0){
		buf[*count - 1] = '\0';
		*count = *count - 1;
	}
}

void exeAutoTab(char buf[], int* count){
	char ** ls = getCurDir(ls);
	char ** ovl = (char**)malloc(sizeof(char*) * BUFSIZE);
	char * ptrLs;
	char argBuf[BUFSIZE];
	int flag = 0, flagOvl = 0;
	int ovlSize = 0;
	int idxOvl = 0;
	int flagW = 0;
	char ovlElm;

	memset(argBuf, 0, BUFSIZE);
	getArgBuf(buf, count, argBuf);

	for(int i = 0; i < BUFSIZE; i++){
		ovl[i] = (char*)malloc(sizeof(char) * BUFSIZE);
		memset(ovl[i], 0, BUFSIZE);
	}


	for(int i = 0; i < BUFSIZE; i++){
		if(ls[i] == NULL){
			break;
		}
		
		if(ls[i][0] == argBuf[0]){
			flag++;
			strcpy(ovl[ovlSize++], ls[i]);
		}
	}

	for(int i = ovlSize; i < BUFSIZE; i++){
		free(ovl[i]);
		ovl[i] = NULL;
	}

	ovlSize = 0;

	if(ovl[1] != NULL){
		for(int i = 0; i < strlen(argBuf); i++){
			deleteUi(buf, count);
		}
		while(1){
			if(ovl[0] == NULL){
				break;
			}

			ovlElm = ovl[0][ovlSize];
			for(int i = 0; i < BUFSIZE; i++){
				if(ovl[i] == NULL){
					break;
				}
				idxOvl++;
				if(ovl[i][ovlSize] == ovlElm){
					flagOvl++;
				}
			}
			if(flagOvl >= 2){
				buf[*count] = ovl[0][ovlSize];
				*count = *count + 1;
			}
			ovlSize++;
			flagOvl = 0;
			idxOvl = 0;
			for(int i = 0; i < BUFSIZE; i++){
				if(ovl[i] == NULL){
					break;
				}
				if(ovl[i][ovlSize] == '\0'){
					flagW = 1;
					break;
				}
			}
			
			if(flagW == 1){
				break;
			}
		}
	}
	else {
		for(int i = 0; i < strlen(argBuf); i++){
			deleteUi(buf, count);
		}	
		strcpy(buf + *count, ovl[0]);
		*count = *count + strlen(ovl[0]);
	}
	
}

void blockSignal(){
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);

	sigprocmask(SIG_BLOCK, &mask, NULL);
}

void * _exeBack(void * arg){
	char ** str = (char**)arg;
	exeCommand(str);
	return NULL;
}

void exeBack(char** strToken){
	char **str = (char**)malloc(sizeof(char*) * BUFSIZE);
	int size = 0;
	pthread_t t;

	for(int i = 0; i < BUFSIZE; i++){
		str[i] = (char*)malloc(sizeof(char) * BUFSIZE);
		memset(str[i], 0, BUFSIZE);
	}
	for(int i = 0; i < BUFSIZE; i++){
		if(strToken[i] == NULL){
			break;
		}
		if(!strcmp(strToken[i], "&")){
			break;
		}
		strcpy(str[size++], strToken[i]);
	}
	for(int i = size; i < BUFSIZE; i++){
		free(str[i]);
		str[i] = NULL;
	}
	
	pthread_create(&t, NULL, _exeBack, str);
	
}

int main(){
	int i;
	int count = 0;
	char buf[BUFSIZE];
	char** strToken = NULL;
	int childPid, flagBack;
	memset(buf, 0, BUFSIZE);
	
	blockSignal();

	while(1){
		printf("User Shell >> ");
		while(1){
			i = getch();

			if(count >= BUFSIZE){
				break;
			}
			if(i == '\n'){
				system("clear");
				if(!strcmp(buf, "q"))
					exit(0);
				strToken = strParse(buf);
//test
				printf("\n==================\n");
				for(int i = 0; i < BUFSIZE; i++ ){
					if(strToken[i] == NULL){
						break;
					}
					else {
						printf("[%d]th : %s\n", i, strToken[i]);
					}
				}
				printf("======================\n");
				
//end test
				for(int i = 0; i < BUFSIZE; i++){
					if(strToken[i] == NULL){
						break;
					}
					if(!strcmp(strToken[i], "&")){
						flagBack = 1;
					}
				}
				if(flagBack == 1){
					//exe background
					exeBack(strToken);
				}
				else {
					//exe command
					exeCommand(strToken);
				}
				memset(buf, 0, BUFSIZE);
				count = 0;
				strToken = freeStrToken(strToken);
				break;
			}
			else if(i == 9){
				exeAutoTab(buf, &count);
			}
			else if(i == 127){
				deleteUi(buf, &count);
			}
			else {
				buf[count] = (char)i;
				count++;
			}
			system("clear");
			printf("User Shell >> ");
			printf("%s", buf);
		}
	}

	return 0;
}
