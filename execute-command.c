// UCLA CS 111 Lab 1 command execution

// Copyright 2012-2014 Paul Eggert.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http;//www.gnu.org/licenses/>.

#include "command.h"
#include "command-internals.h"
#include <error.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdint.h>

/* FIXME; You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
void execute_command_h(command_t c,int profiling);
void exe_simple_cmd(command_t c,int profiling);
void exe_if_cmd(command_t c,int profiling);
void exe_while_cmd(command_t c,int profiling);
void exe_until_cmd(command_t c,int profiling);
void exe_seq_cmd(command_t c,int profiling);
void exe_subshell_cmd(command_t c,int profiling);
void exe_pipe_cmd(command_t c,int profiling);
void apply_io(command_t c);
double get_timestamp(double* ftime, double* user_time, double* sys_time);
int outputLog(int fd, double col1,double col2, double col3, double col4,command_t c,int pid_t);

int CAPPERLINE=1024;
double NANO=1000000000;
double MICRO=1000000;

int
prepare_profiling (char const *name)
{
  /* FIXME; Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
     int fd;
     fd = open(name, O_WRONLY | O_APPEND | O_CREAT,0666);
     if(fd==-1)
     	error (0, 0, "error opening profiling file");
     return fd;
}

int
command_status (command_t c)
{
  return c->status;
}

void execute_command(command_t c,int profiling)
{
	double starting_time;
    double user_time_start;  	
    double sys_time_start;
    get_timestamp(&starting_time,&user_time_start,&sys_time_start);

	execute_command_h(c,profiling);

	double ending_time;
	double abs_ending_time;
    double user_time_end;
    double sys_time_end;
    abs_ending_time = get_timestamp(&ending_time,&user_time_end,&sys_time_end);
	double real_time = ending_time - starting_time;
	double user_time = user_time_end - user_time_start;
	double sys_time = sys_time_end - sys_time_start;
	if(outputLog(profiling,abs_ending_time,real_time,user_time,sys_time,c,(int)getpid())==-1)
		c->status=1;//non zero exit status
}

void execute_command_h(command_t c,int profiling)
{
	if(c->type==SIMPLE_COMMAND)
		exe_simple_cmd(c,profiling);
	else if(c->type==IF_COMMAND)
		exe_if_cmd(c,profiling);
	else if(c->type==WHILE_COMMAND)
		exe_while_cmd(c,profiling);
	else if(c->type==UNTIL_COMMAND)
		exe_until_cmd(c,profiling);
	else if(c->type==SUBSHELL_COMMAND)
		exe_subshell_cmd(c,profiling);
	else if(c->type==SEQUENCE_COMMAND)
		exe_seq_cmd(c, profiling); 
	else if(c->type==PIPE_COMMAND)
		exe_pipe_cmd(c,profiling);

}

void exe_simple_cmd(command_t c,int profiling)
{
	
	double starting_time;
    double user_time_start;  	
    double sys_time_start;
    get_timestamp(&starting_time,&user_time_start,&sys_time_start);	
	pid_t pid = fork(); //child process id
	if(pid<0)
		error(1, 0, "Unsucessful forking");
	else if(pid==0)
	{
		apply_io(c);
		if(strcmp(c->u.word[0],"exec")==0)
		{
			if(execvp(c->u.word[1], &c->u.word[1]) == -1)
				error(1,0, "%s: fail to open simple command",c->u.word[1]);
		}
		else if(strcmp(c->u.word[0],":")==0) //colon should be treated as true
			_exit(0);
		else if (execvp(c->u.word[0], c->u.word) == -1)
			error(1,0, "%s: fail to open simple command",c->u.word[0]);
	}
	else
	{
		//waitpid to kill child;
		int status;
		if(waitpid(pid, &status, 0) == -1)
			error(1, 0, "Error with child process exiting");
		if(!WIFEXITED(status))
			error(1, 0, "Error with terminating child process");
		c->status = WEXITSTATUS(status);	
		double ending_time;
		double abs_ending_time;
	    double user_time_end;
	    double sys_time_end;

	    abs_ending_time = get_timestamp(&ending_time,&user_time_end,&sys_time_end);
		double real_time = ending_time - starting_time;
		double user_time = user_time_end - user_time_start;
		double sys_time = sys_time_end - sys_time_start;
		outputLog(profiling,abs_ending_time,real_time,user_time,sys_time,c,-1);
	}
}

void exe_while_cmd(command_t c,int profiling)
{
	apply_io(c);
	execute_command_h(c->u.command[0],profiling);
	while(c->u.command[0]->status==0)
	{
		execute_command_h(c->u.command[1], profiling);
		execute_command_h(c->u.command[0], profiling);
	}
	c->status = c->u.command[0]->status;
}

void exe_seq_cmd(command_t c,int profiling)
{
	apply_io(c);
	execute_command_h(c->u.command[0], profiling);
	c->status = c->u.command[0]->status;
	execute_command_h(c->u.command[1], profiling);
	c->status = c->u.command[1]->status;
}

void exe_subshell_cmd(command_t c,int profiling)
{
	apply_io(c);
	//extra fork??
	double starting_time;
    double user_time_start;  	
    double sys_time_start;
    get_timestamp(&starting_time,&user_time_start,&sys_time_start);

	pid_t pid = fork();
	if(pid==-1)
		error(1,0,"forking error");
	else if(pid==0)
	{
		execute_command_h(c -> u.command[0], profiling);
		_exit(c -> u.command[0] -> status);
	}
	else
	{
		int status;
		if (waitpid(pid, &status, 0)==-1) 
			error(0,0,"error in waitpid");
		double ending_time;
		double abs_ending_time;
	    double user_time_end;
	    double sys_time_end;
	    abs_ending_time = get_timestamp(&ending_time,&user_time_end,&sys_time_end);
		double real_time = ending_time - starting_time;
		double user_time = user_time_end - user_time_start;
		double sys_time = sys_time_end - sys_time_start;
		outputLog(profiling,abs_ending_time,real_time,user_time,sys_time,c,pid);
		c -> status = WEXITSTATUS(status);
	}
}

void exe_until_cmd(command_t c,int profiling)
{
	apply_io(c);
	execute_command_h(c->u.command[0],profiling);
	while(c->u.command[0]->status!=0)
	{
		execute_command_h(c->u.command[1],profiling);
		execute_command_h(c->u.command[0],profiling);
	}
	c->status = c->u.command[0]->status;	
}

void exe_if_cmd(command_t c,int profiling)
{
	apply_io(c);
    //case; if A then B fi, and case; if A then B else C fi
    execute_command_h(c->u.command[0],profiling);
	//if false then b fi => a
	if((c->u.command[0]->status) !=0 && c->u.command[2]==NULL)
	{
		c->status = 0; //according to TA: if false, if command should exit true
	}
	//if true then b (else c) fi =>b
    else if((c->u.command[0]->status) == 0)
    {
        execute_command_h(c->u.command[1],profiling);
        c->status = c->u.command[1]->status;
    }
	// if false (then b) else c ==> c
    else if(c->u.command[2]!=NULL)
    {
        execute_command_h(c->u.command[2],profiling);
        c->status = c->u.command[2]->status;
    } 
}

void exe_pipe_cmd(command_t c,int profiling)
{
    //A|B
    int status;
    int filedes[2];
    pid_t pid_right;
    pid_t pid_left;
    pid_t get_return_pid;
    if (pipe(filedes) == -1 )
        error (1, 0, "Error when piping the file");
    
    pid_right = fork(); //create the child process

    if(pid_right<0) error(1, 0, "Unsucessful forking");
    
    else if(pid_right>0) //successfully forking in the parent process
    {
        pid_left=fork();
        if(pid_left<0) error(1, 0, "Unsucessful forking");
        else if(pid_left>0)
        {
            close(filedes[0]);
            close(filedes[1]); //close and waitpid to wait the child to end
            
            //-1 means all children
            //kill 1st child that terminate
            get_return_pid = waitpid(-1,  &status, 0); 
            
            //kill 2nd child
            if(get_return_pid== pid_right ) //return,B's child end
            {
                c->status = status;
                waitpid(pid_left, &status, 0); //wait for A child end
                return;
            }
            else if(get_return_pid== pid_left)
            {
                c->status = status;
                waitpid(pid_right, &status, 0);
                return;
            }
        }
        else if(pid_left==0 )
        {
            close(filedes[0]); //read nothing
            if (dup2(filedes[1], 1)!= -1)    //direct output to this file
            {
                execute_command_h(c->u.command[0], profiling);
                 _exit(c->u.command[0]->status);
            }
            else
                error (1, 0,  "dup2 error");
        }
    }
    else if (pid_right == 0)
    {
        // in the B child process, command B in the pipe
        close(filedes[1]); //nothing writing
        if( dup2(filedes[0], 0)!= -1 ) //read to standin
        {
            execute_command_h(c->u.command[1], profiling);
            _exit(c->u.command[1]->status);
        }
        else
            error (1, 0,  "dup2 error ");
    }
  
}

void apply_io(command_t c)
{
	int fdin;
	int fdout;

	if(c->input!=NULL)
	{
		if((fdin=open(c->input, O_RDONLY)) ==-1)
			error(1, 0, "Error when opening input file");
		if(dup2(fdin, 0)==-1) //direct this file to input
			error(1, 0, "Error in dup2");
		close(fdin);
	}
	if(c->output!=NULL)
	{
		//O_TRUNC makes sure to delete what's already in the file
		//same as sh
		if((fdout=open(c->output, O_WRONLY|O_CREAT|O_TRUNC, 0664)) ==-1)
			error(1, 0, "Error when opening output file");
		if(dup2(fdout, 1)==-1) //direct output to this file
			error(1, 0, "Error in dup2");
		close(fdout);
	}	
}

double get_timestamp(double* ftime, double* user_time, double* sys_time)
{
	struct timespec abstime;
	struct rusage parent_use;
	struct rusage child_use;
	if(clock_gettime(CLOCK_MONOTONIC, &abstime)==-1)
	{
		error(1, 0, "clcok_gettime error");
	}
    //for timeval, tv_usec is in microseconds
    //for timespec,tv_nsec is in nanoseconds
	*ftime = abstime.tv_sec+(double)abstime.tv_nsec/NANO;
	getrusage(RUSAGE_SELF, &parent_use);
    getrusage(RUSAGE_CHILDREN, &child_use);
    *user_time=
    	parent_use.ru_utime.tv_sec + child_use.ru_utime.tv_sec + 
    	(parent_use.ru_utime.tv_usec + child_use.ru_utime.tv_usec)/MICRO;
    *sys_time=
    	parent_use.ru_stime.tv_sec + child_use.ru_stime.tv_sec + 
    	(parent_use.ru_stime.tv_usec + child_use.ru_stime.tv_usec)/MICRO;
    if(clock_gettime(CLOCK_REALTIME, &abstime)==-1)
	{
		error(1, 0, "clcok_gettime error");
	}
	return abstime.tv_sec + (double)abstime.tv_nsec/NANO;
}

int outputLog(int fd, double col1,double col2, double col3, double col4,command_t c,int pid)
{
	char buff[CAPPERLINE];
	//write to buff instead of stdout
	//what if exceeds 1024?? ==> discarded
	if(pid==-1)
	{
		char** w = c->u.word;
		char cmd[CAPPERLINE];
		cmd[0]='\0'; //static array will not be deleted until program ends, this enforces empty array
		strcat(cmd, *w);
		w++;
	    while ((*w)!=NULL) 
	    {
	    	strcat(cmd, " ");
	        strcat(cmd, *w);
	        w++;
	    }
	    if (c->input!=NULL) 
	    {
	        strcat(cmd, "<");
	        strcat(cmd, c->input);
	    }
	    if (c->output!=NULL) 
	    {
	        strcat(cmd, ">");
	        strcat(cmd, c->output);
	    }
		snprintf(buff,CAPPERLINE,"%f %f %f %f %s\n", col1, col2, col3, col4,cmd);
	}
	else
	{
		snprintf(buff,CAPPERLINE,"%f %f %f %f [%d]\n", col1, col2, col3, col4,pid);
	}
	/*char **w = c->u.word;
	printf ("%s", *w);
	while (*++w)
	  printf (" %s",*w);*/
	if(write(fd, buff, strlen(buff))==-1)
		return -1;
	return 0;
}



































