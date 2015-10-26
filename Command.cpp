#include <iostream>
#include <vector>
#include <unistd.h>
#include <glob.h>
#include <string.h>
#include <signal.h>

#include "Command.h"

Command::Command(std::string cmdLine, std::string token, bool hasPrevPipe)
{
	this->cmdLine = cmdLine;
	this->command = token;
	this->prevPipe = hasPrevPipe;
	this->nextPipe = false;
}

bool Command::hasInvalidChar()
{
	char cmdInvalidChar[11] = {32, 9, 62, 60, 124, 42, 33, 96, 39, 34};
	char argInvalidChar[10] = {32, 9, 62, 60, 124, 33, 96, 39, 34};

	if(strcspn(this->command.c_str(), cmdInvalidChar) < strlen(this->command.c_str()))
		return true;
	for(int i=0; i<this->argVector.size(); i++)
	{
		if(strcspn(this->argVector[i].c_str(), argInvalidChar) < strlen(this->argVector[i].c_str()))
			return true;
	}
	return false;
}

void Command::forkProcess()
{
	std::vector<char *> argv(this->argVector.size()+2);//exec argument vector
	for(int i=0; i<this->argVector.size(); ++i)
		argv[i+1] = &this->argVector[i][0];
	argv[0] = &this->command[0];
	argv[this->argVector.size()+1] = NULL;

	if(this->command.compare(0,2,"./")!=0 || this->command.compare(0,3,"../")!=0 || this->command.compare(0,1,"/")!=0)
		setenv("PATH", "/bin:/usr/bin:.", 1);//set to relative path
	signal(SIGINT,  SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);

	if(execvp(this->command.c_str(), argv.data()) == -1)
		std::cout << "["<<this->command<<"]: command not found"<<std::endl;
	else
		std::cout << "["<<this->command<<"]: unknown error"	<<std::endl;
	//safety measure exit, prevent zombie
	exit(0);
}

void Command::fdAssignment(int *prevFD, int *nextFD, int noOfPipe)
{
	if(noOfPipe == 0)//no pipe
	{
		this->forkProcess();
	}
	else if(noOfPipe == 1)
	{
		if(prevFD==NULL && nextFD)
		{
			dup2(nextFD[1],1);
			close(nextFD[0]);
			close(nextFD[1]);

			this->forkProcess();
		}
		else if(prevFD && nextFD==NULL)
		{
			dup2(prevFD[0],0);
			close(prevFD[0]);
			close(prevFD[1]);

			this->forkProcess();
		}
	}
	else if(noOfPipe == 2)
	{
		if(prevFD==NULL && nextFD)
		{
			dup2(nextFD[1],1);
			close(nextFD[0]); close(nextFD[1]);

			this->forkProcess();
		}
		else if(prevFD && nextFD)
		{
			dup2(prevFD[0],0);
			dup2(nextFD[1],1);
			close(prevFD[0]); close(prevFD[1]);
			close(nextFD[0]); close(nextFD[1]);

			this->forkProcess();
		}
		else if(prevFD && nextFD==NULL)
		{
			dup2(prevFD[0], 0);
			close(prevFD[0]); close(prevFD[1]);

			this->forkProcess();
		}
	}
}

void Command::wildcardExpand(int argIndex)
{
    glob_t glob_buffer;
	int match_count;

	glob( this->argVector[argIndex].c_str() , 0 , NULL , &glob_buffer ); 
	match_count = glob_buffer.gl_pathc;

	if(match_count == 0)
	{
		globfree( &glob_buffer );
		return;//do not modify the array
	}
	else
	{
		for (int i=match_count-1; i>=0; i--)
			this->argVector.insert(this->argVector.begin() + argIndex + 1, glob_buffer.gl_pathv[i]);
		this->argVector.erase(this->argVector.begin() + argIndex);
	}

	globfree( &glob_buffer );
}