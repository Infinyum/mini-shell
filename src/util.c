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
/////														  TOOLBOX OF FUNCTIONS												/////
/////																															/////
/////															   	MADE BY 													/////
/////												 ROBIN MALMASSON	| 	THOMAS VON ASCHEBERG								/////
/////																															/////
/////												  	SHELL PROJECT - POLYTECH 2017-2018										/////
/////																															/////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/**
 * Function that finds a token in a string
 * @param string, the string to look in...
 * @param token, the token we are looking for...
 * @param startIndex, where we start in the string (Will be update with the token position of the string if found)
 * @return the string from start index to the token (if found), NULL if no token or an Error
 */
char* findToken(const char* string,const char token, int* startIndex)
{
	if (!string)
	{
		fprintf(stderr, "ERROR ! INVALID EMPTY STRING ENTRY !\n");
		return NULL;
	}
	
	if (*startIndex > strlen(string))
		return NULL;

	int c;			// Our reading char
	char cc[2]; 	// It makes strcat easy... (Not very clean...)
    cc[1] = '\0'; 	// The 2nd char is end of String (Just a Security)


	// Our buffer is 256 chars long
	int buffersize = 256;
 	char* buffer = (char*)calloc(buffersize, sizeof(char));

 	int size = 0; // size is the number of current read char in the buffer

 	int i = *startIndex;
 	int str_sz = strlen(string);

 	
	while (i <= str_sz) // while there is a char
	{
        c = string[i]; // reading it

	    // Dynamic Allocation: if our reading is bigger than our buffer...
	    if(size > buffersize)
	    {
            // We update the size of our buffer
            buffersize = 2*size + 1; // limit the memory allocations to log(n);
            buffer = (char*)realloc(buffer,buffersize+1);
	    }
	    
        // if we find the token
 		if (c == token || c == '\0')
 		{
 			// the next StartIndex is the index just after the token
 			*startIndex = i+1;

 			char* res = calloc(size+1,sizeof(char));
 			strcpy(res,buffer);
 			free(buffer);
            return res;
 		}
		else // otherwise it is a "normal" char
		{
			cc[0] = c; 			// We store our char
			strcat(buffer, cc); // We add it to the buffer	
 			size++; 			// We update the exact size our reading buffer
 		}

 		i++;
    }

    free(buffer);
    return NULL; // if no token has been found
}


/**
 * Function that says if the object with the name is a file or a directory
 * @param name, the string representing the name of the object
 * @return 0 => is a directory, 1 => is a file, if negative value ==> error
 */
int isFile(const char* name)
{
    DIR* directory = opendir(name);

    if(directory != NULL)
    {
		closedir(directory);
		return 0;
    }

    if(errno == ENOTDIR)
    	return 1;
  

    return -1;
}


/**
 * Function that copy a file
 * @param source, the name of the source file
 * @param dest, the name of the destination
 * @return 0 if no problem, a negative int otherwise
 */
int cp2(char* source, char* dest)
{
	//If source and dest have the same name ==> ERROR !
	if(strcmp(source,dest)==0){
	
		fprintf(stderr,"ERROR, source == destination !");
		return -2;
		
	}
	
	int src = open(source,O_RDONLY);
	
	//struct that get the info of the file to copy
	struct stat info;
	
	//If we can't open the source file... ==> Error
	if(src==-1){
		fprintf(stderr,"ERROR in opening source file\n");
		return -1;	
	}

	//we get the info of the source file
	stat(source,&info);
	
	int destination=open(dest, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	
	//Creation of a max reading buffer (size of a page)
	int buffersize=2048;
	
	//If we can't open/create the destination file... ==> Error
	if(destination==-1){
		fprintf(stderr,"ERROR in opening destination file\n");
		return -1;	
	}

	//Update the destination rights
	chmod(dest,info.st_mode);
	
	//Init the reading buffer and it's real size
	char* buffer=malloc(buffersize*sizeof(char));
	int buffer_sz=read(src,buffer,buffersize);
	
	//While the size of the buffer is not empty
	while(buffer_sz>0){
		
		//Write buffer_sz Bytes of the source into the destination 
		int ecr=write(destination,buffer,buffer_sz);
		if(ecr<0){
			fprintf(stderr,"ERROR write problem");
			return -2;	
		}
		
		//Then read the following Bytes
		buffer_sz=read(src,buffer,buffersize);
		
		if(buffer_sz<0){
			fprintf(stderr,"ERROR read problem");
			return -2;	
		}
		
	}

	free(buffer);
	close(src);
	close(destination);
	
	return 0;
}


/**
 * Function that realize a copy of a file or of a full directory (recursive function)
 * @param source, a string representing the path of the file/directory to copy
 * @param dest, a string representing the path of the place to copy the file/directory
 * @return 0 if everything was OK, a negative int otherwise
 */
int cp4(char* source, char* dest)
{
	//If source and dest have the same name ==> ERROR !
	if(strcmp(source,dest)==0){
	
		fprintf(stderr,"ERROR, source == destination !");
		return -2;
		
	}
	
	//Initialisation of the directories
	DIR *dirIn;
	DIR *dirOut;
	struct dirent *entry;
	
	//We check if we can open the directories correctly
	if ((dirIn = opendir(source)) == NULL){
		printf("Source opendir() error\n");
		return -1;
	}
	else if ((dirOut = opendir(dest)) == NULL){
		printf("Destination opendir() error\n");
		return -1;
	}
	
	//While the entry is not empty...
	while((entry = readdir(dirIn)) != NULL){
		
		//if the source name is not just a point
		if(entry->d_name[0]!='.'){
			
			//We prepare the source name and the dest name
			char* strSource=calloc((strlen(source)+strlen(entry->d_name)+2),sizeof(char));
			strcat(strSource,source);
			
			//We add backslash where it is needed...
			if(strSource[strlen(strSource)-1]!='/'){
					
				strcat(strSource,"/");
				
			}
			
			//we concat the names...
			strcat(strSource,entry->d_name);
			
			char* strDest=calloc((strlen(source)+strlen(entry->d_name)+2),sizeof(char));
			strcat(strDest,dest);
			strcat(strDest,"/");
			strcat(strDest,entry->d_name);
			
			//If it is a file...
			if(isFile(strSource)){
				
				//We just copy it with the function we created early
				cp2(strSource,strDest);
					
				
			}
			//Otherwise if it is a directory...
			else if (isFile(strSource)==0){
				
				struct stat info;
				stat(strSource,&info);
				
				//We create the destination directory and we update it's protection level
				mkdir(strDest,0777);
				chmod(strDest,info.st_mode);
				
				//Then we copy the content of the source directory into the destination directory
				cp4(strSource,strDest);
				
			}
			//Otherwise it is an error...
			else{
				fprintf(stderr,"ERROR !");
				return -3;
			}
			
			free(strSource);
			free(strDest);
		}
	}
	
	//We close the directories when we have finished...
	closedir(dirIn);
	closedir(dirOut);
	
	return 0;
}