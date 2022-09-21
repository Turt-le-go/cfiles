#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <curses.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>


int getNumberOfFiles(const char*);
void getFiles();
int comparator(const void* str1, const void* str2);
int getStatString(const char* filename, char* buf);
int isDir(char* filename);
int openFile(char* filename);
void init_curses();
void logger(const char* tag, const char* message);

const char* LOG_FILE = "/tmp/cf.log";
const char* LOG_FMT = "%s [%s]: %s;\n";

const char* DEFAULT_OPEN_APP = "xdg-open";
const char* OPEN_FMT = "%s %s";

const char filetypes[] = {'?','p','c','?','d','?','b','?','-','?','l','?','s'};
const int sizeStatBuf = 10;

int maxItmesOnScreen;
int numItmesOnScreen;
int numOfItemsInMenu;
char **menuItems;

int main(int argc, char *argv[]){
	
	setlocale(LC_ALL, "");
	init_curses();

	//win
	int termHight, termWidth;
	getmaxyx(stdscr, termHight, termWidth);
	int winHeight = termHight-2;
	int winWidth = termWidth;
	maxItmesOnScreen =  winHeight - 5;
	WINDOW *win = newwin(winHeight, winWidth, 0, 0);
	WINDOW *statWin = newwin(2, winWidth, termHight-2, 0);
	keypad(win, true);
	refresh();
	
	//get files
	if (argc == 2){
		char* dir = argv[1];
		if (chdir(dir) == -1){
			char* err = "Can't open %s directory";
			int size = snprintf(NULL, 0, err, dir);
			char errString[size+1];
			snprintf(errString, sizeof(errString), err, dir);
			logger("ERROR", errString);
		}
	}
	getFiles();
	int selectedItemNum = 0;

	int ch;
	while(ch != 'q'){
		switch (ch){
			case 'j':
				if (selectedItemNum < numOfItemsInMenu-1){
					selectedItemNum +=1;
				}
				break;
			case 'k':
				if (selectedItemNum > 0){
					selectedItemNum -= 1;
				}
				break;
			case 'l':
				if (isDir(menuItems[selectedItemNum])){
					chdir(menuItems[selectedItemNum]);
					getFiles();
					selectedItemNum = 0;
				} else {
					openFile(menuItems[selectedItemNum]);
				}
				break;
			case 'h':
				chdir("..");
				getFiles();
				selectedItemNum = 0;
				break;
		}

		box(win, 0, 0);

		//printMenu
		for(int i = 0; i <= numItmesOnScreen; i++){
			if (selectedItemNum > numItmesOnScreen){
				if ( i == numItmesOnScreen){
					wattron(win,COLOR_PAIR(1));
				}
				mvwprintw(win, 2+i, 3, "%s", menuItems[i + (selectedItemNum-numItmesOnScreen)]);
			} else {
				if ( i == selectedItemNum){
					wattron(win,COLOR_PAIR(1));
				}
				mvwprintw(win, 2+i, 3, "%s", menuItems[i]);
			}
			wattroff(win,COLOR_PAIR(1));
		}
		wrefresh(win);

		//printStat
		char fileStatString[sizeStatBuf];
		getStatString(menuItems[selectedItemNum], fileStatString);
		mvwprintw(statWin, 0,1, "%s", fileStatString);
		wrefresh(statWin);
		ch = getch();
		wclear(statWin);
		wclear(win);
	}

	free(menuItems);
	clear();
	endwin();

	return 0;
}

void init_curses(){
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	start_color();
	init_pair(1, 0, COLOR_BLUE);
}

int getNumberOfFiles(const char* dir){
	int len = 0;
	DIR *d;
	struct dirent *info;

	d = opendir(dir);
	while ((info = readdir(d)) != NULL){
		if (!strcmp(info->d_name, ".") || !strcmp(info->d_name, "..")){
			continue;
		}
		len++;
	}
	return len;
}

void getFiles(){
	for(int i=0; i < numOfItemsInMenu; i++){
		free(menuItems[i]);
	}
	numOfItemsInMenu = getNumberOfFiles(".");
	numItmesOnScreen = (numOfItemsInMenu < maxItmesOnScreen)? numOfItemsInMenu-1: maxItmesOnScreen;
	free(menuItems);
	menuItems = malloc(sizeof(char*)*numOfItemsInMenu);

	int i = 0;
	DIR *d;
	struct dirent *info;

	d = opendir(".");
	while ((info = readdir(d)) != NULL){
		if (!strcmp(info->d_name, ".") || !strcmp(info->d_name, "..")){
			continue;
		}
		menuItems[i] = malloc(256*sizeof(char));
		memcpy(menuItems[i], info->d_name, 256);
		i++;
	}
	qsort(menuItems, numOfItemsInMenu, sizeof(char*), comparator);
}

int comparator(const void* str1, const void* str2) {
   if(strcmp(*(char**)str1, *(char**)str2) >= 0)
      return 1;
   else return 0;
}

int getStatString(const char* filename, char* buf){
	struct stat fileStat;
	if (stat(filename, &fileStat) < 0){
		//Error while getting stats
		return 0;
	}
	// drwxr-xr-x  4.0K
	int i = 0;
	buf[i++] = filetypes[(fileStat.st_mode >> 12) & 017];
	buf[i++] = (fileStat.st_mode & S_IRUSR)? 'r': '-';
	buf[i++] = (fileStat.st_mode & S_IWUSR)? 'w': '-';
	buf[i++] = (fileStat.st_mode & S_IXUSR)? 'x': '-';
	buf[i++] = (fileStat.st_mode & S_IRGRP)? 'r': '-';
	buf[i++] = (fileStat.st_mode & S_IWGRP)? 'w': '-';
	buf[i++] = (fileStat.st_mode & S_IXGRP)? 'x': '-';
	buf[i++] = (fileStat.st_mode & S_IROTH)? 'r': '-';
	buf[i++] = (fileStat.st_mode & S_IWOTH)? 'w': '-';
	buf[i++] = (fileStat.st_mode & S_IXOTH)? 'x': '-';
	buf[i++] = '\0';

	return 1;
}

int isDir(char* filename){
	struct stat fileStat;
	if (stat(filename, &fileStat) < 0){
		//Error while getting stats
		return 0;
	}
	return (((fileStat.st_mode >> 12) & 017) == 4);
}

int openFile(char* filename){
	int size = snprintf(NULL, 0, OPEN_FMT, DEFAULT_OPEN_APP, filename);
	char openString[size+1];
	snprintf(openString, sizeof(openString), OPEN_FMT, DEFAULT_OPEN_APP, filename);
	return system(openString);
}

void logger(const char* tag, const char* message) {
	time_t now;
	time(&now);
	char* timeString = ctime(&now);
	timeString[strlen(timeString)-1] = '\0';

	int size = snprintf(NULL, 0, LOG_FMT, timeString, tag, message);
	char logString[size+1];
	snprintf(logString, sizeof(logString), LOG_FMT, timeString, tag, message);

	int flog;
	if ((flog = open(LOG_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0){
		perror("Error opening log file");
		return;
	}
	write(flog, logString, sizeof(logString));
	close(flog);
}
