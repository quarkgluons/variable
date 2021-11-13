#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define NUM_SCOPES 20
#define NUM_VARIABLE 10

typedef struct scope {
  char env[32];
  char *variables[NUM_VARIABLE];
  char dependence[NUM_VARIABLE][100];
  int number_of_variables;
} scope_t;

int number_of_scopes = 0;
int variables_in_scope = 0;
scope_t scope_arr[NUM_SCOPES];

char visited[30][50];
int visited_count = 0;
void print_visited(char * str) {
  for (int i = 0; i < visited_count; i++) {
    if (strcmp(visited[i], str) == 0) continue;
    if (i != 0) 
      printf(", ");
    printf("%s", visited[i]);
  }
}

bool is_visited(char *str) {
  for (int i=0; i < visited_count; i++) {
    if (strcmp(str, visited[i]) == 0) return true;
  }

  return false;
}
