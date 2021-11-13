#include "variables.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *types[] = {"int", "char", "float", "double", "void", NULL};

void dfs(scope_t *, char *);

typedef enum {
  NO_COMMENT,
  FOR_SLASH_ENC,
} comment_t;

comment_t comment_state = NO_COMMENT;

typedef enum {
  NO_DECLARATION,
  TYPE_READ,
  VARIABLE_READ,
  ASSIGN_READ,
  FUNCTION_DECL,
  EXPRESSION_VARIABLE_READ,
  EXPRESSION_ASSIGN_READ,
} parsing_declaration_state;

parsing_declaration_state declaration_state = NO_DECLARATION;
/* Function params being read */
bool function_params = false;
bool is_token_type(char *token) {
  int index = 0;
  while (types[index] != NULL) {
    if (strcmp(token, types[index]) == 0)
      return true;
    index++;
  }
  return false;
}
char ch; /* current character */
int current_scope = 0;
int curly_braces_number = 0; /* If this is zero means we are in global scope */

/* Will hold the dependency string for the current token incase of an assignment
 */
char *current_dependency = NULL;

void parse_comment() {
  if (ch == '/') {
    while ((ch = getchar()) != '\n')
      ;
  }
  if (ch == '*') {
  false_terminating_star:
    while ((ch = getchar()) != '*')
      ;
    if ((ch = getchar()) != '/')
      goto false_terminating_star;
  }
  comment_state = NO_COMMENT;
}

char *get_token() {
  char *token = malloc(32 * sizeof(char));
  int index = 0;

  while ((ch = getchar()) != EOF) {
    if (comment_state == FOR_SLASH_ENC) {
      parse_comment();
    }

    switch (ch) {
    case ' ':
    case '!':
    case '\t':
    case '\n':
    case '/':
    case ',':
    case ')':
    case '+':
    case '*':
    case '=':
    case '\r':
    case ';':
      if (ch == '/' && declaration_state == NO_DECLARATION) {
        comment_state = FOR_SLASH_ENC;
        continue;
      }
      if (ch == '!') declaration_state = NO_DECLARATION;
      token[index] = '\0';
      return token;
    case '(':
      if (declaration_state == TYPE_READ)
        declaration_state = FUNCTION_DECL;
      token[index] = '\0';
      return token;
    case '{':
      curly_braces_number++;
      function_params = false;
      if (declaration_state == FUNCTION_DECL) {
        current_scope = number_of_scopes;
      }
      declaration_state = NO_DECLARATION;
      break;
    case '}':
      curly_braces_number--;
      if (curly_braces_number == 0) {
        current_scope = 0;
      }
      declaration_state = NO_DECLARATION;
      break;
    default:
      token[index++] = ch;
      break;
    }
    if (index == 0) {
      token[0] = ch;
      token[1] = '\0';
      return token;
    }
  }

  free(token);
  return NULL;
}

// track the scope of the variable;
char *variable_in_function = "global";
bool is_variable(char *token, bool flag) {
  scope_t *temp = &scope_arr[number_of_scopes - 1];
  for (int i = 0; i < temp->number_of_variables; i++) {
    if (strcmp(token, temp->variables[i]) == 0) {
      if (flag)
        current_dependency = temp->dependence[i];
      variable_in_function = temp->env;
      return true;
    }
  }

  temp = &scope_arr[0];
  for (int i = 0; i < temp->number_of_variables; i++) {
    if (strcmp(token, temp->variables[i]) == 0) {
      if (flag)
        current_dependency = temp->dependence[i];
      variable_in_function = temp->env;
      return true;
    }
  }
  return false;
}

void init() {
  for (int i = 0; i < NUM_SCOPES; i++) {
    for (int j = 0; j < NUM_VARIABLE; j++) {
      scope_arr[i].dependence[j][0] = '\0';
    }
  }
}

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("Wrong usage: ./main <input-file>\n");
    exit(-1);
  }

  close(STDIN_FILENO);
  if (open(argv[1], O_RDONLY) == -1) {
    printf("%s\n", argv[1]);
    perror("read");
    exit(-1);
  }
  init();
  strcpy(scope_arr[0].env, "global");
  scope_arr[0].number_of_variables = 0;

  number_of_scopes = 1;

  // strcpy(current_scope, scope_arr[0].env);

  //char *variable[20];
  //int number_variables = 0;
  char *token = NULL;
  while ((token = get_token()) != NULL) {
    if (isspace(token[0]) || token[0] == '\0') {
      if (declaration_state == VARIABLE_READ) {
        if (ch == '=')
          declaration_state = ASSIGN_READ;
        if (ch == '(') {
          declaration_state = FUNCTION_DECL;
          printf("function decl %s\n", token);
        }
        if (ch == ';')
          declaration_state = NO_DECLARATION;
        /* if (ch == ')') function_params = false; */
      } else {
        if (declaration_state == EXPRESSION_VARIABLE_READ) {
          if (ch == '=') {
            declaration_state = EXPRESSION_ASSIGN_READ;
          }
        }else if (declaration_state == EXPRESSION_ASSIGN_READ && ch == '=') {
          declaration_state = NO_DECLARATION;
        }
      }
      if (declaration_state == EXPRESSION_ASSIGN_READ && ch == ';')
        declaration_state = NO_DECLARATION;
      free(token);
      continue;
    }
    if (strcmp(token, "NULL") == 0) {
      free(token);
      continue;
    }

    if (is_token_type(token)) {
      if (declaration_state == FUNCTION_DECL) {
        function_params = true;
      }
      declaration_state = TYPE_READ;
      free(token);
      continue;
    }

    if (declaration_state == TYPE_READ) {
      declaration_state = VARIABLE_READ;

      scope_t *temp = &scope_arr[(curly_braces_number || function_params)
                                     ? number_of_scopes - 1
                                     : 0];
      temp->variables[(temp->number_of_variables)++] = token;
      if (ch == ';') declaration_state = NO_DECLARATION;
      if (ch == '=')
        declaration_state = ASSIGN_READ;
      continue;
    }
    if (declaration_state == ASSIGN_READ) {
      declaration_state = NO_DECLARATION;
      if (isdigit(token[0]) || token[0] == '\'' || token[0] == '"' || ch == '(') {
        free(token);
        continue;
      }

      is_variable(token, false);
      scope_t *temp =
          &scope_arr[curly_braces_number ? number_of_scopes - 1 : 0];
      strcat(temp->dependence[temp->number_of_variables - 1], variable_in_function);
      strcat(temp->dependence[temp->number_of_variables - 1], ".");
      strcat(temp->dependence[temp->number_of_variables - 1], token);
      free(token);
      continue;
    }
    if (declaration_state == VARIABLE_READ && ch == ';') {
      declaration_state = NO_DECLARATION;
      current_dependency = NULL;
      continue;
    }

    if (declaration_state == FUNCTION_DECL) {
      function_params = true;
      strcpy(scope_arr[number_of_scopes++].env, token);
      free(token);
      scope_arr[number_of_scopes - 1].number_of_variables = 0;
      declaration_state = NO_DECLARATION;
      continue;
    }

    if (declaration_state == EXPRESSION_ASSIGN_READ) {
      if (isdigit(token[0]) || token[0] == '\'' || token[0] == '"') {
        free(token);
        continue;
      }

      is_variable(token, false);
      strcat(current_dependency, ",");
      strcat(current_dependency, variable_in_function);
      strcat(current_dependency, ".");
      strcat(current_dependency, token);
      if (ch == ';')
        declaration_state = NO_DECLARATION;
      free(token);
      continue;
    }

    if (declaration_state == NO_DECLARATION && is_variable(token, true) &&
        !function_params) {
      if (ch == '=')
        declaration_state = EXPRESSION_ASSIGN_READ;
      else
        declaration_state = EXPRESSION_VARIABLE_READ;
      free(token);
      continue;
    }

    free(token);
  }

  /* for (int i = 0; i < number_variables; i++) { */
    /* puts(variable[i]); */
  /* } */
  for (int i = 0; i < number_of_scopes; i++) {
    scope_t *current = &scope_arr[i];
    printf("In scope: %s\n", current->env);
    for (int j = 0; j < current->number_of_variables; j++) {
      printf("%s -> %s\n", current->variables[j], current->dependence[j]);
    }
  }

  printf("Starting DFS\n");

  for (int i = 0; i < number_of_scopes; i++) {
    scope_t *current = &scope_arr[i];
    printf("--------In scope %s -------\n", current->env);
    for (int j = 0; j < current->number_of_variables; j++) {
      printf("%s.%s => {", current->env, current->variables[j]);
      visited_count = 0;
      dfs(current, current->dependence[j]);
      char temp[100];

      strcpy(temp, current->env);
      strcat(temp, ".");
      strcat(temp, current->variables[j]);
      print_visited(temp);
      printf("}\n");
    }
    printf("\n");
  }
}

void dfs(scope_t *source, char *dependency) {
  if (*dependency == '\0') {
    return;
  }
  if (*dependency == ',')
    dependency++;
  char temp[32];
  int i = 0;
  while ((temp[i] = *dependency) != '.') {
    dependency++;
    i++;
  }

  scope_t *new_scope = source;
  //char *new_dependence;
  temp[i] = '\0';

  for (int i = 0; i < number_of_scopes; i++) {
    if (strcmp(scope_arr[i].env, temp) == 0) {
      new_scope = &scope_arr[i];
      break;
    }
  }
  i = 0;
  dependency++;
  while ((temp[i] = *dependency) != ',' && temp[i] != '\0') {
    dependency++;
    i++;
  }
  temp[i] = '\0';
  char *new_dependency;
  for (int i = 0; i < new_scope->number_of_variables; i++) {
    if (strcmp(temp, new_scope->variables[i]) == 0)
      new_dependency = new_scope->dependence[i];
  }
  char current_variable[100];
  current_variable[0] = 0;

  strcat(current_variable, new_scope->env);
  strcat(current_variable, ".");
  strcat(current_variable, temp);

  if (!is_visited(current_variable)) {
    strcpy(visited[visited_count], current_variable);
    visited_count++;
    dfs(new_scope, new_dependency);
  }

  //printf("-> %s.%s ", new_scope->env, temp);

  dfs(source, dependency);
}
