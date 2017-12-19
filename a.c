#include <stdio.h>
#include <termio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>

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

void exeCommand(char** strToken)
{
	pid_t childPid;

	//fork to exe command
		childPid = fork();
		
		if(childPid == -1){
			printf("fork error\n");
			exit(1);
		}

		if(childPid == 0){
	//child
			execvp(strToken[0], &strToken[0]);		
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
	printf("\b");
	fputs(" ", stdout);
	printf("\b");

	if(*count > 0){
		buf[*count - 1] = '\0';
		*count = *count - 1;
	}
}

void exeAutoTab(char buf[], int* count){
	char ** ls = getCurDir(ls);
	char * ptrLs, *ptrBuf;
	char argBuf[BUFSIZE];
	int flag = 0;

	memset(argBuf, 0, BUFSIZE);
	getArgBuf(buf, count, argBuf);

	for(int i = 0; i < BUFSIZE; i++){
		if(ls[i] == NULL){
			break;
		}

		if( (ptrLs = strstr(ls[i], argBuf)) != NULL ){
			for(int i = 0; i < strlen(argBuf); i++){
				deleteUi(buf, count);
			}
			printf("%s", ls[i]);
			strcpy(buf + *count, ls[i]);
			*count = *count + strlen(ls[i]);
		}
	}	

	
}

int main(){
	
	int i;
	int count = 0;
	char buf[BUFSIZE];
	char** strToken = NULL;
	int childPid;
	memset(buf, 0, BUFSIZE);

	while(1){

		printf("User Shell >> ");
		while(1){
			i = getch();

			printf("%c", i);

			if(count >= BUFSIZE){
				break;
			}
			if(i == '\n'){
				if(!strcmp(buf, "q"))
					exit(0);
				strToken = strParse(buf);
//test
				printf("==================\n");
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
			//exe command
				exeCommand(strToken);
				memset(buf, 0, BUFSIZE);
				count = 0;
				strToken = freeStrToken(strToken);
				break;
			}
			if(i == 9){
				exeAutoTab(buf, &count);
				continue;
			}
			if(i == 127){
				deleteUi(buf, &count);
				continue;
			}	
			buf[count++] = (char)i;
		}
	}

	return 0;
}
