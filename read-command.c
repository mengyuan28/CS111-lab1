// UCLA CS 111 Lab 1a command reading


#include "alloc.h"
#include "command.h"
#include "command-internals.h"
#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int CAPACITY = 12;
int BASIC_CMD_SIZE = 16;
int LINENUM=1;
/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
/*
 struct command
 {
 enum command_type type;
 
 // Exit status, or -1 if not known (e.g., because it has not exited yet).
 int status;
 
 // I/O redirections, or null if none.
 char *input;
 char *output;
 
 union
 {
 // For SIMPLE_COMMAND.
 char **word;
 
 // For all other kinds of commands.  Trailing entries are unused.
 // Only IF_COMMAND uses all three entries.
 struct command *command[3];
 } u;
 };

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;
*/

struct command_stream  //command_stram has the command tree
{
  char **words; // the word array [ [] [] [] [] []]
  int index;    //index of the word in the array
  int word_num; // number of words（char) in the total command stream
  int now_root;
  int before_root; //use before_root for the sequence commands. Child sequence commands directly under a root sequence command should still be considered root
};

typedef enum
{
    WORD,		    //"one or more adjacent characters that are ASCII letters (either upper or lower case), digits, or any of: ! % + , - . / : @ ^ _"
    LEFTBRA,		// (
    RIGHTBRA,		// )
    PIPE,			// |
    SEMI,			// ;
    IN,			    // <
    OUT,			// >
    NEWLINE,
    IF,
    THEN,
    ELSE,
    FI,
    WHILE,
    DO,
    DONE,
    UNTIL,
    INVALID,		// Invalid token (e.g. <<<)
} token_type;

typedef struct
{
    token_type type;
    char* word;
} token;



static int
is_empty(command_t new_cmd) {
  return new_cmd->type == SIMPLE_COMMAND && !strcmp(new_cmd->u.word[0], "");
}

static int is_word_char(int c)  //all possible ACII letters and possible words in the script
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
    || (c == '@') || (c == '^') || (c == '!') || (c == '%')
    || (c == '+') || (c == ',') || (c == '-') || (c == '.')
    || (c == '/') || (c == ':') ||(c == '_');
}

static int is_special_token(int c) //list of all special char, ) ( > < |
{
  return c == ';' || c == '|' || c == '(' || c == ')' || c == '<' || c == '>' || c == '\n' || c == '#';
}

static int is_whitespace(char c) //see if it is whitespace
{
    return (c==' ') || (c=='\t');
}

static char * processing (int (*get_next_byte) (void*), void *file_stream) // read all the valid word or special token and put it into the buffer,  returns 0 when reaches EOF (finish)   get_next_byte will read the next char from the file stream

{

    size_t length = CAPACITY;
  char current_char;
  int found_word = 0;
  int found_spec = 0;
  int buffer_size = 0;                           // buffer_size: the number of character in a word
  char *buffer = (char*)checked_malloc(sizeof(char)*length);
  
    
  while ((current_char = (*(get_next_byte))(file_stream)) != EOF)
  {
    if(current_char=='#' && (buffer_size==0 || is_whitespace(buffer[buffer_size-1]) || buffer[buffer_size-1]=='\n'))
    {
        while (((current_char = (*(get_next_byte))(file_stream)) != EOF) && (current_char != '\n'))
          continue;
        if(current_char==EOF)
          break;
    }
      if (is_word_char(current_char) || is_special_token(current_char) || is_whitespace(current_char))
      {
    if ((!is_word_char(current_char) && found_word) || (!is_special_token(current_char) && found_spec))
    {
      if (current_char != EOF) fseek(file_stream, -1, SEEK_CUR); // sets the file position of the stream to the given offset.SEEK_CUR	Current is the position of the file pointer
      break;
    }
      if (is_word_char(current_char) || is_special_token(current_char) ) {
          if (current_char=='\n') LINENUM+=1;
      if (is_word_char(current_char) && !found_word) found_word = 1;
      if (is_special_token(current_char) && !found_spec) found_spec = 1;
      buffer[buffer_size] = current_char;
      buffer_size+=1;
      //reallocating
      if (buffer_size*sizeof(char) >= length)
      {
	buffer = (char*)checked_grow_alloc(buffer, &length); //reallocate the memory for the buffer
      }
      if (is_special_token(current_char)) break; // special token is 1 char long
    }
  }
      else
      {
          error (1, 0, "Line number: %d, Invalid character",LINENUM);
      }
  }
  if (current_char == EOF && !found_word && !found_spec)
  {
    free(buffer);
    return 0;
  }
  buffer[buffer_size] = 0;
  return buffer;
}



command_stream_t make_command_stream (int (*get_next_byte) (void *), void *get_next_byte_argument)// get_next_byte_argument is the to read the next command in buffer
{
 
  command_stream_t stream = (command_stream_t)checked_malloc(sizeof(struct command_stream));
  stream->index = 0;
  stream->word_num = 0;
  stream->now_root = 0;
  stream->before_root = 0;
  size_t command_size = BASIC_CMD_SIZE;
    
  stream->words = (char**)checked_malloc(sizeof(char*)*command_size);

  char *temp; //use the return buffer in processing function
  int do_break = 0;

  while ((temp= processing(get_next_byte, get_next_byte_argument))) {
    while (!strcmp(temp, "#"))  // if the current_char is #
    {
        if ((stream->word_num > 0) && strcmp(stream->words[stream->word_num-1],"\n")) error (1, 0, "Line number: %d,unrecognized char",LINENUM);
      while (strcmp(temp, "\n"))
      {
	free(temp);
	temp = processing(get_next_byte, get_next_byte_argument);
          if (!temp)
        {
            do_break = 1;
            break;
        }
      }
      free(temp); // might free 0x0, which is okay
      temp = processing(get_next_byte, get_next_byte_argument);
      if (!temp) do_break = 1;
      if (do_break) break;
    }
    if (do_break) break;
    stream->words[stream->word_num++] = temp;
      //reallocate the size
    if (((stream->word_num)*sizeof(char*)) >= command_size) {
      stream->words = (char**)checked_grow_alloc(stream->words, &command_size);
     
    }
  }

  return stream;
}


command_t read_command (command_stream_t s) //read the simple command and compound command
{
  int is_root = s->now_root;
  int before_root = s->before_root;
  int word_num = s->word_num;
 s->before_root = 0;

  if (s->now_root) s->now_root = 0;
  

  command_t new_cmd = (command_t)checked_malloc(sizeof(struct command));
  new_cmd->input = NULL;
  new_cmd->output = NULL;
  new_cmd->type = SIMPLE_COMMAND;
    
  new_cmd->u.word = (char**)checked_malloc(sizeof(char*));
  new_cmd->u.word[0] = ""; // the word to be entered, initialized by 0
  new_cmd->u.word[1] = 0;

  if (s->index >= s->word_num) return 0;
  
  char *curr = s->words[s->index++];

  int flag = 0;
    
    
  //Everytime check the index whether is smaller than the word_num
  while (!strcmp(curr, "\n"))  //when enters the newline
  {
    if (s->index >= word_num)
    {
      if (is_root) return 0; // which means the value of s->root is non-zero return 0, there should be no comment left
      else return new_cmd; // return the command tree
    }
    curr = s->words[s->index++]; //putin the nex char
  }

  if (!strcmp(curr, "while")) //case: while A do B done, u.command[0]=A, u.command[1]=B
  {
        new_cmd->type = WHILE_COMMAND;
        free(new_cmd->u.word);
        
        new_cmd->u.command[0] = read_command(s);
        if (s->index >= word_num) error (1, 0, "Line number: %d,  Invalid while syntax",LINENUM);
        curr = s->words[s->index++];
        
        if (strcmp(curr, "do")) error (1, 0, "Line number: %d,  Invalid while syntax,lack do",LINENUM); //if there is no do
        
        new_cmd->u.command[1] = read_command(s);
        if (s->index >= word_num) error (1, 0, "Line number: %d,  Invalid while syntax",LINENUM);
        curr = s->words[s->index++];
        
        if (strcmp(curr, "done")) error (1, 0, "Line number: %d,  Invalid while syntax, may lack done",LINENUM); //if there is no down function
        if (s->index >= word_num) return new_cmd;      // the command is completed reaching EOF
        curr = s->words[s->index++];
    }
    
  else if (!strcmp(curr, "if"))  //case if A then B fi, u.command[0]=A, u.command[1]=B; case if A then B else C fi, u.command[0]=A, u.command[1]=B, u.command[2]=C
  {
    new_cmd->type = IF_COMMAND;
    free(new_cmd->u.word);

    new_cmd->u.command[0] = read_command(s);
    if (s->index >= word_num) error (1, 0, "Line number: %d,  Invalid if syntax",LINENUM);
    curr = s->words[s->index++];
      
    if (strcmp(curr, "then")) error (1, 0, "Line number: %d,  Invalid if syntax, lack of then",LINENUM); // ifthere is no then

    new_cmd->u.command[1] = read_command(s);
    if (s->index >= word_num) error (1, 0, "Line number: %d,  Invalid if syntax",LINENUM);
    curr = s->words[s->index++];
      
    if (strcmp(curr, "fi") && strcmp(curr, "else")) error (1, 0, "Line number: %d,  Invalid if syntax,lack of fi or else",LINENUM); //if there is no else or fi
    if (!strcmp(curr, "else"))
    {
      new_cmd->u.command[2] = read_command(s);
      if (s->index >= word_num) error (1, 0, "Line number: %d,  Invalid if syntax",LINENUM);
      curr = s->words[s->index++];
        
      if (strcmp(curr, "fi")) error (1, 0, "Line number: %d,  Invalid if syntax, lack of fi",LINENUM); //if there is no fi
    }
    else new_cmd->u.command[2] = 0; //which means we do not need command[2] here, sice we do not use else
    
    if (s->index >= word_num) return new_cmd; // the command is completed reaching EOF
    curr = s->words[s->index++];
      
  }
  
  else if (!strcmp(curr, "until")) //deal the case until A do B done, u.command[0]=A, u.command[1]=B
  {
      new_cmd->type = UNTIL_COMMAND;
      free(new_cmd->u.word);
      
      new_cmd->u.command[0] = read_command(s);
      if (s->index >= word_num) error (1, 0, "Line number: %d,  Invalid until syntax",LINENUM);
      curr = s->words[s->index++];
      
      if (strcmp(curr, "do")) error (1, 0, "Line number: %d,  Invalid until syntax, may lack do",LINENUM); ////if there is no do function
      
      new_cmd->u.command[1] = read_command(s);
      if (s->index >= word_num) error (1, 0, "Line number: %d,  Invalid until syntax",LINENUM);
      curr = s->words[s->index++];
      
      if (strcmp(curr, "done")) error (1, 0, "Line number: %d,  Invalid until syntax, may lack done",LINENUM); //if tehre is no done
      if (s->index >= word_num) return new_cmd; // the command is completed reaching EOF
      curr = s->words[s->index++];
  }
    
  else if (!strcmp(curr, "("))  //deal with subshell problem
  {
    new_cmd->type = SUBSHELL_COMMAND;
    free(new_cmd->u.word);

    new_cmd->u.command[0] = read_command(s); 
    if (s->index >= word_num) error (1, 0, " Line number: %d,  Invalid subshell syntax",LINENUM); //the subshell is incomplete ( EOF
      
    curr = s->words[s->index++];  //read next char
      
    if (strcmp(curr, ")")) error (1, 0, "Line number: %d,  Invalid subshell syntax",LINENUM); //which means subeshell is not complete
    if (s->index >= word_num) return new_cmd; // if the stream reach EOF after completed command
      
    curr = s->words[s->index++]; //read the next char
  }
    
  else if (!strcmp(curr, "\n"))
  { // shouldn't ever enter this region
    if (s->index >= word_num) error (1, 0, "Line number: %d, Unexpected token",LINENUM);
    return read_command(s);
  }
    
  else if (!strcmp(curr, "#")) //need to be modified
  {
    do {
      if (s->index >= word_num)
    {
      if (is_root) return 0; // no more commands
      else return new_cmd; // return empty command.
    }
    curr = s->words[s->index++];
    }while (strcmp(curr, "\n")); //while it is not new line
      
  }
    
  else if(!strcmp(curr, "<") || !strcmp(curr, ">") || !strcmp(curr, "do") || !strcmp(curr, "done") ||!strcmp(curr, "|") || !strcmp(curr, ";") || !strcmp(curr, "then") || !strcmp(curr, "fi") || !strcmp(curr, "else") ||  !strcmp(curr, ")"))
  {
      error (1, 0, "Line number: %d,Invalid token used",LINENUM);
  }
    
  else {
    new_cmd->type = SIMPLE_COMMAND; //redundant but clear, the lefte
    int i = 0;
    size_t command_size = BASIC_CMD_SIZE;
    free(new_cmd->u.word);
    new_cmd->u.word = (char**)checked_malloc(sizeof(char*)*command_size);
    while (strcmp(curr, ";")&& strcmp(curr, "\n")  && strcmp(curr, "<") && strcmp(curr, "|") && strcmp(curr, ">") && strcmp(curr, ")")) {
      new_cmd->u.word[i] = curr;
      
      if (s->index >= word_num) return new_cmd; // return the simple command
      curr = s->words[s->index++];
      i++;
      if ((i*sizeof(char*)) >= command_size) {
	new_cmd->u.word = (char**)checked_grow_alloc(new_cmd->u.word, &command_size);
      }
    }
    new_cmd->u.word[i] = 0;
  }
    
  // now curr should have the following possibles ";", "|", "\n", "<", ">", ")" and invalid
  if  (strcmp(curr, ";") && strcmp(curr, "|") && strcmp(curr, "<") && strcmp(curr, ">") && strcmp(curr, "\n")) //if there is no
  {
    if (!strcmp(curr, ")"))
    { // exception. ")" can come without a preceding ; or \n
      s->index--;
      return new_cmd;
    }
    else {
      error (1, 0, "Line number: %d,  Invalid syntax. Missing a semicolon or newline?",LINENUM);
    }
  }

  // I/O reduction
  if (s->index >= word_num) return new_cmd;
  if (!strcmp(curr, "<"))
  {
    if (s->index >= word_num) error (1, 0, "Line number: %d,Redirection is incomplete",LINENUM);
    curr = s->words[s->index++];
      
    if (is_special_token(curr[0])) error (1, 0, "Line number: %d,Unexpected end of command",LINENUM);
    new_cmd->input = curr;
      
    if (s->index >= word_num) return new_cmd; // return the command
    curr = s->words[s->index++];
  }
  if (!strcmp(curr, ">"))
  {
    if (s->index >= word_num) error (1, 0, "Line number: %d,Redirection is incomplete",LINENUM);
    curr = s->words[s->index++];
      
    if (is_special_token(curr[0])) error (1, 0, "Line number: %d,Unexpected end of command",LINENUM);
    new_cmd->output = curr;
      
    if (s->index >= word_num) return new_cmd; // return completed command
    curr = s->words[s->index++];
  }

    
  if (!strcmp(curr, ";") || !strcmp(curr, "\n")) //which means sequence command a
  {
    if (is_root|| before_root )
    {
      s->before_root = 1;
      is_root = 1;
      
    }
    if (s->index >= word_num) return new_cmd; // return the complete command
      
    if (!strcmp(curr, "\n")) flag = 1;
    curr = s->words[s->index]; // only check the next word
      
    if (flag && !strcmp(curr, "\n") && is_root) { // new command tree!!!
      return new_cmd;
    }
      
    while (!strcmp(curr, "\n"))
    {
      if (s->index >= word_num-1) //since the new line
      {
	if (is_root) return 0; // no more commands
	else return new_cmd; //
      }
      curr = s->words[++s->index];
    }
   if (!strcmp(curr,"then") || !strcmp(curr, "fi") || !strcmp(curr, "else") || !strcmp(curr, "do") || !strcmp(curr, "done") || !strcmp(curr,")"))
   {
      return new_cmd;
    }

    if (is_empty(new_cmd)) return read_command(s);
    else {
      command_t a_comm = read_command(s);
      if (is_empty(a_comm)) return new_cmd;
      else {
	command_t top = (command_t)checked_malloc(sizeof(struct command));
	top->type = SEQUENCE_COMMAND;
	command_t temp = new_cmd;
	new_cmd = top;
	top->u.command[0] = temp;
	top->u.command[1] = a_comm;
      }
    }
  }
  else if (!strcmp(curr, "|"))  {

    if (s->index >= word_num || is_empty(new_cmd)) error (1, 0, "Line number: %d,  Invalid pipe syntax",LINENUM);
        command_t a_comm = read_command(s);
        
        if (is_empty(a_comm)) error (1, 0, "Line number: %d,  Invalid pipe syntax",LINENUM);
            
            command_t top = (command_t)checked_malloc(sizeof(struct command));
            top->type = PIPE_COMMAND;
            command_t temp = new_cmd;
            new_cmd = top;
            top->u.command[0] = temp;
            top->u.command[1] = a_comm;
            if (top->u.command[0]->type == SEQUENCE_COMMAND) //  to see right
            {
                
                command_t temp1 = new_cmd;
                new_cmd = new_cmd->u.command[0];
                temp1->u.command[0] = new_cmd->u.command[1];
                new_cmd->u.command[1] = temp1;
            }
            else if (top->u.command[1]->type == SEQUENCE_COMMAND) //to see left
            {
                command_t temp1 = new_cmd;
                new_cmd = new_cmd->u.command[1];
                temp1->u.command[1] = new_cmd->u.command[0];
                new_cmd->u.command[0] = temp1;
            }
}

  else s->index--; //

  return new_cmd;
}

command_t read_command_stream (command_stream_t s)
{
  s->now_root = 1;
  command_t new_cmd = read_command(s);
  return new_cmd;
}
