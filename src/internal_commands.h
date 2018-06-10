#ifndef INTERNAL_COMMANDS_H
#define INTERNAL_COMMANDS_H

int internal_command(char **args_array, int args_sz, int ntok_read, char* path, char* ifile);
int internal_command_search(char *command);

int cd(char* NewDir);
int senv(char* VarName,char* NewValue);
int copy(char* Source, char* Destination);
void cat(char** args_array);
void ls(char** args_array,char* path, unsigned int StartIndex);
int find(char** args_array,char* path, unsigned int StartIndex ,unsigned int callNum);

#endif