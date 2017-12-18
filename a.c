#include <stdio.h>
#include <termio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
	return strToken;
}

int main(){
	
	int i;
	int count = 0;
	char buf[BUFSIZE];
	char** strToken = NULL;
	memset(buf, 0, BUFSIZE);

	while(1){

		printf("User Shell > ");
		while(1){
			i = getch();
			printf("%c", i);

			if(count >= BUFSIZE){
				break;
			}
			if(i == '\n'){
				if(!strcmp(buf, "q"))
					exit(0);
				system("clear");
				printf("<log> : %s\n", buf);
				strToken = strParse(buf);
//test
				for(int i = 0; i < BUFSIZE; i++ ){
					if(strlen(strToken[i]) == 0){
						break;
					}
					else {
						printf("[%d]th : %s\n", i, strToken[i]);
					}
				}
				
//end test
				memset(buf, 0, BUFSIZE);
				count = 0;
				break;
			}
			else if(i == 127){
				printf("\b");
				fputs(" ", stdout);
				printf("\b");

				if(count > 0){
					buf[count - 1] = '\0';
					count--;
				}
			}
			else {
				buf[count++] = (char)i;
			}
		}
	}

	return 0;
}
