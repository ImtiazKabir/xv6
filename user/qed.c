#include "common/fcntl.h"
#include "printf.h"
#include "ulib.h"
#include "usys.h"

static char lines[128][128] = {0};

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

void addLine(int index, char *line) { strcpy(lines[index], line); }

void showLines() {
  int i;
  int show = 0;
  for (i = 0; i < (int)(sizeof(lines) / sizeof(lines[0])); i++) {
    if (lines[i][0] != '\0') {
      show = 1;
      printf("[%d]: %s\n", i, lines[i]);
    }
  }
  if (show == 0) {
    printf("<EMPTY>\n");
  }
}

int runCommand(char *cmd) {
  char op[8] = {0};
  char *rest = getword(cmd, op, sizeof(op));
  if (strcmp(op, "quit") == 0) {
    return 1;
  }
  if (strcmp(op, "s") == 0) {
    showLines();
    return 0;
  }
  addLine(atoi(op), rest);
  return 0;
}

void getLines(int fd) {
  char line[150] = {0};
  int i = 0;
  while (!readline(fd, line, sizeof(line))) {
    addLine(i, line);
    i += 10;
  }
}

void writeLines(int fd) {
  int i;
  for (i = 0; i < (int)(sizeof(lines) / sizeof(lines[0])); i++) {
    if (lines[i][0] != '\0') {
      fprintf(fd, "%s\n", lines[i]);
    }
  }
}

int main(int argc, char **argv) {
  int readfd, writefd;

  if (argc != 2) {
    fprintf(2, "usage: %s file\n", argv[0]);
    exit(1);
  }

  readfd = open(argv[1], O_RDONLY | O_CREATE);
  getLines(readfd);
  close(readfd);

  while (1) {
    char cmd[150] = {0};
    readline(0, cmd, sizeof(cmd));
    if (runCommand(cmd)) {
      break;
    }
  }

  writefd = open(argv[1], O_WRONLY);
  writeLines(writefd);
  close(writefd);

  exit(0);
}
