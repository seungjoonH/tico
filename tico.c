#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INT 127
#define MIN_INT -128
#define INST_LEN 15
#define MEM_SIZ 256

typedef enum {
  VALUE, READ, WRITE, ASSIGN, MOVE, LOAD,
  STORE, ADD, MINUS, MULT, MOD,
  EQ, LESS, JUMP, JUMPIF, TERM
} Type;

typedef union {
  Type type;
  int num;
} Inst;

typedef struct {
  char *line;
  Type type;
  int regs[3];
  int value;
} MemoryCell;

char *readLine();
void initMem();
void visMem();
void splitAdrInst(char *str, int *addr, char **inst);
void saveToMem(int addr, char *str); 
Type toType(char *str);
int execute(int addr);

/* INSTRUCTION FUNCTIONS */
int read   (int cur, int m);
int write  (int cur, int m);

int assign (int cur, int m, int c);
int move   (int cur, int md, int ms);
int load   (int cur, int md, int ms);
int store  (int cur, int md, int ms);

int add    (int cur, int md, int mx, int my);
int minus  (int cur, int md, int mx, int my);
int mult   (int cur, int md, int mx, int my);
int mod    (int cur, int md, int mx, int my);

int eq     (int cur, int md, int mx, int my);
int less   (int cur, int md, int mx, int my);

int jump   (int cur, int m);
int jumpIf (int cur, int m, int c);

FILE *fp;
Inst insts[INST_LEN];
MemoryCell memory[MEM_SIZ];
int iptCount = 0;
int optCount = 0;

const char *instructions[INST_LEN + 1] = {
  "VALUE", "READ", "WRITE", "ASSIGN", "MOVE", "LOAD", 
  "STORE", "ADD", "MINUS", "MULT", "MOD", 
  "EQ", "LESS", "JUMP", "JUMPIF", "TERM",
};

const  int params[INST_LEN + 1] = {
  -1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 1, 2, 0,
};

int main() {
  char *filename = malloc(100);
  
  printf("\nProgram starts ...");
  printf("\n");
  printf("\n  *************************");
  printf("\n  *  TICO: TIny COmputer  *");
  printf("\n  *************************");
  printf("\n");
  printf("\n  Enter the name of the executable file");
  printf("\n  > ");
  scanf("%[^\n]", filename);
  
  printf("\n  '%s' file successfully loaded", filename);
  printf("\n\n");

  fp = fopen(filename, "r");
  initMem();

  char *s = 0x0;
  while ((s = readLine())) {
    int address;
    char *line;
    
    splitAdrInst(s, &address, &line);
    if (strcmp(line, "")) memory[address].line = line;
	}

  for (int i = 0; i < MEM_SIZ; i++) {
    char *str;

    str = malloc(strlen(memory[i].line) + 1);
    strcpy(str, memory[i].line);
    saveToMem(i, str);
    
    free(str);
  }

  visMem();

  int cur = 0;
  while (1) {
    cur = execute(cur);
    if (cur == -1) break;
  }

  printf("\n\nProgram terminated!\n");

  free(filename);
  
  return 0;
}

char *readLine() {
  static char buf[BUFSIZ];
  static int buf_n = 0;
  static int curr = 0;

  if (feof(fp) && curr == buf_n - 1) return 0x0;

  char *s = 0x0;
  size_t s_len = 0;
  do {
    int end = curr;
    while (!(end >= buf_n || !iscntrl(buf[end]))) end++;
    if (curr < end && s != 0x0) { curr = end; break; }
    curr = end;
    while (!(end >= buf_n || iscntrl(buf[end]))) end++;
    if (curr < end) {
      if (s == 0x0) {
        s = strndup(buf + curr, end - curr);
        s_len = end - curr;
      }
      else {
        s = realloc(s, s_len + end - curr + 1);
        s = strncat(s, buf + curr, end - curr);
        s_len = s_len + end - curr;
      }
    }
    if (end < buf_n) { curr = end + 1; break; }

    buf_n = fread(buf, 1, sizeof(buf), fp);
    curr = 0;
  } while (buf_n > 0);

  return s;
}

void splitAdrInst(char *str, int *addr, char **inst) {
  char *colonPtr, *addrPtr, *instPtr, *endPtr;
  colonPtr = strchr(str, ':');
  *colonPtr = '\0';

  addrPtr = str;
  instPtr = colonPtr + 1;

  while (*instPtr == ' ') instPtr++;
  endPtr = instPtr + strlen(instPtr) - 1;

  while (endPtr > instPtr && *endPtr == ' ') {
    *endPtr = '\0'; endPtr--;
  }

  *addr = atoi(addrPtr);
  *inst = instPtr;
}

void saveToMem(int addr, char *str) {
  if (strchr(str, ' ') != NULL || !strcmp(str, "TERM")) {
    char *typeStr;
    typeStr = strtok(str, " ");
    char *token = strtok(NULL, " ");

    for (int i = 0; i < 3; i++) {
      if (!token) { 
        memory[addr].regs[i] = params[memory[addr].type]; 
        break;
      }
      memory[addr].regs[i] = atoi(token);
      token = strtok(NULL, " ");
    }

    memory[addr].type = toType(typeStr);
  }
  else {
    char *dqPtr = strstr(str, "\"");
    if (dqPtr) {
      char *end = strstr(dqPtr + 1, "\"");
      if (end) {
        *end = '\0';
        memory[addr].value = atoi(dqPtr + 1);
      }
    }
  }
}

void initMem() {
  for (int i = 0; i < MEM_SIZ; i++) {
    memory[i].line = "\"0\"";
    for (int j = 0; j < 3; j++) memory[i].regs[j] = -1;
  }
}

void visMem() {
  printf("\n***************\n MEMORY\n");
  for (int i = 0; i < MEM_SIZ; i++) {
    printf("\n %3d | ", i);
    if (memory[i].type > 0 && memory[i].type < INST_LEN) {
      printf("%8s | ", instructions[memory[i].type]);
      for (int j = 0; j < params[memory[i].type]; j++) printf("%3d ", memory[i].regs[j]);
      continue;
    }
    printf("%8s | %3d", "VALUE", memory[i].value);
  }
  printf("\n***************\n");
}

Type toType(char *str) {
  for (int i = 0; i < INST_LEN; i++)
    if (!strcmp(instructions[i], str)) return i;
  return -1;
}

int execute(int addr) {
  if (memory[addr].type < 0 || memory[addr].type > INST_LEN) return -2;
  else if (!memory[addr].type) return addr + 1;

  int reg0 = memory[addr].regs[0];
  int reg1 = memory[addr].regs[1];
  int reg2 = memory[addr].regs[2];

  // visMem();
  switch (memory[addr].type) {
    case READ   : return read  (addr, reg0);
    case WRITE  : return write (addr, reg0);
    case ASSIGN : return assign(addr, reg0, reg1);
    case MOVE   : return move  (addr, reg0, reg1);
    case LOAD   : return load  (addr, reg0, reg1);
    case STORE  : return store (addr, reg0, reg1);
    case ADD    : return add   (addr, reg0, reg1, reg2);
    case MINUS  : return minus (addr, reg0, reg1, reg2);
    case MULT   : return mult  (addr, reg0, reg1, reg2);
    case MOD    : return mod   (addr, reg0, reg1, reg2);
    case EQ     : return eq    (addr, reg0, reg1, reg2);
    case LESS   : return less  (addr, reg0, reg1, reg2);
    case JUMP   : return jump  (addr, reg0);
    case JUMPIF : return jumpIf(addr, reg0, reg1);
    case TERM   : return -1;
    default: break;
  }

  return -2;
}

int read(int cur, int m) {
  int n;
  if (!iptCount) printf("  << INPUT >>\n");
  printf("  %d > ", ++iptCount); scanf("%d", &n);
  memory[m].value = n;
  return cur + 1;
}

int write(int cur, int m) { 
  if (!optCount) printf("\n  << OUTPUT >>\n  ");
  printf("%d", memory[m].value); optCount++;
  return cur + 1; 
}
int assign(int cur, int m, int c) { 
  memory[m].value = c; 
  return cur + 1;
}
int move(int cur, int md, int ms) { 
  memory[md].value = memory[ms].value; 
  return cur + 1;
}
int load(int cur, int md, int ms) { 
  memory[memory[md].value].value = memory[memory[ms].value].value; 
  return cur + 1;
}
int store(int cur, int md, int ms) { 
  memory[memory[ms].value].value = memory[memory[md].value].value; 
  return cur + 1;
}
int add(int cur, int md, int mx, int my) { 
  memory[md].value = memory[mx].value + memory[my].value; 
  return cur + 1;
}
int minus(int cur, int md, int mx, int my) { 
  memory[md].value = memory[mx].value - memory[my].value; 
  return cur + 1;
}
int mult(int cur, int md, int mx, int my) { 
  memory[md].value = memory[mx].value * memory[my].value; 
  return cur + 1;
}
int mod(int cur, int md, int mx, int my) { 
  memory[md].value = memory[mx].value % memory[my].value; 
  return cur + 1;
}
int eq(int cur, int md, int mx, int my) { 
  memory[md].value = memory[mx].value == memory[my].value; 
  return cur + 1;
}
int less(int cur, int md, int mx, int my) { 
  memory[md].value = memory[mx].value < memory[my].value; 
  return cur + 1;
}
int jump(int cur, int m) { return m; }
int jumpIf(int cur, int m, int c) { return memory[c].value ? m : cur + 1; }
