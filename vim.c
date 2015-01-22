#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "stat.h"
#include "fs.h"

#define CONSOLE_HEIGHT 25
#define CONSOLE_WIDTH 80
#define MAX_LINE 100
#define MAX_LENGTH 80

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

#define KEY_HOME        0xE0
#define KEY_END         0xE1
#define KEY_UP          0xE2
#define KEY_DN          0xE3
#define KEY_LF          0xE4
#define KEY_RT          0xE5
#define KEY_PGUP        0xE6
#define KEY_PGDN        0xE7
#define KEY_INS         0xE8
#define KEY_DEL         0xE9
#define KEY_ESC         0x1B

//Vim variables
char* membuf[1000];

int saved = 1;
int top = 0, bottom = 24, left = 0, right = 80;
int offset = 0; //offset of every line(for line number)
int startline = 0;
int cursorX, cursorY;
int num_line = 0; //Number of lines
int mode = 0; //0,Control 1,Insert 2,Replace
char filename[100]; //Default filename
char textbuf[MAX_LINE][MAX_LENGTH + 1];

char controlbuf[65];
int controlp = 0;

int 
isTextChar(char c)
{
    if (c >= ' ' && c <= '~')
        return 1;
    return 0;
}

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

void
quit()
{
    init();
    setconsole(-1, 0, 0, 0, 0);
    exit();
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
            if (p == MAX_LENGTH){
                textbuf[num_line++][p] = '\0';
                p = 0;
            }
            if (buf[i] == '\n'){
                textbuf[num_line++][p] = '\0';
                p = 0;
            }
            else{
                textbuf[num_line][p++] = buf[i];
            }
        }
    }
    close(fd);
    return 0;
}

int
saveFile(char* filename)
{
    int fd = open(filename, O_WRONLY | O_CREATE);
    if (fd < 0)
        return fd;
    int i;
    for (i = 0; i < num_line; ++i)
    {
        write(fd, textbuf[i], strlen(textbuf[i]));
        write(fd, "\n", 1);
    }
    close(fd);
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
showMessage(char* msg)
{
    int line = 24;
    int i;
    int l = strlen(msg);
    for (i = 0; i < 65; ++i)
        setconsole(coor(line, i), 0, BLACK, -1, 2);
    if (l > 60)
        l = 60;
    for (i = 0; i < l; ++i)
        setconsole(coor(line, i), msg[i], WHITE_ON_BLACK, -1, 2);
}

void
showCoor()
{
    int line = 24;
    int i, p = 0, l;
    for (i = 65; i < CONSOLE_WIDTH; ++i)
        setconsole(coor(line, i), 0, WHITE, -1, 2);
    char tmp[20];
    char* digit;
    digit = intToString(cursorX + 1 + startline);
    for (i = 0; i < strlen(digit); ++i)
        tmp[p++] = digit[i];
    tmp[p++] = 'R';
    tmp[p++] = ',';
    tmp[p++] = ' ';
    digit = intToString(cursorY + 1 - left);
    for (i = 0; i < strlen(digit); ++i)
        tmp[p++] = digit[i];
    tmp[p++] = 'C';
    tmp[p++] = 0;
    l = strlen(tmp);
    for (i = 0; i < l; ++i)
        setconsole(coor(line, i + 65), tmp[i], WHITE, -1, 2);
}

void
showText()
{
    int i, j, l;
    //char *digit;
    int n = num_line - startline;
    if (n > 24)
        n = 24;
    for (i = 0; i < n; ++i)
        for (j = 0; j < CONSOLE_WIDTH; ++j)
            setconsole(coor(i, j), 0, BLACK, -1, 2);
    for (i = startline; i < startline + n; ++i){
        /*
        digit = intToString(i);
        for (j = 0; j < strlen(digit); ++j)
            setconsole(coor(i, j), digit[j], GRAY, -1, 2);*/
        l = strlen(textbuf[i]);
        for (j = 0; j < l; ++j)
            if (textbuf[i][j] != '\n')
                setconsole(coor(i - startline, j + offset), textbuf[i][j], WHITE_ON_BLACK, -1, 2);
    }
    showCoor();
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
    if (cursorX < top){
        if (startline > 0){
            startline--;
            showText();
        }
        cursorX = top;
    }
    if (cursorX > bottom - 1){
        if (num_line - startline > 24){
            startline++;
            showText();
        }
        cursorX = bottom - 1;
    }
    if (startline + cursorX > num_line - 1)
        cursorX = num_line - startline - 1;
    if (cursorY < left)
        cursorY = left;
    int l = strlen(textbuf[startline + cursorX]);
    if (cursorY > l)
        cursorY = l;
    setconsole(-1, 0, 0, coor(cursorX, cursorY), 2);
    showCoor();
}

//Cursor control 
int
runCursorCtrl(char c)
{
    int t = c;
    t &= 0xff;
    switch (t){
        case KEY_UP:
            moveCursor(-1, 0);
            return 1;
        case KEY_DN:
            moveCursor(1, 0);
            return 1;
        case KEY_LF:
            moveCursor(0, -1);
            return 1;
        case KEY_RT:
            moveCursor(0, 1);
            return 1;
        case KEY_HOME:
            moveCursor(0, -10000);
            return 1;
        case KEY_END:
            moveCursor(0, 10000);
            return 1;
        case KEY_INS:
            mode = 1;
            showMessage("-- INSERT --");
            return 1;
    }
    return 0;
}

void
delete()
{
    int i, textX = cursorX + startline;
    char c;
    if (cursorY == 0){
        return;
    }
    for (i = cursorY - 1; i < MAX_LENGTH; i++){
        textbuf[textX][i] = textbuf[textX][i + 1];
    }
    for (i = cursorY - 1; i < MAX_LENGTH; i++){
        c = textbuf[textX][i];
        if (c == '\n'){
            c = 0;
        }
        setconsole(coor(cursorX, i), c, GRAY, -1, 2);    
    }
    moveCursor(0, -1);
}

void
runTextInput(char c)
{
    int i, j, textX = cursorX + startline;
    char tail, ch;
    switch (c){
        case '\n':
            for (i = MAX_LINE - 1; i > textX + 1; i--){
                if (!textbuf[i][0]){
                    continue;
                }           
                for (j = 0; textbuf[i][j] || textbuf[i - 1][j]; j++){
                    textbuf[i][j] = textbuf[i - 1][j];                
                }
            }
            memset(textbuf[textX+1], 0, MAX_LENGTH);
            if (textX < MAX_LINE - 1){
                strcpy(textbuf[textX+1], textbuf[textX] + cursorY);
            }
            memset(textbuf[textX] + cursorY, 0, MAX_LENGTH - cursorY);
            textbuf[textX][cursorY] = '\n';
            for (i = CONSOLE_HEIGHT - 2; i >= cursorX; i--){
                if (!textbuf[i + startline][0]){
                    continue;
                }           
                for (j = 0; j < CONSOLE_WIDTH; j++){
                    ch = textbuf[i + startline][j];
                    if (ch == '\n'){
                        ch = 0;
                    }
                    setconsole(coor(i, j), ch, GRAY, -1, 2);              
                }
            }
            if (cursorX < CONSOLE_HEIGHT-1){
                setconsole(-1, 0, 0, coor(cursorX + 1, 0), 2);
                cursorX += 1;
            }else{
                setconsole(-1, 0, 0, coor(cursorX, 0), 2);
            }
            cursorY = 0;
            break;  
        case 8: //back
            delete();
            break;
        default:  
            tail = textbuf[textX][CONSOLE_WIDTH -1];
            for (i = CONSOLE_WIDTH -1; i > cursorY; i--){
                if (textbuf[textX][i - 1] == 0){
                    continue;
                } 
                if (textbuf[textX][i - 1] != '\n'){
                    setconsole(coor(cursorX, i), textbuf[textX][i - 1], GRAY, -1, 2);
                }
                textbuf[textX][i] = textbuf[textX][i - 1];
            }
            textbuf[textX][cursorY] = c;
            setconsole(coor(cursorX, cursorY), c, GRAY, -1, 2);
            moveCursor(0, 1);
            if (tail != 0){
            //drop
            }
    }
}
        
    
        
    
    


/*
command list
i Insert at cursor position
o Open a new line
a Insert at next position
A Insert at end of line
r Replace character
R Replace mode
x Delete character
h move left
j move up
k move down
l move right
y copy a line
p paste a line
d delete a line
:q quit
:w save
:wq save & quit
:q! quit without save
*/
void
runCommand()
{
    if (controlbuf[0] != ':')
        return ;
    if (controlbuf[1] == 'w' && (controlbuf[2] == ' ' || controlbuf[2] == 0 || controlbuf[2] == 'q')){
        if (controlbuf[2] == ' '){
            int i, l = strlen(controlbuf);
            for (i = 3; i < l; ++i)
                filename[i - 3] = controlbuf[i];
            filename[i - 3] = 0;
        }
        if (saveFile(filename) != 0){
            showMessage("Save failed!");
            return ;
        }
        showMessage("Save succeed!");
        if (controlbuf[2] == 'q')
            quit();
        return ;
    }
    if (controlbuf[1] == 'q'){
        if (controlbuf[2] == '!' && controlbuf[3] == 0)
            quit();
        if (controlbuf[2] == 0){
            if (saved == 0){
                showMessage("File isn't saved. Force quit by \'q!\'");
                return;
            }
            else
                quit();
        }
    }
    showMessage("Invalid command");
}

void
runControl()
{
    switch (controlbuf[0]){
        case 'i':
            mode = 1;
            showMessage("-- INSERT --");
            break;
        case 'o':
            break;
        case 'a':
            break;
        case 'A':
            break;
        case 'r':
            break;
        case 'R':
            mode = 2;
            showMessage("-- REPLACE --");
            break;
        case ':':
            showMessage(controlbuf);
            break;
        default:
            controlp = 0;
            controlbuf[0] = 0;
            break;
    }
}

void
parseInput(char c)
{
    if (c == KEY_ESC){
        mode = 0;
        controlp = 0;
        controlbuf[0] = 0;
        showMessage("");
        return ;
    }
    if (runCursorCtrl(c) != 0){
        controlp = 0;
        return ;
    }
    if (!isTextChar(c) && c != 8 && c != 10 && c != 13)
        return ;
    if (mode == 0){
        showMessage("");
        if (controlp >= 60)
            return ;
        if (c == ':'){
            controlp = 0;
            controlbuf[0] = 0;
        }
        if (c == 10){
            runCommand();
            controlp = 0;
            controlbuf[0] = 0;
            return ;
        }
        if (c == 8)
            controlp--;
        else
            controlbuf[controlp++] = c;
        controlbuf[controlp] = 0;
        runControl();
    }
    else{
        saved = 0;
        runTextInput(c);
    }
}

int
main(int argc, char *argv[])
{
    char buf[1];
    int i;

    if (argc > 1){
        if (openFile(argv[1]) < 0)
        {
            printf(1, "vim: cannot open %s\n", argv[1]);
            exit();
        }
        for (i = 0; i < strlen(argv[1]); ++i)
            filename[i] = argv[1][i];
        filename[i] = 0;
    }
    else{
        printf(1, "vim: please input filename\n");
        exit();
    }
    cursorX = cursorY = 0;
    init();
    showText();
    if (argc > 1){
        showMessage(getFileInfo());
    }
    int n;
    while ((n = read(0, buf, sizeof(buf))) > 0){
        if (buf[0] != 0)
            parseInput(buf[0]);
    }
  return 0;
}

