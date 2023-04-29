/* VIDEO LINK */
// https://www.youtube.com/watch?v=D8M03FNB2K4&ab_channel=sjh

/* TEAM MEMBER */
// Kim Hongchan (21700214)
// Hyeon Seungjoon (21800788)

/* PREPROCESSORS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

#define MAX_SCHR 127    // maximum of signed char
#define MIN_SCHR -128   // minimum of signed char
#define MAX_UCHR 255    // maximum of unsigned char
#define MIN_UCHR 0      // minimum of unsigned char
#define MEM_SIZ 256     // size of memory
#define MEM_VIS 80      // memory range to visualize

/* ENUMS */
typedef enum { 
  NO_OPR, OPD_NUM, 
  WF_COLON, WF_DQ,
  NUM_IPT, NUM_ADR, NUM_OPD, NUM_VAL, NUM_ASN
} CompileErrorType;

typedef enum {
  OVFL, NOT_NUM, DIV_ZERO
} NumErrorType;

typedef enum { VALUE,  
  READ,   WRITE,  ASSIGN, MOVE, 
  LOAD,   STORE,  ADD,    MINUS, 
  MULT,   MOD,    EQ,     LESS, 
  JUMP,   JUMPIF, TERM,   TYPE_LEN
} Type;

/* STRUCTURE */
typedef struct {
  unsigned char operands[3];
  Type operator;
} Instruction;

/* UNION */
typedef union {
  char value;
  Instruction inst;
} MemoryCell;

/* FUNCTION PROTOTYPES */
// string-integer extended functions
int intlen(int i);
int extractNumber(char *str);
int ctoi(char *str, int *num, int min, int max);

// tico main functions
char *readLine      ();
void initMem        ();
void visMem         ();
void ignoreComment  (char *line);
void splitAdrInst   (char *str,  int *addr, char **inst);
void saveToMem      (int  addr,  char *str, char *line); 
Type toType         (char *str);
int  execute        (int  addr);
int ticoInput       ();
void ticoOutput     (int  n);

// Error-handling related functions
void fileIOError();
void inputError(NumErrorType type);
void compileError(CompileErrorType type, char *line);
void numError(CompileErrorType cType, NumErrorType nType, char *line);
void visError(char *msg, char *desc);
void visCompileError(char *msg, char *desc, char *line);

// Instruction functions
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

/* GLOBAL VARIABLES */
FILE *fp;
MemoryCell memory[MEM_SIZ];
int iptCount = 0;
int optCount = 0;
char *filename;
Type curType;
int r = 0, c = 1;

// operator type string array
const char *instStrs[TYPE_LEN] = {
  0x0, "READ", "WRITE", "ASSIGN", "MOVE", "LOAD", 
  "STORE", "ADD", "MINUS", "MULT", "MOD", 
  "EQ", "LESS", "JUMP", "JUMPIF", "TERM",
};

// Number of operands in each operator
const int params[TYPE_LEN] = {
-1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 1, 2, 0,
};

int main(int argc, char *argv[]) {
  filename = malloc(100);
  
  printf("\nProgram starts ...");
  printf("\033[0;34m");
  printf("\n");
  printf("\n  *************************");
  printf("\n  *  TICO: TIny COmputer  *");
  printf("\n  *************************");
  printf("\n");
  printf("\033[0m");
  
  if (argc != 2) fileIOError();
  else strcpy(filename, argv[1]);

  if (!(fp = fopen(filename, "r"))) fileIOError();
  else {
    initMem();

    char *s = 0x0;
    while ((s = readLine())) {
      char *l; c = 1;
      int addr; char *line;
      
      r++;
      ignoreComment(s);
      if (!strcmp(s, "")) continue;
      strcpy(l, s);

      splitAdrInst(s, &addr, &line);
      if (strcmp(line, "")) saveToMem(addr, line, l);
    }

    printf("\033[1;32m");
    printf("\n  %s", filename);
    printf("\033[0;32m");
    printf(" file successfully loaded");
    printf("\033[0m");
    printf("\n\n");

    // visMem();

    int cur = 0;
    while (1) {
      cur = execute(cur);
      if (cur == -1) break;
      if (cur >= MEM_SIZ) break;
    }
  }

  printf("\n\nProgram terminated!\nGOOD BYE!\n\n");

  free(filename);
  
  return 0;
}

/// @brief Calculates the number of digits in a given integer 
///        when it is converted to a string
/// @param i The integer to calculate the number of digits for
/// @return The number of digits in the integer as an integer value
int intlen(int i) { return 1 + (i ? (int) log10(i) : 1); }

/// @brief Extracts the first integer found in a given character 
///        string and returns its integer value
/// @param str The character string to search for the first integer
/// @return The integer value of the first integer found in the character string
int extractNumber(char *str) {
  char numStr[10];
  int count = 1, first;

  for (int i = 0; i < strlen(str); i++) {
    numStr[0] = '+';
    if (!isdigit(str[i])) continue;
    if (count == 1) first = i;
    numStr[count++] = str[i];
  }
  numStr[count] = '\0';
  if (str[first - 1] == '-') numStr[0] = '-';

  return atoi(numStr);
}

// const to i
// 0: no error
// 1: " not exist
// 2: " not closed
// 3: non numeric
// 4: overflow

/// @brief Converts a string representation of an integer within double
///        quotes to an integer value, and checks if it falls within
///        a specified range
/// @param str The string containing the integer within double quotes
/// @param num A pointer to an integer where the parsed integer value
///            will be store
/// @param min The minimum allowed value for the integer
/// @param max The maximum allowed value for the integer
/// @return An integer value indicating success (0) or an error code (1-4)
int ctoi(char *str, int *num, int min, int max) {
  *num = extractNumber(str);

  char *dqPtr = strchr(str, '\"');
  if (!dqPtr || str[0] != '\"') return 1;

  char *end = strchr(dqPtr + 1, '\"');
  if (!end) return 2;
  *end = '\0';

  int val = atoi(str + 1);
  if (!val && strcmp(str + 1, "0")) return 3;
  if (val < min || val > max) return 4;
  return 0;
}

/// @brief Reads a line from a file pointed to by the 
///        global variable fp, dynamically allocating memory 
///         as needed to store the line
/// @return A pointer to a dynamically allocated string 
///         containing the read line, or a null pointer 
///         if the end of the file is reached
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

/// @brief Initializes the global memory array by setting 
///        all values to zero and setting each instruction 
///        to a value operator with no operands
void initMem() {
  for (int addr = 0; addr < MEM_SIZ; addr++) {
    memory[addr].value = 0;
    memory[addr].inst.operator = VALUE;
    for (int i = 0; i < 3; i++)
      memory[addr].inst.operands[i] = 0;
  }
}

/// @brief Visualize a formatted table of the contents of the memory
void visMem() {
  printf("\n┏");
  for (int i = 0; i < 63; i++) printf("━"); printf("┓");
  printf("\n┃ MEMORY (0~%d) %47s", MEM_VIS - 1, "");
  if (MEM_VIS - 1 < 100) printf(" ");
  if (MEM_VIS - 1 < 10) printf(" ");
  printf("┃\n┠");
  for (int i = 0; i < 31; i++) printf("─"); printf("┬");
  for (int i = 0; i < 31; i++) printf("─"); printf("┨");
  for (int addr = 0; addr < MEM_VIS / 2 + MEM_VIS % 2; addr++) {
    printf("\n");
    for (int i = 0; i < 2; i++) {
      printf(i ? "│ " : "┃ ");
      int idx = (MEM_VIS / 2 + MEM_VIS % 2) * i + addr;
      if (idx == MEM_VIS) { printf("%30s", ""); break; }
      printf("%3d | ", idx);
      Instruction inst = memory[idx].inst;
      if (inst.operator > 0 && inst.operator < TYPE_LEN) {
        printf("%6s | ", instStrs[inst.operator]);
        for (int j = 0; j < 3; j++) {
          if (j >= params[inst.operator]) printf("     ");
          else if (inst.operator == ASSIGN && j == 1) 
            printf("%4d ", (char) inst.operands[j]);
          else printf("%4d ", inst.operands[j]);
        }
        continue;
      }
      printf("%6s | %4d %9s ", "VALUE", memory[idx].value, "");
    }
    printf("┃");
  }
  printf("\n┗");
  for (int i = 0; i < 31; i++) printf("━"); printf("┷");
  for (int i = 0; i < 31; i++) printf("━"); printf("┛\n\n");
}

/// @brief Ignores inline comments from a given string
/// @param line The input string to be processed
void ignoreComment(char *line) { 
  char *ptr = strstr(line, "//"); 
  if (ptr) *ptr = '\0'; 
}

/// @brief Splits a string containing an address and an instruction
///        separated by a colon into its address and instruction components
/// @param str  the string to be split
/// @param addr a pointer to an integer that will hold the parsed address
/// @param inst a pointer to a char pointer that will point to the parsed instruction
void splitAdrInst(char *str, int *addr, char **inst) {
  char *line; 
  char *colonPtr, *addrPtr, *instPtr, *endPtr;
  strcpy(line, str);

  colonPtr = strchr(str, ':');
  if (!colonPtr) { c++; compileError(WF_COLON, line); }
  *colonPtr = '\0';

  addrPtr = str;
  instPtr = colonPtr + 1;

  while (*instPtr == ' ') instPtr++;
  endPtr = instPtr + strlen(instPtr) - 1;

  while (endPtr > instPtr && *endPtr == ' ') {
    *endPtr = '\0'; endPtr--;
  }

  *addr = atoi(addrPtr);

  bool nonNumeric = !*addr && strcmp(addrPtr, "0");
  bool overflow = *addr < MIN_UCHR || *addr > MAX_UCHR;
  
  if (overflow) numError(NUM_ADR, NOT_NUM, line);
  if (nonNumeric) numError(NUM_ADR, OVFL, line);

  *inst = instPtr;
}

/// @brief A function that saves str into the memory 
///        at the addr position after processing it appropriately
/// @param addr specific memory address to save
/// @param str  assembly code information to process
/// @param line pure line from the file
void saveToMem(int addr, char *str, char *line) {
  bool hasSpace = strchr(str, ' ');

  char *typeStr;
  typeStr = strtok(str, " ");
  curType = toType(typeStr);
  c += 1 + intlen(addr);

  bool noOperator = true;
  noOperator &= strcmp(typeStr, "VALUE");
  noOperator &= !curType;
  noOperator &= !strcmp(typeStr, "0") || !atoi(typeStr);
  
  if (noOperator) { c++; compileError(NO_OPR, line); }

  bool isTerm = !strcmp(str, "TERM");
  bool isAssign = !strcmp(str, "ASSIGN");

  if (hasSpace) {
    c += 1 + strlen(typeStr);
    
    if (isTerm) compileError(OPD_NUM, line);
    char *token = strtok(NULL, " ");

    int i = 0, operand = 0;
    for (; i < 3; i++) {
      if (!token) break;
      
      if (isAssign && i == 1) {
        int err = ctoi(token, &operand, MIN_SCHR, MAX_SCHR);

        switch (err) {
          case 4: c += 2; numError(NUM_ASN, OVFL, line);
          case 3: c += 2; numError(NUM_ASN, NOT_NUM, line);
          case 2: c += 2 + intlen(operand);
          case 1: compileError(WF_DQ, line);
          default: break;
        }
      }
      else {
        c++;
        operand = atoi(token);

        bool nonNumeric = !operand && strcmp(token, "0");
        bool overflow = operand < MIN_UCHR || operand > MAX_UCHR;

        if (overflow) numError(NUM_OPD, OVFL, line);
        if (nonNumeric) numError(NUM_OPD, NOT_NUM, line);

        c += intlen(operand);
      }

      memory[addr].inst.operands[i] = operand;

      token = strtok(NULL, " ");

      if (token && i + 1 >= params[curType]) compileError(OPD_NUM, line);
    }
    
    if (i < params[curType]) compileError(OPD_NUM, line);

    memory[addr].inst.operator = curType;
  }
  else {
    if (isTerm) { memory[addr].inst.operator = TERM; return; }
    
    int n, err;
    err = ctoi(str, &n, MIN_UCHR, MAX_UCHR);
    switch (err) {
      case 4: numError(NUM_VAL, OVFL, line); break;
      case 3: c += 2; numError(NUM_VAL, NOT_NUM, line); break;
      case 2: c += 2 + intlen(n);
      case 1: compileError(WF_DQ, line); break;
      case 0: memory[addr].value = n; break;
      default: break;
    }
    
    return;
  }
  if (!toType(str)) compileError(NO_OPR, line);
  
}

/// @brief Converts a string to a Type enumeration value
/// @param str The string to convert
/// @return The Type value corresponding to the input string
Type toType(char *str) {
  for (int i = 1; i < TYPE_LEN; i++)
    if (!strcmp(instStrs[i], str)) return i;
  return VALUE;
}

/// @brief Executes the instruction at the given memory address
/// @param addr The memory address of the instruction to execute
/// @return The address of the next instruction to execute, 
///         or a negative value if an error occurred.
int execute(int addr) {
  Instruction inst = memory[addr].inst;
  if (!inst.operator) return addr + 1;

  int reg0 = inst.operands[0];
  int reg1 = inst.operands[1];
  int reg2 = inst.operands[2];

  switch (memory[addr].inst.operator) {
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

/// @brief Reads user input and validates it, 
///        returning a valid input as an integer.
/// @return A valid input as an integer.
int ticoInput() {
  int n; 
  bool overflow = false;
  bool nonNumeric = false;

  if (!iptCount++) printf("  << INPUT >>\n");
  do {
    if (overflow) inputError(OVFL);
    if (nonNumeric) inputError(NOT_NUM);

    overflow = false;
    nonNumeric = false;

    printf("  %d > ", iptCount); 
    nonNumeric |= !scanf("%d", &n);
    while (getchar() != '\n');
    overflow |= n < MIN_SCHR || n > MAX_SCHR;
  } while (nonNumeric || overflow);

  return n;
}

/// @brief Outputs an integer value to the console
/// @param n The integer value to output
void ticoOutput(int n) {
  if (!optCount) printf("\n  << OUTPUT >>\n  ");
  printf("%d", n); optCount++;
}


/// @brief This function is called in case of file I/O errors
void fileIOError() {
  char *msg = malloc(100);
  char *desc = malloc(100);

  if (strcmp(filename, "")) {
    strcpy(msg, "File not found");
    strcpy(desc, "`\033[1;31m");
    strcat(desc, filename);
    strcat(desc, "\033[0;31m`");
    sprintf(desc, "%s file does not exist!", desc);
  }
  else {
    strcpy(msg, "Filename not entered");
    strcpy(desc, "Enter the filename");
  }

  visError(msg, desc);

  free(msg);
  free(desc);
  exit(-1);
}

/// @brief Displays an error message for invalid input
/// @param type The type of error encountered
void inputError(NumErrorType type) {
  char *msg = malloc(100);
  char *desc = malloc(100);

  switch (type) {
  case OVFL: strcpy(desc, "Value overflow"); break;
  case NOT_NUM: strcpy(desc, "Non-numeric Value"); break;
  default: break;
  }

  strcpy(msg, "Input error");
  strcat(desc, "\n\n\033[0;32m");
  sprintf(desc, "%s Enter value %d ~ %d integer", desc, MIN_SCHR, MAX_SCHR);

  visError(msg, desc);

  free(msg);
  free(desc);
}

/// @brief  Generates a compile error message and displays it
/// @param type The type of compile error that occurred
/// @param line The line of code that caused the error
void compileError(CompileErrorType type, char *line) {
  char *msg = malloc(100);
  char *desc = malloc(100);

  switch (type) {
    case NO_OPR: 
      strcpy(msg, "Operator undefined"); 
      strcpy(desc, "Existing operator expected"); 
      break;
    case OPD_NUM: 
      strcpy(msg, "Unmatched number of operand");
      sprintf(
        desc, "%s operator must have %d operand%s", 
        instStrs[curType], 
        params[curType], 
        params[curType] > 1 ? "s" : ""
      );
      break;
    case WF_COLON: 
      strcpy(msg, "Wrong format"); 
      strcpy(desc, ":"); 
      break;
    case WF_DQ: 
      strcpy(msg, "Wrong format"); 
      strcpy(desc, "\""); 
      break;
    default: break;
  }

  visCompileError(msg, desc, line);

  free(msg);
  free(desc);
  exit(-1);
}

/// @brief Report a numeric error and terminate the program
/// @param cType The type of numeric object that caused the error
/// @param nType The type of numeric error that occurred
/// @param line  The line of source code where the error occurred
void numError(CompileErrorType cType, NumErrorType nType, char *line) {
  int min, max;
  char *obj = malloc(100);
  char *msg = malloc(100);
  char *desc = malloc(100);
  
  switch (cType) {
    case NUM_ASN: 
      min = MIN_SCHR; max = MAX_SCHR; 
      strcpy(obj, "Constant"); break;
    case NUM_VAL: 
      min = MIN_SCHR; max = MAX_SCHR; 
      strcpy(obj, "Value"); break;
    case NUM_ADR: 
      min = MIN_UCHR; max = MAX_UCHR; 
      strcpy(obj, "Address"); break;
    case NUM_OPD: 
      min = MIN_UCHR; max = MAX_UCHR; 
      strcpy(obj, "Operand"); break;
    default: break;
  }

  switch (nType) {
    case OVFL:
      sprintf(msg, "%s overflow", obj); 
      sprintf(desc, "Set %s %d ~ %d integer", obj, min, max);
      break;
    case NOT_NUM: 
      sprintf(msg, "Non-numeric %s", obj); 
      sprintf(desc, "Set %s %d ~ %d integer", obj, min, max);
      break;
    case DIV_ZERO: 
      strcpy(msg, "Divide by zero"); 
      strcpy(desc, "Cannot be divided by 0");
      break;
    default: break;
  }

  visCompileError(msg, desc, line);

  free(obj);
  free(msg);
  free(desc);
  exit(-1);
}

/// @brief Displays an error message and its description on
///        the console in red color
/// @param msg The error message to be displayed
/// @param desc The description of the error to be displayed
void visError(char *msg, char *desc) {
  fprintf(stderr, "\033[0;31m");
  fprintf(stderr, "\n  [ERROR] %s", msg);
  fprintf(stderr, "\n  %s\n\n", desc);
  fprintf(stderr, "\033[0m");
}

/// @brief Prints a compile error message with additional 
///        information about the error location and code 
///        line where it occurred
/// @param msg The error message to be displayed
/// @param desc The description of the error to be displayed
/// @param line The code line where the error occurred
void visCompileError(char *msg, char *desc, char *line) {
  fprintf(stderr, "\033[0;31m");
  fprintf(stderr, "\n  [ERROR] Compile error");
  fprintf(stderr, "\n  %s", msg);
  fprintf(stderr, "\033[0m\n\n");

  if (line) {
    fprintf(stderr, "\033[1;37m");
    fprintf(stderr, "  %s:%d:%d", filename, r, c);
    fprintf(stderr, "\033[0m");
    fprintf(stderr, "\n  %s\n  ", line);
    
    fprintf(stderr, "\033[0;32m");
    for (int e = 0; e < c - 1; e++) fprintf(stderr, "~"); fprintf(stderr, "^\n");
    for (int e = 0; e < c - 1; e++) fprintf(stderr, " ");
    fprintf(stderr, "  %s\n\n", desc);
    fprintf(stderr, "\033[0m");
  }
}


int read(int cur, int m) {
  int n = ticoInput();
  memory[m].value = n;
  return cur + 1;
}
int write(int cur, int m) { 
  ticoOutput(memory[m].value);
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
  if (memory[ms].value < MIN_UCHR) numError(NUM_VAL, OVFL, 0x0);
  memory[md].value = memory[memory[ms].value].value; 
  return cur + 1;
}
int store(int cur, int md, int ms) { 
  if (memory[md].value < MIN_UCHR) numError(NUM_VAL, OVFL, 0x0);
  memory[memory[md].value].value = memory[ms].value; 
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
  if (!memory[my].value) numError(NUM_VAL, DIV_ZERO, 0x0);
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