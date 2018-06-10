#ifndef PIPES_H
#define PIPES_H

int pipe_commands_array(char **args_array, int args_sz, int pipe_pos, char* ifile, char* ofile, char* efile, int ntok_read, char* path);
int loop_pipe(char ***cmd, int first_cmd_sz, char* ifile, char* ofile, char* efile, int ntok_read, char* path);

int next_pipe(char **args_array);
void pipe_free(char ***cmd);

#endif