#define _GNU_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
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
/////													 SHELL PROJECT FOR THE SYSTEM CLASS										/////
/////																															/////
/////															   	MADE BY 													/////
/////												 ROBIN MALMASSON	| 	THOMAS VON ASCHEBERG								/////
/////																															/////
/////												  	SHELL PROJECT - POLYTECH 2017-2018										/////
/////																															/////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


char *username = NULL, *path = NULL, *cmd_prompt = NULL;
char machine[HOST_NAME_MAX+1];


/**
 * Sets the value of the prompt line based on the current path, username and machine name
 */
void set_prompt()
{
	// Getting info on the current environment
	username = getenv("USER");
	if (path != NULL)
		free(path);
	path = get_current_dir_name(); // _GNU_SOURCE is useful for this call
	
	if (gethostname(machine, HOST_NAME_MAX) == -1 || !username || !path) {
		fprintf(stderr, "Error while getting information about the environment\n");
		exit(EXIT_FAILURE);
	}

	// Resetting cmd_prompt
	if(cmd_prompt != NULL)
		free(cmd_prompt);

	// Creating the new command prompt line
	cmd_prompt = (char*)calloc(strlen(username) + strlen(path) + strlen(machine) + 5, sizeof(char));
	sprintf(cmd_prompt, "%s@%s:%s> ", username, machine, path);
}



int main(int argc, char** argv)
{
	set_prompt();
	const size_t BUF_SZ = 64;
	char *buffer = malloc(BUF_SZ);

	while (true)
	{
		// Read a line (until \n)
		char* line = calloc(1,1); // leading zero for strcat
		int line_sz = 0;
		bool eol = false;

		printf("%s", cmd_prompt); 

		while (true)
		{
			if (!fgets(buffer, BUF_SZ, stdin))
			{
				fprintf(stderr, "Error while reading the line\n");
				exit(EXIT_FAILURE);
			}
			size_t buf_sz = strlen(buffer); // effective size

			// If there is a backslash before the newline character
			if (buffer[buf_sz-2] == '\\') {
				buffer[buf_sz-2] = '\0';
				buf_sz -= 2;
				eol = false;
			}
			else if (buffer[buf_sz-1] == '\n')
			{
				buffer[buf_sz-1] = '\0';
				buf_sz--;
				eol = true;
			}
			
			line_sz += buf_sz;
			line = realloc(line, line_sz+1);
			strcat(line, buffer);
			if (eol)
				break;
		}

		// Parse line into tokens
		bool pipe_tok = false;
		int ntok_read = 0, args_sz = 0, pipe_pos = -1, index = 0;
		char **args_array = NULL;
		char *token = findToken(line,' ',&index), *argument = NULL;
		char *ifile = NULL, *ofile = NULL, *efile = NULL;
		
		// The internal command corresponding to exit
		if (!strcmp(line, "exit"))
		{
			free(cmd_prompt);
			free(path);
			free(buffer);
			free(argument);
			free(args_array);
			free(token);
			free(line);
			exit(EXIT_SUCCESS);
		}

		while (token != NULL)
		{
			// If the first character is a quotation mark
	    	if (token[0] == '\"')
	    	{			
				// If an empty string is in between quotation marks ==> the command fails
				if (!strcmp(token, "\"\"") || !strcmp(token, "\""))
				{
					fprintf(stderr, "ERROR: empty string");
					break;
				}

				// We create our argument (and keep the size of it near to use it)
				size_t arg_sz = strlen(token);
				argument = calloc(arg_sz, sizeof(char));

				// Boolean to know if only have one word between the two quotes
				int one_word = false;
				
				// one word argument into quotes
				if(token[arg_sz-1] == '\"')
				{
					strncpy(argument, token+1, arg_sz-2);
					one_word = true;
				}
				// if more than one word
				else
				{
					// we create the substring (without the quotation marks)
					strcpy(argument, token+1);

					// we add a space after the first word
					char* space_to_add = " ";
					strcat(argument, space_to_add);
					
					// we are looking for the second quotation mark
					token = findToken(line, '\"', &index);
				}
		
				// If we get to the end of the command line and have not found the end of argument
				if (token == NULL && !one_word)
				{
					fprintf(stderr, "ERROR: missing the ending quote");
					break;
				}
				// Otherwise we found it:
				else if (token != NULL && !one_word)
				{
					// just concat the token to the current argument
					arg_sz += strlen(token)-1;
					argument = realloc(argument, arg_sz*sizeof(char));
					strcat(argument, token);
				}		

				// Finally we add our argument to the argument array
				// if our argument array is too small ==> we have to allow more space for it
				if (args_sz <= ntok_read) 
				{
					args_sz = 2*args_sz + 1; //limit the memory allocations to log(n)
					args_array = realloc(args_array, args_sz*sizeof(char*));
				}

				// we add our homemade argument to the arguments array
				args_array[ntok_read] = argument;
				ntok_read++;

				// then we continue to look for other arguments
				token = findToken(line, ' ', &index);
				continue;
			}

			// Redirections ?
			if (!strcmp(token,"<") || !strcmp(token,">") || !strcmp(token, "2>"))
			{
				char redirect = token[0];
				token = findToken(line, ' ', &index);
				redirect == '<' ? (ifile=token) : (redirect == '>' ? (ofile=token) : (efile=token));
			}
			// Pipe ? Notice that we enter this if statement only for the first pipe in the line
			else if (token[0] == '|' && !pipe_tok)
			{
				pipe_tok = true;
				// Corresponds to the position of the following token in args_array
				// (which is the beginning of the second command)
				pipe_pos = ntok_read;
			}
			else
			{
				if (args_sz <= ntok_read) // not enough memory
				{
					args_sz = 2*args_sz + 1; // limit the memory allocations to log(n)
					args_array = realloc(args_array, args_sz*sizeof(char*));
				}
				args_array[ntok_read++] = token;
			}
			token = findToken(line, ' ', &index);
		}

		free(token);

		if (args_sz <= ntok_read)
			args_array = realloc(args_array, (args_sz+1)*sizeof(char*));
		args_array[ntok_read] = NULL; // marker for execv to know where to end...

		
		if (pipe_tok) // If there is at least one pipe entered
		{
			pid_t pid = fork();
			if (pid == 0) // Child
			{
				if (pipe_commands_array(args_array, ntok_read + 1, pipe_pos, ifile, ofile, efile, ntok_read, path) == 1)
					set_prompt(); // If the prompt line has to be changed
				
				exit(EXIT_SUCCESS); // To exit the sub-process created by the pipes
			}
			else // Parent
				wait(NULL);

		} 
		else // No pipe
		{
			// Saving current stdin, stdout, stderr
			int saved_stdin = dup(0), saved_stdout = dup(1), saved_stderr = dup(2);

			// Doing redirections
			int i_desc = -1, o_desc = -1, e_desc = -1;
			if (ifile)
			{
				i_desc = open(ifile, O_RDONLY);
				dup2(i_desc, fileno(stdin));
			}
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

			// Running this function to execute the external command if there is one
			int res = internal_command(args_array, args_sz, ntok_read, path, ifile);

			// Restoring everything back to normal
			if (ifile)
			{
				dup2(saved_stdin, fileno(stdin));
				close(saved_stdin);
				close(i_desc);
			}
			if (ofile)
			{
				dup2(saved_stdout, fileno(stdout));
				close(saved_stdout);
				close(o_desc);
			}
			if (efile)
			{
				dup2(saved_stderr, fileno(stderr));
				close(saved_stderr);
				close(e_desc);
			}
			
			if (res == 1)
				set_prompt(); 	// If there is a need to change the prompt line
			else if (res == -1)
				continue; 		// If there is an error while executing an internal command
			
			else if (res == 2) 	// If there is no internal command i.e. "standard execution"
			{
				pid_t pid = fork();
				if (pid == 0) // Child: prepare redirections, execute command
				{
					int i_desc = -1, o_desc = -1, e_desc = -1;
					if (ifile)
					{
						i_desc = open(ifile, O_RDONLY);
						dup2(i_desc, fileno(stdin));
					}
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
					// Original files can be closed: they just have been duplicated
					if (ifile)
						close(i_desc);
					if (ofile)
						close(o_desc);
					if (efile)
						close(e_desc);

					// Empty command cases
					if(args_array[0] == NULL) exit(EXIT_SUCCESS);
					if(args_array[0][0] == '\0') exit(EXIT_SUCCESS);

					
					// The command is executed here
					execvp(args_array[0], args_array);

					// If the program gets here, execvp failed
					fprintf(stderr, "Error while launching %s\n", args_array[0]);
					exit(EXIT_FAILURE);
				}
				else if (pid > 0) // Parent: wait for child to finish
					wait(NULL);
			}
		}
		free(argument);
		free(args_array);
		free(line);
	}
	free(cmd_prompt);
	free(buffer);
	return 0;
}