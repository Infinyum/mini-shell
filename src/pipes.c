#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/stat.h>

#include "internal_commands.h"
#include "pipes.h"
#include "util.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////																															/////
/////														      PIPES HANDLING												/////
/////																															/////
/////															   	MADE BY 													/////
/////												 	ROBIN MALMASSON AND THOMAS VON ASCHEBERG								/////
/////																															/////
/////												  	  SHELL PROJECT - POLYTECH 2017-2018									/////
/////																															/////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
 * Creates an array of all the commands separated by pipes, then
 * calls loop_pipe() to execute those
 * @param args_array containing all the arguments of all the commands
 * @param args_sz is the size of args_array
 * @param pipe_pos corresponds to the position of the name of the second command
 * @return the return statement of the loop_pipe() function i.e. -1, 0 or 1
 */
int pipe_commands_array(char **args_array, int args_sz, int pipe_pos, char* ifile, char* ofile, char* efile, int ntok_read, char* path)
{
	char ***cmd;
	int next_pipe_pos = -1;
	int first_cmd_sz = -1;

	// If no other pipe was found, meaning there is one pipe in total
	// i.e two different commands
	if ((next_pipe_pos = next_pipe(args_array)) == -1)
	{
		// Creating an args_array for each command
		char **args_array1 = calloc(pipe_pos + 1, sizeof(char*));
		for (size_t i = 0 ; i < pipe_pos ; i++)
			args_array1[i] = args_array[i];
		args_array1[pipe_pos] = NULL;
		first_cmd_sz = pipe_pos+1;

		char **args_array2 = calloc(args_sz - pipe_pos, sizeof(char*));
		for (size_t i = 0 ; i < (args_sz - pipe_pos) ; i++)
			args_array2[i] = args_array[pipe_pos + i];

		cmd = calloc(3, sizeof(char**));
		cmd[0] = args_array1;
		cmd[1] = args_array2;
		cmd[2] = NULL;

	}

	// Otherwise there is at least 3 commands
	else {
		
		int cmdarr_sz = 2, ncmd_read = 2;
		cmd = calloc(cmdarr_sz, sizeof(char**));


		// Dealing with the first command
		cmd[0] = calloc(pipe_pos + 1, sizeof(char*));
		for (size_t i = 0 ; i < pipe_pos ; i++)
			cmd[0][i] = args_array[i];
		cmd[0][pipe_pos] = NULL;
		first_cmd_sz = pipe_pos+1;

		// Dealing with the second command
		int cmd_sz = next_pipe_pos - pipe_pos;
		cmd[1] = calloc(cmd_sz + 1, sizeof(char*));
		for (size_t i = 0 ; i < cmd_sz ; i++)
			cmd[1][i] = args_array[pipe_pos + i];
		cmd[1][cmd_sz] = NULL;


		// The position of next command to handle
		pipe_pos = next_pipe_pos + 1;


		// Dealing with all the remaining commands (except the last one)
		char **argument = NULL;
		while ((next_pipe_pos = next_pipe(args_array + pipe_pos)) != -1)
		{
			cmd_sz = next_pipe_pos;
			argument = calloc(cmd_sz + 1, sizeof(char**));

			for (size_t i = 0 ; i < cmd_sz ; i++)
				argument[i] = args_array[pipe_pos + i];
			argument[cmd_sz] = NULL;

			if (cmdarr_sz <= ncmd_read) // not enough memory
			{
				cmdarr_sz = 2*cmdarr_sz + 1; // limit the memory allocations to log(n)
				cmd = realloc(cmd, cmdarr_sz*sizeof(char**));
			}
			cmd[ncmd_read++] = argument;

			pipe_pos += next_pipe_pos + 1;
		}


		if (cmdarr_sz <= ncmd_read + 1) // not enough memory
			// +2 for the remaining command and the NULL at the end
			cmd = realloc(cmd, (cmdarr_sz+2)*sizeof(char**));
		

		// Dealing with the last command
		cmd_sz = args_sz - pipe_pos - 1;
		cmd[ncmd_read] = calloc(cmd_sz+1, sizeof(char*));
		for (size_t i = 0 ; i < cmd_sz ; i++)
			cmd[ncmd_read][i] = args_array[pipe_pos + i];
		cmd[ncmd_read][cmd_sz] = NULL;
		ncmd_read++;

		if (cmdarr_sz <= ncmd_read) // not enough memory
			cmd = realloc(cmd, (cmdarr_sz+1)*sizeof(char**));
		cmd[ncmd_read] = NULL;
	}

	return loop_pipe(cmd, first_cmd_sz, ifile, ofile, efile, ntok_read, path);
}



/**
 * Simultaneously redirects the input and output of each
 * command in the array cmd and executes it
 * @param cmd : an array containing the array of arguments
 *				of each command
 * @return -1 if there was an error, res otherwise (0 or 1)
**/
int loop_pipe(char ***cmd, int first_cmd_sz, char* ifile, char* ofile, char* efile, int ntok_read, char* path) 
{
	int fd[2];
	pid_t pid;
	int fd_in = 0;
	int i_desc = -1, o_desc = -1, e_desc = -1;
	int res = 0;

	//char ****cmd_backup = cmd; // To free everything

	if (internal_command_search(cmd[0][0]) != -1)
	{
		if (pipe(fd) == -1)
		{
			fprintf(stderr, "ERROR: issue while creating the pipe\n");
			return -1;
		}

		int saved_stdin = dup(0), saved_stdout = dup(1);
		if (ifile)
		{
			i_desc = open(ifile, O_RDONLY);
			dup2(i_desc, fileno(stdin));
		}

		// Setting the output to the entrance of the pipe
    	dup2(fd[1], 1);

		res = internal_command(cmd[0], first_cmd_sz, first_cmd_sz, path, ifile);
		if (res == -1) return -1;
		
		if (ifile)
		{
			dup2(saved_stdin, fileno(stdin));
			close(saved_stdin);
			close(i_desc);
		}

		// Setting back the stdout to the screen
		dup2(saved_stdout, 1);

		fd_in = fd[0]; // save the input for the next command
	}
	
	while (*cmd != NULL)
	{
		if (pipe(fd) == -1)
		{
			fprintf(stderr, "ERROR: issue while creating the pipe\n");
			return -1;
		}

		pid = fork();
		if (pid == -1)
			return -1;
		
		else if (pid == 0) // Child process
		{
			// If it is the first command and there is an entry redirection
			if (fd_in == 0 && ifile)
			{
				i_desc = open(ifile, O_RDONLY);
				dup2(i_desc, fileno(stdin));
			}
			else dup2(fd_in, 0); // Change the input according to the last command
			
			if (*(cmd + 1) != NULL)
		    	dup2(fd[1], 1);

		    else // If it is the last command 
		    {
		    	if (ofile)
				{
					o_desc = open(ofile, O_CREAT | O_WRONLY, 0644);
					dup2(o_desc, fileno(stdout));
				}
				if (efile)
				{
					e_desc = open(efile, O_CREAT | O_WRONLY, 0644);
					dup2(e_desc, fileno(stderr));
				}
		    }

			close(fd[0]);
		  	execvp((*cmd)[0], *cmd);
		  	
		  	// In case there is an issue
		  	return -1;
		}
		else // Parent process
		{
			wait(NULL);
			close(fd[1]);
			fd_in = fd[0]; // save the input for the next command
			cmd++;
		}

	}

	//pipe_free(cmd_backup);
	return res;
}



/**
 * Looks for a pipe in args_array
 * @param args_array: an array of arguments which may contain some pipes
 * @return the position of the pipe if there is one, -1 otherwise
 */
int next_pipe(char **args_array)
{
	size_t i = 0;
	while (args_array[i])
	{
		if (args_array[i][0] == '|')
			return i;
		i++;
	}
	return -1;
}



/**
 * This function calls free() for every item in cmd
 * In the end, it was not used because of errors
 */
void pipe_free(char ***cmd)
{
	if (cmd)
	{
		int i;
		for (i = 0 ; *(cmd+i) != NULL ; i++)
		{
			int j;
			for (j = 0 ; *(*(cmd+i)+j) != NULL ; j++)
			{
				free(*(*(cmd+i)+j));
			}
			free(*(cmd+i));
		}
		free(cmd);
	}
}