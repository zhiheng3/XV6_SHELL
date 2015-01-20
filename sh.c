// Shell.

#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "history.h"
// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5

#define MAXARGS 10

struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

struct inputHistory hs;

void initHistory(struct inputHistory* hs);
void recordHistory(char* cmd);
void addHistory(struct inputHistory* hs,char* cmd);
void getHistory(struct inputHistory* hs);
void setHistory(struct inputHistory* hs);
char* substring(char*,char*,int,int);
char* strcat(char*,char *);


int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit();
  
  switch(cmd->type){
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit();
    exec(ecmd->argv[0], ecmd->argv);
    printf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    close(rcmd->fd);
    if(open(rcmd->file, rcmd->mode) < 0){
      printf(2, "open %s failed\n", rcmd->file);
      exit();
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    if(fork1() == 0)
      runcmd(lcmd->left);
    wait();
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p) < 0)
      panic("pipe");
    if(fork1() == 0){
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait();
    wait();
    break;
    
  case BACK:
    bcmd = (struct backcmd*)cmd;
    if(fork1() == 0)
      runcmd(bcmd->cmd);
    break;
  }
  exit();
}

int
getcmd(char *buf, int nbuf, char *currentpath)
{
  int len;
  if (currentpath[1] != 0){
    currentpath[(len = strlen(currentpath)) - 1] = 0;
    printf(2, "%s$ ", currentpath);
    currentpath[len - 1] = '/';
  }else{
    printf(2, "%s$ ", currentpath);
  }
  
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

static char*
skipelem(char *path, char *name)
{
  char *s;
  int len;

  while(*path == '/')
    path++;
  if(*path == 0)
    return 0;
  s = path;
  while(*path != '/' && *path != 0)
    path++;
  len = path - s;
  memmove(name, s, len);
  name[len] = 0;
  while(*path == '/')
    path++;
  return path;
}

static int
updatecurrentpath(char *path, char *currentpath)
{
  int i, j;
  char name[255];
  if(*path == '/'){
    strcpy(currentpath, "/");
    return 0;
  }

  while((path = skipelem(path, name)) != 0){
    if (strcmp(name, ".") == 0){
      continue;
    }
    if (strcmp(name, "..") == 0){
      for (i = 0; currentpath[i]; i++)
        ;
      i--;
      if (i == 0){
        continue;
      }
      while (1){
        currentpath[i--] = 0;
        if (currentpath[i] == '/'){
          break;
        }
      }
      continue;
    }
    for (i = 0; currentpath[i]; i++)
      ;
    for (j = 0; name[j]; j++){
      currentpath[i++] = name[j];    
    }
    currentpath[i++] = '/';
    currentpath[i] = 0;
      
  }
  
  return 0;
}



int
main(void)
{
  static char buf[100];
  int fd;
  initHistory(&hs);
  getHistory(&hs);
  passHistory(&hs);
  
  char currentpath[255];
  // Assumes three file descriptors open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }
  strcpy(currentpath, "/");
  // Read and run input commands.
  while(getcmd(buf, sizeof(buf), currentpath) >= 0){

    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      

      addHistory(&hs,buf);
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0){
        printf(2, "cannot cd %s\n", buf+3);
      }else{
        updatecurrentpath(buf+3, currentpath);    
      }
      continue;
    }
    addHistory(&hs,buf);
    
    passHistory(&hs);

    
    //setHistory(&hs);
    if(fork1() == 0)
      runcmd(parsecmd(buf));
    wait();
  }
  
  exit();
}



void
panic(char *s)
{
  printf(2, "%s\n", s);
  exit();
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

//PAGEBREAK!
// Constructors

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd*)cmd;
}
//PAGEBREAK!
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    printf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while(peek(ps, es, "&")){
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if(peek(ps, es, ";")){
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch(tok){
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    case '+':  // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    }
  }
  return cmd;
}

struct cmd*
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if(!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if(!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  if(peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|)&;")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if(argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NUL-terminate all the counted strings.
struct cmd*
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    return 0;
  
  switch(cmd->type){
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;
    
  case LIST:
    lcmd = (struct listcmd*)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}






void initHistory(struct inputHistory* hs){
  hs->len = 1;
  hs->lastusedcmd = 0;
  hs->currentcmd = 0;

  int i;
  for(i = 0; i < H_NUMBER;i++){
    memset(hs->history[i],0,H_LENGTH);
    //hs->history[i][0] = '\0';
  }
  hs->history[0][0] = '\0';
}
/*
void recordHistory(char* cmd){
  
  struct inode *ippath;
  begin_op();
  if((ippath = namei("/input_history")) == 0){
      end_op();
      return;
  }
  int n;
  ilock(ippath);
  if ((n = writei(ippath, buf, 0, strlen(cmd))) < 0){
    return;
  }
  iunlockput(ippath);



  const int bufSize = 256;
  char buf[bufSize];
  int fp = open("input_history",O_RDWR | O_CREATE);
  while(read(fp,buf,bufSize)){
    memset(buf,0,bufSize);
  }
  write(fp,cmd,strlen(cmd));
}*/

void addHistory(struct inputHistory* hs,char* cmd){
  if(*cmd =='\n'){
    return;
  }
  int cmdl = strlen(cmd);
  if(hs->len == H_NUMBER){
    int last = hs->lastusedcmd % H_NUMBER;
    strcpy(hs->history[last],cmd);
    hs->history[last][cmdl-1] = '\0';


    hs->lastusedcmd = (last+1) % H_NUMBER;
    hs->currentcmd = hs->lastusedcmd;
    hs->history[hs->lastusedcmd][0] = '\0';
  }
  else{
    strcpy(hs->history[hs->lastusedcmd],cmd);
    hs->history[hs->lastusedcmd][cmdl-1] = '\0';

    hs->history[hs->len][0] = '\0';
    hs->currentcmd = hs->len;
    hs->lastusedcmd = hs->currentcmd;
    hs->len++;
  }
}

void getHistory(struct inputHistory* hs){ 
  int fp = open("/input_history",O_RDWR | O_CREATE);  

  const int bufSize = 256;
  char buf[bufSize];
  //all '\n' position
  int posEnter[bufSize];

  //pervious line content left by '\n'
  char preleft[bufSize];

  //substring of every bufSize read
  char subbuf[bufSize];
  int i;
  int history_number = 0;
  int len;
  memset(preleft,0,bufSize);

  while((len = read(fp,buf,bufSize)) > 0){
    int numEnter = 0;
    memset(posEnter,0,bufSize);
    for(i = 0;i < strlen(buf);i++){
      if(buf[i] == '\n'){
        posEnter[numEnter] = i;
        numEnter++;
      }
    }
    
    if(numEnter >= 1){
      memset(hs->history[history_number],0,bufSize);
      memset(subbuf,0,bufSize);
      strcat(hs->history[history_number % H_NUMBER],preleft);
      strcat(hs->history[history_number % H_NUMBER],substring(subbuf,buf,0,posEnter[0]));
      history_number++;
    }

    for(i = 1; i <= numEnter - 1;i++){
      memset(hs->history[history_number % H_NUMBER],0,bufSize);
      memset(subbuf,0,bufSize);
      char* s = substring(subbuf,buf,posEnter[i-1]+1,posEnter[i]);
      strcpy(hs->history[history_number % H_NUMBER],s);
      history_number++;     
    }
    if(len == bufSize || (posEnter[numEnter - 1] < len - 1)){
      memset(preleft,0,bufSize);
      memset(subbuf,0,bufSize);
      char* s = substring(subbuf,buf,posEnter[numEnter-1],len);
      strcpy(hs->history[history_number % H_NUMBER],s);
    }
  }
  close(fp);
  hs->history[(history_number) % H_NUMBER][0]='\0';
  if(history_number+1 >= H_NUMBER){
    hs->len = H_NUMBER;
  }
  else{
    hs->len = history_number+1;
  }

  hs->currentcmd = (history_number) % H_NUMBER;
  hs->lastusedcmd = hs->currentcmd;

  return;
}

void setHistory(struct inputHistory* hs){
  if(!hs || hs->len == 0){
    return;
  }
  int fp = open("/input_history",O_RDWR | O_CREATE);  

  const int bufSize = 256;
  char buf[bufSize];
  while(read(fp,buf,bufSize)){
    memset(buf,0,bufSize);
  }

  int i;
  if(hs->len >= H_NUMBER){
    int last = hs->lastusedcmd;
    for(i = 1;i <= H_NUMBER;i++){
      int index = (last+i) % H_NUMBER;
      write(fp,hs->history[index],strlen(hs->history[index]));
      write(fp,"\n",1);
    }
  }
  else{
    for(i = 0;i < hs->len;i++){
      write(fp,hs->history[i],strlen(hs->history[i]));
      write(fp,"\n",1);

    }
  }
  close(fp);

}

char* substring(char *dst,char *src,int start,int end){
  char* os;
  os = dst;
  int i;
  for(i = start;i < end;i++){
    *dst++ = src[i];
  }
  *dst++ = '\0';
  return os;
}

char* strcat(char* dst,char* source){
  char *os;
  os = dst;
  while(*os != '\0'){
    os++;
  }
  while((*os++ = *source++) != '\0'){

  }
  return dst;
}