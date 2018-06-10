#ifndef UTIL_H
#define UTIL_H

// function which finds a matching token in a string (homemade strtok)
char* findToken(const char* string,const char token, int* startIndex);

int isFile(const char* name); // useful to determine whether it is a file or a directory

int cp2(char* source, char* dest);
int cp4(char* source, char* dest);

#endif