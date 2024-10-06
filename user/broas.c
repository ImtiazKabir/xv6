#include "common/fcntl.h"
#include "common/syscall.h"
#include "common/types.h"
#include "printf.h"
#include "ulib.h"
#include "usys.h"

#define MAX_TOKENS_IN_FILE 1024
#define MAX_TOKENS_IN_LINE 5
#define MAX_INSTRUCTIONS 100

#define MAX_LABELS 100

#define MAX_VARIABLES 100

#define MEMORY_SIZE 1024

#define MX_TOK_SZ 32

struct Label {
  char *name;
  long int instructionIndex;
};

struct Variable {
  char *name;
  void *value;
};

enum TokenType { LABEL, OPCODE, VARIABLE, IMMEDIATE };

struct LexToken {
  enum TokenType type;
  union {
    long int tokint;
    char tokstr[MX_TOK_SZ];
  } token;
};

long readchar(int fd, char *pc) { return read(fd, pc, 1); }

int readline(int fd, char *line, unsigned long max) {
  char c;

  if (max == 0) {
    return -1;
  }

  if (readchar(fd, &c) == 0) {
    return 1;
  }

  if (c == '\n') {
    *line = '\0';
    return 0;
  }

  *line = c;
  return readline(fd, line + 1, max - 1);
}

char *getword(char *line, char *word, unsigned long max) {
  unsigned long i = 0;

  while ((i < max) && (*line != ' ') && (*line != '\0')) {
    *word = *line;
    word++;
    line++;
    i++;
  }

  while (*line == ' ') {
    line++;
  }

  *word = '\0';
  return line;
}

enum TokenType toktype(char const *word) {
  static char const opcodes[][8] = {
      "add", "sub", "lw",  "sw",    "mult",  "div",  "beq",    "bneq", "mod",
      "xor", "or",  "and", "not",   "sl",    "sr",   "blt",    "bgt",  "ble",
      "bge", "jmp", "ref", "deref", "print", "exit", "syscall"};
  static int const n = sizeof(opcodes) / sizeof(opcodes[0]);
  int i;

  for (i = 0; i < n; i++) {
    if (strcmp(word, opcodes[i]) == 0) {
      return OPCODE;
    }
  }

  if (*word == '@') {
    return LABEL;
  }

  if (('0' <= *word && *word <= '9') || *word == '-' || *word == '\'') {
    return IMMEDIATE;
  }

  return VARIABLE;
}

int getImmVal(char *word) {
  if (('0' <= *word && *word <= '9') || *word == '-') {
    return atoi(word);
  }
  if (word[0] == '\'' && word[1] != '\\') {
    return word[1] - '\0';
  }
  switch (word[2]) {
  case 'a':
    return '\a';
  case 'b':
    return '\b';
  case 'f':
    return '\f';
  case 'n':
    return '\n';
  case 'r':
    return '\r';
  case 't':
    return '\t';
  case 'v':
    return '\v';
  case 's':
    return ' '; /* spaces messed up word tokenization, so .. */
  case '0':
    return '\0';
  }
  return word[2];
}

int getTokens(int fd, struct LexToken *tokens) {
  int n = 0;
  char line[128];
  while (!readline(fd, line, sizeof(line))) {
    char word[16];
    char *l = line;

    while (*l != '\0') {
      enum TokenType type;
      l = getword(l, word, sizeof(word));

      if (*word == ';') {
        break;
      }

      type = toktype(word);

      tokens[n].type = type;

      strcpy(tokens[n].token.tokstr, word);
      if (type == IMMEDIATE) {
        tokens[n].token.tokint = getImmVal(word);
      }
      n++;
    }
  }
  return n;
}

void *getValue(struct LexToken valueToken, struct Variable *variables,
               int numberOfVariables, struct Label *labels,
               int numberOfLabels) {
  if (valueToken.type == IMMEDIATE)
    return (void *)valueToken.token.tokint;
  else if (valueToken.type == VARIABLE) {
    int i;
    for (i = 0; i < numberOfVariables; ++i) {
      if (strcmp(variables[i].name, valueToken.token.tokstr) == 0) {
        return variables[i].value;
      }
    }
  } else if (valueToken.type == LABEL) {
    int i;
    for (i = 0; i < numberOfLabels; ++i) {
      if (strcmp(labels[i].name, valueToken.token.tokstr) == 0) {
        return (void *)labels[i].instructionIndex;
      }
    }
  } else {
    fprintf(2,
            "Cannot get value of %s, (can only access value of a variable or "
            "immediate or label)\n",
            valueToken.token.tokstr);
    exit(1);
  }
  fprintf(2, "%s not defined\n", valueToken.token.tokstr);
  exit(1);
}

void setValue(struct LexToken *pVariableToken, void *value,
              struct Variable *variables, int *pNumberOfVariables) {
  int i;
  struct LexToken variableToken = *pVariableToken;

  if (variableToken.type != VARIABLE) {
    fprintf(2, "Can only set value of a variable\n");
    exit(1);
  }

  for (i = 0; i < *pNumberOfVariables; ++i) {
    if (strcmp(variables[i].name, variableToken.token.tokstr) == 0) {
      variables[i].value = value;
      return;
    }
  }

  variables[*pNumberOfVariables].name = pVariableToken->token.tokstr;
  variables[*pNumberOfVariables].value = value;
  ++(*pNumberOfVariables);
}

int main(int argc, char **argv) {
  int fd;

  static struct LexToken instructions[MAX_INSTRUCTIONS][MAX_TOKENS_IN_LINE];
  static struct LexToken tokens[MAX_TOKENS_IN_FILE];
  int totalTokens;
  int nextInstruction = 0;
  int totalInstructions;

  static struct Label labels[MAX_LABELS];
  int nextLabel = 0;

  static struct Variable variables[MAX_VARIABLES];
  int nextVariable = 0;

  static void *memory[MEMORY_SIZE];

  int i;

  if (argc < 2) {
    fprintf(
        2,
        "Wrong usage. Sample usage: broas <broas_code_file> <...arguments>\n");
    exit(1);
  }

  fd = open(argv[1], O_RDONLY);

  memory[0] = (void *)((long int)argc - 2);
  for (i = 0; i < argc - 2; ++i) {
    memory[i + 1] = argv[i + 2];
  }

  totalTokens = getTokens(fd, tokens);

  i = 0;
  while (i < totalTokens) {
    struct LexToken lexToken = tokens[i++];

    if (lexToken.type == OPCODE) {
      instructions[nextInstruction][0] = lexToken;

      if (strcmp(lexToken.token.tokstr, "add") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "sub") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "lw") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "sw") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "mult") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "div") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "beq") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "bneq") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "mod") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "xor") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "or") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "and") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "not") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "sl") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "sr") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "blt") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "bgt") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "ble") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "bge") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "jmp") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "ref") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "deref") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "print") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "syscall") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
        instructions[nextInstruction][2] = tokens[i++];
        instructions[nextInstruction][3] = tokens[i++];
      } else if (strcmp(lexToken.token.tokstr, "exit") == 0) {
        instructions[nextInstruction][1] = tokens[i++];
      } else {
        fprintf(2, "Unknown operation %s\n", lexToken.token.tokstr);
        exit(1);
      }

      ++nextInstruction;
    }

    else if (lexToken.type == LABEL) {
      labels[nextLabel].name = tokens[i - 1].token.tokstr;
      labels[nextLabel].instructionIndex = nextInstruction;
      ++nextLabel;
    }

    else {
      fprintf(2,
              "Unexpected token %s in place of opcode or label, (encountered "
              "at %dth token position)\n",
              lexToken.token.tokstr, i);
      exit(1);
    }
  }

  totalInstructions = nextInstruction;
  nextInstruction = 0;

  while (nextInstruction < totalInstructions) {
    struct LexToken *instruction = instructions[nextInstruction];
    char *opcode = instruction[0].token.tokstr;

    if (strcmp(opcode, "add") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);
      long int result = leftOperand + rightOperand;

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "sub") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);
      long int result = leftOperand - rightOperand;

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "lw") == 0) {
      long int memoryOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);

      setValue(&instruction[1], memory[memoryOperand], variables,
               &nextVariable);
    }

    else if (strcmp(opcode, "sw") == 0) {
      void *variableOperand =
          getValue(instruction[1], variables, nextVariable, labels, nextLabel);
      long int memoryOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);

      memory[memoryOperand] = variableOperand;
    }

    else if (strcmp(opcode, "mult") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);
      long int result = leftOperand * rightOperand;

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "div") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);
      long int result = leftOperand / rightOperand;

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "beq") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[1], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int branchAddress = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);

      if (leftOperand == rightOperand) {
        nextInstruction = branchAddress;
        continue;
      }
    }

    else if (strcmp(opcode, "bneq") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[1], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int branchAddress = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);

      if (leftOperand != rightOperand) {
        nextInstruction = branchAddress;
        continue;
      }
    }

    else if (strcmp(opcode, "mod") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);
      long int result = leftOperand % rightOperand;

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "xor") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);
      long int result = leftOperand ^ rightOperand;

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "or") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);
      long int result = leftOperand | rightOperand;

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "and") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);
      long int result = leftOperand & rightOperand;

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "not") == 0) {
      long int operand = (long int)getValue(instruction[2], variables,
                                            nextVariable, labels, nextLabel);
      long int result = ~operand;

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "sl") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);
      long int result = leftOperand << rightOperand;

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "sr") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);
      long int result = leftOperand >> rightOperand;

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "blt") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[1], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int branchAddress = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);

      if (leftOperand < rightOperand) {
        nextInstruction = branchAddress;
        continue;
      }
    }

    else if (strcmp(opcode, "bgt") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[1], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int branchAddress = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);

      if (leftOperand > rightOperand) {
        nextInstruction = branchAddress;
        continue;
      }
    }

    else if (strcmp(opcode, "ble") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[1], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int branchAddress = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);

      if (leftOperand <= rightOperand) {
        nextInstruction = branchAddress;
        continue;
      }
    }

    else if (strcmp(opcode, "bge") == 0) {
      long int leftOperand = (long int)getValue(
          instruction[1], variables, nextVariable, labels, nextLabel);
      long int rightOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int branchAddress = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);

      if (leftOperand >= rightOperand) {
        nextInstruction = branchAddress;
        continue;
      }
    }

    else if (strcmp(opcode, "jmp") == 0) {
      long int jumpAddress = (long int)getValue(
          instruction[1], variables, nextVariable, labels, nextLabel);

      nextInstruction = jumpAddress;
      continue;
    }

    else if (strcmp(opcode, "ref") == 0) {
      long int memoryOperand = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);

      setValue(&instruction[1], (void *)(memory + memoryOperand), variables,
               &nextVariable);
    }

    else if (strcmp(opcode, "deref") == 0) {
      long int *addressOperand = (long int *)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int sizeOperand = (long int)getValue(
          instruction[3], variables, nextVariable, labels, nextLabel);

      long int result = 0;

      int byteCount;
      for (byteCount = sizeOperand - 1; byteCount >= 0; --byteCount) {
        char byte = *((char *)addressOperand + byteCount);
        result = (result << 8) | byte;
      }

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "print") == 0) {
      long int operand = (long int)getValue(instruction[1], variables,
                                            nextVariable, labels, nextLabel);

      printf("%c", (char)operand);
    }

    else if (strcmp(opcode, "syscall") == 0) {
      long int syscallNumber = (long int)getValue(
          instruction[2], variables, nextVariable, labels, nextLabel);
      long int paramStart = (long int)getValue(instruction[3], variables,
                                               nextVariable, labels, nextLabel);
      long int result = syscall(
          syscallNumber, (uint64)memory[paramStart],
          (uint64)memory[paramStart + 1], (uint64)memory[paramStart + 2],
          (uint64)memory[paramStart + 3], (uint64)memory[paramStart + 4]);

      setValue(&instruction[1], (void *)result, variables, &nextVariable);
    }

    else if (strcmp(opcode, "exit") == 0) {
      long int exitCode = (long int)getValue(instruction[1], variables,
                                             nextVariable, labels, nextLabel);
      exit(exitCode);
    }

    else {
      printf("Unknown operation %s\n", opcode);
    }

    ++nextInstruction;
  }

  close(fd);
  return 0;
}
