#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "stat.h"
#include "fs.h"

#define CONSOLE_HEIGHT 25
#define CONSOLE_WIDTH 80
#define MAX_LINE 20
#define MAX_LENGTH 80
#define OFFSET 4

#define BLACK 0x0000
#define DARKBLUE 0x0100
#define DARKGREEN 0x0200
#define DARKCYANBLUE 0x0300
#define DARKRED 0x0400
#define DARKPINK 0x0500
#define DARKYELLOW 0x0600
#define GRAY 0x0700
#define DARKGRAY 0x0800
#define BLUE 0x0900
#define GREEN 0x0a00
#define CYANBLUE 0x0b00
#define RED 0x0c00
#define PINK 0x0d00
#define YELLOW 0x0e00
#define WHITE 0x0f00

#define WHITE_ON_BLACK 0x0700

//Vim variables
char* membuf[1000];

int offset = 0; //offset of every line(for line number)
int startline = 0;
int cursorX, cursorY;
int num_line = 0; //Number of lines
int mode = 0; //0,Control 1,Insert 2,Replace
char* filename = 0; //Default filename
char textbuf[MAX_LINE][MAX_LENGTH + 1];

char controlbuf[10];
int controlp = 0;

int
coor(int x, int y)
{
    return x * CONSOLE_WIDTH + y;
}

void
init()
{
    int i, j;
    for (i = 0; i < CONSOLE_HEIGHT; ++i)
        for (j = 0; j < CONSOLE_WIDTH; ++j)
            setconsole(coor(i, j), 0, WHITE_ON_BLACK, -1, 2);
    setconsole(-1, 0, 0, coor(cursorX, cursorY), 2);
}

int
openFile(char* filename)
{
    int fd = open(filename, O_RDONLY | O_CREATE);
    if (fd < 0)
        return fd;
    char buf[128];
    int n, i, p = 0;
    num_line = 0;
    while ((n = read(fd, buf, 128)) > 0){
        for (i = 0; i < n; ++i){
            textbuf[num_line][p++] = buf[i];
            if (buf[i] == '\n'){
                textbuf[num_line++][p] = '\0';
                p = 0;
            }
        }
    }
    return 0;
}

char*
intToString(int k)
{
    char* result = malloc(sizeof(char) * 10);
    if (k == 0)
    {
        result[0] = '0';
        result[1] = 0;
        return result;
    }
    int l = 0, p = k;
    while (p > 0){
        l++;
        p /= 10;
    }
    result[l--] = 0;
    while (k > 0){
        result[l--] = k % 10 + '0';
        k /= 10;
    }
    return result;
}

void
showText()
{
    init();
    int i, j, l;
    //char *digit;
    int n = num_line - startline;
    if (n > 24)
        n = 24;
    for (i = startline; i < startline + n; ++i){
        /*
        digit = intToString(i);
        for (j = 0; j < strlen(digit); ++j)
            setconsole(coor(i, j), digit[j], GRAY, -1, 2);*/
        l = strlen(textbuf[i]);
        for (j = 0; j < l; ++j)
            if (textbuf[i][j] != '\n')
                setconsole(coor(i, j + offset), textbuf[i][j], WHITE_ON_BLACK, -1, 2);
    }
}

void 
showMessage(char* msg)
{
    int line = 24;
    int i, p = 0;
    int l = strlen(msg);
    if (l > 60)
        l = 60;
    for (i = 0; i < l; ++i)
        setconsole(coor(line, i), msg[i], WHITE_ON_BLACK, -1, 2);
    //position
    char tmp[20];
    char* digit;
    digit = intToString(cursorX + 1);
    for (i = 0; i < strlen(digit); ++i)
        tmp[p++] = digit[i];
    tmp[p++] = 'R';
    tmp[p++] = ',';
    tmp[p++] = ' ';
    digit = intToString(cursorY + 1);
    for (i = 0; i < strlen(digit); ++i)
        tmp[p++] = digit[i];
    tmp[p++] = 'C';
    l = strlen(tmp);
    for (i = 0; i < l; ++i)
        setconsole(coor(line, i + 65), tmp[i], WHITE, -1, 2);
}



char*
getFileInfo()
{
    char* result = malloc(sizeof(char) * 60);
    char* digit = intToString(num_line);

    int fl = strlen(filename);
    int dl = strlen(digit);

    int i, p = 1;
    result[0] = '\"';
    for (i = 0; i < fl; ++i)
        result[p++] = filename[i];
    result[p++] = '\"';
    for (i = 0; i < 3; ++i)
        result[p++] = ' ';
    for (i = 0; i < dl; ++i)
        result[p++] = digit[i];
    result[p++] = ' ';
    result[p++] = 'l';
    result[p++] = 'i';
    result[p++] = 'n';
    result[p++] = 'e';
    if (num_line != 1)
        result[p++] = 's';
    result[p++] = 0;
    return result;
}

void
moveCursor(int dx, int dy)
{
    cursorX += dx;
    cursorY += dy;
    setconsole(-1, 0, 0, coor(cursorX, cursorY), 2);
}

//Cursor control 
int
runCursorCtrl(char c)
{
    return 0;
}

void
runTextInput(char c)
{

}

void
runCommand()
{

}

void
parseInput(char c)
{
    if (runCursorCtrl(c) != 0){
        controlp = 0;
        return ;
    }
    if (mode == 0){
        if (c == ':'){
            controlp = 0;
        }
        controlbuf[controlp++] = c;
        runCommand();
    }
    else{
        runTextInput(c);
    }
}

int
main(int argc, char *argv[])
{
    char buf[1];

    if (argc > 1){
        if (openFile(argv[1]) < 0)
        {
            printf(1, "vim: cannot open %s\n", argv[1]);
            exit();
        }
        filename = argv[1];
    }
    else{
        printf(1, "vim: please input filename\n");
        exit();
    }
    cursorX = cursorY = 0;
    showText();
    if (argc > 1){
        showMessage(getFileInfo());
    }
    int n;
    while ((n = read(0, buf, sizeof(buf))) > 0){
        if (buf[0] != 0){
            write(1, buf, n);
        }
        if (buf[0] == 27)
            break;
    }
    init();
    setconsole(-1, 0, 0, 0, 0);
  exit();
  return 0;
}

