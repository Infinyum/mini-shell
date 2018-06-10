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
/////														INTERNAL SHELL COMMANDS												/////
/////																															/////
/////															   	MADE BY 													/////
/////												 	ROBIN MALMASSON AND THOMAS VON ASCHEBERG								/////
/////																															/////
/////												  	  SHELL PROJECT - POLYTECH 2017-2018									/////
/////																															/////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// An array containing the name of the internal commands
const char* internal_commands[] = {"cd", "senv", "copy", "cat", "ls", "find"};
// The number of internal commands
const int internal_comm_sz = 6;


/**
 * Run the internal commands
 * @return -1 if there is an error
 * @return 0 if everything executed properly
 * @return 1 if everything executed properly and the prompt line has to be changed
 * @return 2 if there is no internal command
 */
int internal_command(char **args_array, int args_sz, int ntok_read, char* path, char* ifile)
{
	// Looking for an internal command
	int i = internal_command_search(args_array[0]);
	if (i == -1) return 2;

	// position 0 corresponds to the cd command which changes the current directory
	if (i == 0)
	{
		if (cd(args_array[1]) == -1)
		{
			fprintf(stderr, "Error while changing the directory\n");
			return -1;
		}
		return 1;
	}


	// position 1 corresponds to the senv command which sets the value of the specified environment variable
	// syntax: senv VARNAME NEWVALUE
	// ex: senv USER Bob
	else if (i == 1)
	{
		// if there is a missing parameter
		if (!args_array[1] || !args_array[2])
		{
			fprintf(stderr, "ERROR: empty parameter for senv command\n");
			return -1;
		}

		//set the new environnment value
		senv(args_array[1],args_array[2]);
		return 1;
	}

	
	//Position 2 is for the homemade copy command
	//prototype : copy SOURCE DESTINATION
	//ex: copy /home/toto /home/user/Desktop/testCopy
	else if (i == 2){

		//if we have empty parameters....
		if(args_array[1]==NULL || args_array[2]==NULL){
			fprintf(stderr, "ERROR ! INVALID VALUES : EMPTY VALUES ARE NOT AUTORIZED FOR COPY COMMAND !\n");
			return -1;
		}

		//Just use the function we made
		copy(args_array[1],args_array[2]);
	}

	//Position 3 is for the homemade cat command
	//prototype : cat file1 file2 ...
	//ex : cat toto.txt test.txt test2.txt
	else if (i == 3){
		//if we have empty parameter(s)....
		if(args_array[1]==NULL && !ifile) {
			fprintf(stderr, "ERROR ! INVALID VALUES : EMPTY VALUES ARE NOT AUTORIZED FOR CAT COMMAND !\n");
			return -1;
		}
		else
		{
			// If stdin changed, adding the actual entry to use as an argument for cat
			if (ifile)
			{
				args_sz++;
				args_array = realloc(args_array, args_sz*sizeof(char*));
				args_array[ntok_read++] = ifile;
				args_array[ntok_read] = NULL;
			}
			cat(args_array);
		} 
	}

	//Position 4 is for the homemade ls command
	//prototype : ls (path) -[arg]
	//arg is l or a or la
	//if no path ==> in current directory
	//ex : ls -la
	//ex2: ls /home -a
	else if(i == 4){
		
		//if we have an argument...
		if(args_array[1]!=NULL){
			//if it is not a specifier ==> then it is the path argument
			if(args_array[1][0]!='-')
				ls(args_array,args_array[1],2);
			//otherwise (no path)
			else
				//ls in the current dir (no path argument)
				ls(args_array,path,1);					
		}
		//no argument...
		else ls(args_array,path,1);	
	}

	//Position 5 is for the homemade find command
	//prototype : find path -[arg] [argValue]
	//path can be empty ==> current directory
	//arg = name or exec / argValue is the value for the arg
	//ex : find -name *.c -exec cat {}
	else if(i == 5){
		//find with no parameter or no path argument
		if(args_array[1]==NULL || args_array[1][0]=='-')
			find(args_array,path,1,0);
		
		//find with a path argument
		else find(args_array,args_array[1],2,0);
	}

	return 0;
}



/**
 * Look for the specified command in the internal_commands array
 * @param command: the command to look for
 * @return -1 if the command isn't in the array, its position in the array otherwise
 */
int internal_command_search(char *command)
{
	if (!command) return -1;

	int i = 0;
	while (internal_commands[i] != NULL)
	{
		if (!strcmp(command, internal_commands[i])) return i;
		i++; 
	}

	return -1;
}


/**
 * INTERNAL COMMAND 0
 * Function that changes the current directory
 * @param nothing...
 * @return a positive integer if no problem, -1 otherwise
 */
int cd(char* NewDir){

	if (chdir(NewDir) == -1) {
		//fprintf(stderr, "Error while changing the directory\n");
		return -1;
	}

	return 0; 
}


/**
 * INTERNAL COMMAND 1
 * Function that changes the value of the environment variable
 * @param VarName, the name of the environment's variable to change
 * @param NewValue, the new value to attribute to the variable
 * @return 0 if no problem, -1 if a problem occured
 */
int senv(char* VarName,char* NewValue){

	int res = setenv(VarName,NewValue,1);

	//if an error occured we have to tell the user
	if(res!=0){

		fprintf(stderr, "ERROR ! A PROBLEM OCCURED WHILE CHANGING THE VALUE !\n");
	}

	return res;


}


/**
 * INTERNAL COMMAND 2
 * Function that copy a file/directory to a new place
 * @param Source, the file/dir to copy
 * @param Destination, the place to copy the Source
 * @return 0 if no problem, a negative int otherwise
 */
int copy(char* Source,char* Destination){

	int res = 0;

	int isfile = isFile(Source);

	if(isfile==1){
		//Just use the function we made to copy a file(TP1)
		res=cp2(Source,Destination);
	}
	else if(isfile==0){
		//Just use the function we made to copy a directory(TP1)
		res=cp4(Source,Destination);
	}
	else{
		//error case...
		fprintf(stderr, "ERROR ! CAN'T COPY THE OBJECT %s !\n", Source);
		res=isfile;
	}
	

	return res;

}


/**
 * INTERNAL COMMAND 3
 * Function that concat a file to display
 * @param args_array, the array of argument (the list of the files)
 * @return nothing...
 */
void cat(char** args_array){

	int j=1;

	//while we have Paramaters/Files...
	while(args_array[j]!=NULL){

		int src = open(args_array[j],O_RDONLY);

		//If we can't open the source file... ==> Error
		if(src==-1){
			fprintf(stderr,"ERROR in opening source file : %s \n",args_array[j]);
			return;	
		}

		//Creation of a max reading buffer (size of a page)
		int buffersize=2048; //2048 B
	
		//Init the reading buffer and it's real size
		char* buffer=calloc(buffersize,sizeof(char));

		if(buffer==NULL){
			fprintf(stderr,"ERROR ! NOT ENOUGHT MEMORY !");
			return;
		}

		int buffer_sz=read(src,buffer,buffersize-1);
		
		
		//While the size of the buffer is not empty
		while(buffer_sz>0){

			//printf("%d\n",buffer_sz);
			
			printf("%s",buffer);
			
			//Then read the following Bytes
			buffer_sz=read(src,buffer,buffersize-1);
			
			buffer[buffer_sz]='\0';
			
			if(buffer_sz<0){
				fprintf(stderr,"ERROR read problem");
				return;	
			}

			
		}	
		
		
		
		//loop
		j=j+1;
		free(buffer);
		close(src);
	}
	
	
}


/**
 * INTERNAL COMMAND 4
 * Function that list the files and directory of a directory and display it
 * @param args_array, the array of argument (the list of the files)
 * @param path, the name of the directory to list and display
 * @param StartIndex, the index we start in the args_array
 * @return nothing...
 */
void ls(char** args_array,char* path, unsigned int StartIndex){

	//No parameter
	if(args_array[StartIndex]==NULL){


		DIR *dirIn;
		struct dirent *entry;		
				
		//We check if we can open the directories correctly
		if ((dirIn = opendir(path)) == NULL){
			printf("Path : %s opendir() error\n",path);
			return;
		}
		
		//While the entry is not empty...
		while((entry = readdir(dirIn)) != NULL){

			if(entry->d_name[0]!='.'){
				//printf("%s\t",entry->d_name);
				printf("%s  ", entry->d_name);
			}
			
		}

		closedir(dirIn);
	}
	else if(!strcmp(args_array[StartIndex],"-l")){

		DIR *dirIn = opendir(path);
		struct dirent *entry=NULL;

		//While the entry is not empty...
		while((entry = readdir(dirIn)) != NULL){

			if(entry->d_name[0]!='.'){
			char* filename = calloc(strlen(path)+strlen(entry->d_name)+2,sizeof(char));
			strcat(filename,path);
			strcat(filename,"/");
			strcat(filename,entry->d_name);

			//struct that get the info of the file to print on screen
			struct stat info;
			
			//we get the info of the source file
			if(stat(filename,&info)==-1){
				perror("stat");
				exit(EXIT_SUCCESS);
			}

			char* mode = calloc(64,sizeof(char));
			int nbcharmode=0;


			//The st_mode in stat is number: we have to check things with a mask for each thing
			if(S_ISDIR(info.st_mode)){
				 strcat(mode,"d");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IRUSR){
				strcat(mode,"r");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IWUSR){
				strcat(mode,"w");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IXUSR){
				strcat(mode,"x");
				 nbcharmode++;
				 
			}

			strcat(mode,"-");
			nbcharmode++;

			if(info.st_mode & S_IRGRP){
				strcat(mode,"r");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IRGRP){
				strcat(mode,"w");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IRGRP){
				strcat(mode,"x");
				 nbcharmode++;
				 
			}

			strcat(mode,"-");
			nbcharmode++;

			if(info.st_mode & S_IROTH){
				strcat(mode,"r");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IROTH){
				strcat(mode,"w");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IROTH){
				strcat(mode,"x");
				 nbcharmode++;
				 
			}

			strcat(mode,"-");
			nbcharmode++;
			mode[nbcharmode]='\0';

			//we convert a time_t to a string
			struct tm date;
			date = *localtime(&info.st_atime);					
			
		    char* dt=asctime(&date);

		    //print all the properties
			printf("Name : %s\tPermission: %s\t%zu root\t%ld link(s)\tlenght : %ld Bytes\t\tLast access :%s\n",entry->d_name,mode,entry->d_ino,info.st_nlink,info.st_size,dt);

			free(mode);
			free(filename);
			}
		}

		closedir(dirIn);
		
	}
	else if(!strcmp(args_array[StartIndex],"-a")){

		DIR *dirIn = opendir(path);
		struct dirent *entry;		

		//While the entry is not empty...
		while((entry = readdir(dirIn)) != NULL){

			//printf("%s\t",entry->d_name);
			printf("%s  ", entry->d_name);
		}

		closedir(dirIn);
	}
	else if(!strcmp(args_array[StartIndex],"-la")){
		
		DIR *dirIn = opendir(path);
		struct dirent *entry;

		//While the entry is not empty...
		while((entry = readdir(dirIn)) != NULL){

			char* filename = calloc(strlen(path)+strlen(entry->d_name)+2,sizeof(char));
			strcat(filename,path);
			strcat(filename,"/");
			strcat(filename,entry->d_name);

			//struct that get the info of the file to print on screen
			struct stat info;
			
			//we get the info of the source file
			if(stat(filename,&info)==-1){
				perror("stat");
				exit(EXIT_SUCCESS);
			}

			char* mode = calloc(64,sizeof(char));
			int nbcharmode=0;


			//The st_mode in stat is number: we have to check things with a mask for each thing
			if(S_ISDIR(info.st_mode)){
				 strcat(mode,"d");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IRUSR){
				strcat(mode,"r");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IWUSR){
				strcat(mode,"w");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IXUSR){
				strcat(mode,"x");
				 nbcharmode++;
				 
			}

			strcat(mode,"-");
			nbcharmode++;

			if(info.st_mode & S_IRGRP){
				strcat(mode,"r");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IRGRP){
				strcat(mode,"w");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IRGRP){
				strcat(mode,"x");
				 nbcharmode++;
				 
			}

			strcat(mode,"-");
			nbcharmode++;

			if(info.st_mode & S_IROTH){
				strcat(mode,"r");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IROTH){
				strcat(mode,"w");
				 nbcharmode++;
				 
			}
			if(info.st_mode & S_IROTH){
				strcat(mode,"x");
				 nbcharmode++;
				 
			}

			strcat(mode,"-");
			nbcharmode++;
			mode[nbcharmode]='\0';

			//we convert a time_t to a string
			struct tm date;
			date = *localtime(&info.st_atime);					
			
		    char* dt=asctime(&date);

		    //print all the properties
			printf("Name : %s\tPermission: %s\t%zu root\t%ld link(s)\tlenght : %ld Bytes\t\tLast access :%s\n",entry->d_name,mode,entry->d_ino,info.st_nlink,info.st_size,dt);

			free(mode);
			free(filename);
		}
		
		closedir(dirIn);
	}
		
	printf("\n");
}


/**
 * INTERNAL COMMAND 5
 * Function that find files and execute a command on those file (if -exec) otherwise print them on screen
 * @param args_array, the array of arguments 
 * @param path, the name of the directory 
 * @param StartIndex, the index we start to use in our args_array
 * @param callNum, the num of call (witch level of recusive call)
 * @return 0 if no problem, a negative int otherwise...
 */
int find(char** args_array,char* path,unsigned int StartIndex, unsigned int callNum){

	unsigned int j = StartIndex;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																													 //
	//											NO ARGUMENT ZONE => Find and Print files								 //
	//																													 //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//find with no parameter or just a path parameter
	if(args_array[j]==NULL || 
		(!(args_array[j][0]=='-') && (args_array[2]==NULL)) ){

		//Just print the files and dir recursively

		//Initialisation of the directories
		DIR *dirIn;
		struct dirent *entry;
		
		//We check if we can open the directories correctly
		if ((dirIn = opendir(path)) == NULL){
			printf("Path : %s opendir() error\n",path);
			return -1;
		}
				
		//While the entry is not empty...
		while((entry = readdir(dirIn)) != NULL){
			
			//if the source name is not just a point
			if(entry->d_name[0]!='.'){
				
				//We prepare the source name and the dest name
				char* strSource=calloc((strlen(path)+strlen(entry->d_name)+2),sizeof(char));
				strcat(strSource,path);
				
				//We add backslash where it is needed...
				if(strSource[strlen(strSource)-1]!='/'){
						
					strcat(strSource,"/");
					
				}
				
				//we concat the names...
				strcat(strSource,entry->d_name);
				
				//If it is a file...
				if(isFile(strSource)){
					
					//We just print it 
					printf("%s\n",strSource);
						
					
				}
				//Otherwise if it is a directory...
				else if (isFile(strSource)==0){
					
					//We just print it 
					printf("%s\n",strSource);

					//Then we print the content of the source directory
					find(args_array,strSource,StartIndex,callNum++);

					
				}
				//Otherwise it is an error...
				else{
					free(strSource);
					fprintf(stderr,"ERROR !");
					return -3;
				}
				

				free(strSource);

			}
		}
		
		//We close the directories when we have finished...
		closedir(dirIn);
		j++;
		return 0;

	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																													 //
	//												END OF NO ARGUMENT ZONE 											 //
	//																													 //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																													 //
	//													ARGUMENT ZONE 				 									 //
	//																													 //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//find with argument 
	else{

		char* pattern = NULL;

		//We will stock the list of matching file
		unsigned int nbFile=0;
		unsigned int sizeFileArray=64;
		char** fileArray = calloc(sizeFileArray,sizeof(char*));

		if(args_array[j][0]=='-'){

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//																													 //
			//											-name OPTION ZONE					 									 //
			//																													 //
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//find a -name option
			if(!strcmp(args_array[j],"-name")){

				//no value for name option ==> ERROR !
				if(args_array[j+1]==NULL){

					fprintf(stderr, "ERROR ! MISSING ARGUMENT VALUE FOR -name OPTION !\n");
					return -1;

				}

				//looking for a type of file
				if(args_array[j+1][0]=='*'){
					pattern = args_array[j+1] + 1;
				}
				//looking for a particular file
				else{
					pattern = args_array[j+1];
				}

				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				//																													 //
				//											LOOP to find all the matching Files 									 //
				//																													 //
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				
				//Initialisation of the directories
				DIR *dirIn;
				struct dirent *entry;
				
				//We check if we can open the directories correctly
				if ((dirIn = opendir(path)) == NULL){
					printf("Path : %s opendir() error\n", path);
					return -1;
				}
						
				//While the entry is not empty...
				while((entry = readdir(dirIn)) != NULL){
					
					//if the source name is not just a point
					if(entry->d_name[0]!='.'){
						
						//We prepare the name
						char* strSource=calloc((strlen(path)+strlen(entry->d_name)+2),sizeof(char));
						strcat(strSource,path);
						
						//We add backslash where it is needed...
						if(strSource[strlen(strSource)-1]!='/'){
								
							strcat(strSource,"/");
							
						}
						
						//we concat the names...
						strcat(strSource,entry->d_name);
						
						//If it is a file...
						if(isFile(strSource)){
							
							char* res = strstr(strSource,pattern);

							//if the pattern matches...
							if(res!=NULL){

								//realloc the file Array if too small
								if(nbFile>sizeFileArray){

									sizeFileArray = 2*sizeFileArray + 1; //limit the memory allocations to log(n)
									fileArray = realloc(fileArray, sizeFileArray*sizeof(char*));
								}

								//We just add it to the fileArray
								fileArray[nbFile]=calloc(strlen(strSource)+1,sizeof(char));
								strcpy(fileArray[nbFile],strSource);						
								nbFile++;

								}							
							
						}
						//Otherwise if it is a directory...
						else if (isFile(strSource)==0){
							
							//Then we continue in look for in the content of the source directory
							find(args_array,strSource,j,callNum++);

							
						}
						//Otherwise it is an error...
						else{
							free(strSource);
							fprintf(stderr,"ERROR !");
							return -3;
						}
						

						free(strSource);
						

					}
				}


				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				//																													 //
				//											END OF LOOP to find all the matching Files 								 //
				//																													 //
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				j=j+2;
			}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//																													 //
			//											END OF -name OPTION ZONE			 									 //
			//																													 //
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			//No more argument (=no -exec) => Print all matching files
			if(args_array[j]==NULL){
				unsigned int i = 0;
				
				while(fileArray[i]!=NULL){
					printf("%s\n",fileArray[i]);
					i++;
				}
			}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//																													 //
			//											-exec OPTION ZONE					 									 //
			//																													 //
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			
			//-exec argument
			else if( !strcmp(args_array[j],"-exec") ){

				j++;

				
				unsigned int internalArgs_array_sz=16;
				char** internalArgs_array=calloc(internalArgs_array_sz,sizeof(char*));

				//store the command index for the exec
				unsigned int commandIndex = 0;
				unsigned int i = j;
				
				//loop to copy the rest of the args_array into another array
				while(args_array[i]!=NULL){

					//realloc the internalArgs_array if too small
					if(commandIndex>internalArgs_array_sz){

						internalArgs_array_sz = 2*internalArgs_array_sz + 1; //limit the memory allocations to log(n)
						internalArgs_array = realloc(internalArgs_array, internalArgs_array_sz*sizeof(char*));
					}

					internalArgs_array[commandIndex]=calloc(strlen(args_array[i])+1,sizeof(char));

					strcpy(internalArgs_array[commandIndex],args_array[i]);

					i++;
					commandIndex++;

				}

				//reUsing i for another loop
				i=0;

				//create an array of int representing the positions of {} in the command
				//DIRTY ! STATIC ALLOCATION
				unsigned int position[commandIndex];
				unsigned int nbBracket=0;

				//looking for {} to know where to replace filename
				while(internalArgs_array[i]!=NULL){

					char* res=strstr(internalArgs_array[i],"{}");

					if(res!=NULL){
						position[nbBracket]=i;
						nbBracket++;
					}

					i++;
				}

				//no bracket ==> command doesn't use the found file(s)
				if(nbBracket==0){

					pid_t pid = fork();
					if (pid == 0)
					{
						
						// The command is executed here
						execvp(internalArgs_array[0], internalArgs_array);

						// If the program arrives here, execvp failed
						fprintf(stderr, "Error while launching %s in find command\n", internalArgs_array[0]);
						exit(EXIT_FAILURE);
					}
						
				}
				else{

					//exec loop for all files
					unsigned int index =0;

					while(fileArray[index]!=NULL){

						//replace the {} by the file name
						unsigned int bracketIndex=0;
						while(bracketIndex<nbBracket){

							//realloc the {} string memory by name of file
							internalArgs_array[position[bracketIndex]]=realloc(internalArgs_array[position[bracketIndex]],
								(strlen(fileArray[index])+1)*sizeof(char));
							
							//copy the filename
							strcpy(internalArgs_array[position[bracketIndex]],fileArray[index]);

							bracketIndex++;
						}

						pid_t pid = fork();
						if (pid == 0)
						{
							// The command is executed here
							execvp(internalArgs_array[0], internalArgs_array);

							// If the program arrives here, execvp failed
							fprintf(stderr, "Error while launching %s in find command\n", internalArgs_array[0]);
							exit(EXIT_FAILURE);
						}
						else{
							// Parent: wait for child to finish
							int status; 				//optional; give infos on child termination
							waitpid(pid, &status, 0); 	//or just wait(&status), or wait(NULL)...

						}

						index++;
					}

				}


				//free all the arguments stored locally
				unsigned int k = 0;
				while(internalArgs_array[k]!=NULL){

					free(internalArgs_array[k]);
					k++;
				}
				free(internalArgs_array);
			}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//																													 //
			//											END OF -exec OPTION ZONE			 									 //
			//																													 //
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			
		}


		//free all the filename string stored
		unsigned int i = 0;
		while(fileArray[i]!=NULL){
			free(fileArray[i]);
			i++;
		}
		free(fileArray);

	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																													 //
	//													END OF ARGUMENT ZONE 		 									 //
	//																													 //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	return 0;

}