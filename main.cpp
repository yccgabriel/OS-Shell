#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "FSM.h"
#include "Command.h"
#include "global.h"
#include "Jobs.h"

extern std::vector<Jobs*> jobsVector;

void builtinProcedure(FSM* fsm)
{
	if(fsm->builtinGrammarCheck() == false)
		return;

	if(fsm->builtinCommand == "exit")
	{
		//check jobsVector length. Dont exit
		if(jobsVector.size() > 0)
		{
			std::cout << "There is at least one suspended job" << std::endl;
			return;
		}
		else 
		{
			std::cout << "[ Shell Terminated ]" << std::endl;
			exit(0);
		}
	}
	else if(fsm->builtinCommand == "cd")
	{
		if(chdir(fsm->builtinArgVector[0].c_str()) == -1)
			std::cout << "["<<fsm->builtinArgVector[0]<<"] cannot change directory" << std::endl;
	}
	else if(fsm->builtinCommand == "jobs")
	{
		if(jobsVector.size() == 0)
		{
			std::cout << "No suspended jobs" << std::endl;
			return;
		}
		for(int i=0; i<jobsVector.size(); i++)
		{
			std::cout<<"["<<i+1<<"] "<<jobsVector[i]->cmdLine<<std::endl;
		}
	}
	else if(fsm->builtinCommand == "fg")
	{
		int fgJobNum = atoi(fsm->builtinArgVector[0].c_str());
		//check parameter is on list
		if(1 > fgJobNum || fgJobNum > jobsVector.size())
		{
			std::cout << "fg: no such job" << std::endl;
			return;
		}
		for(int i=0; i<jobsVector[fgJobNum-1]->pidVector.size(); i++)
			if(kill(jobsVector[fgJobNum-1]->pidVector[i], SIGCONT) == -1){
				std::cout << "jobs continue error" << std::endl;
			}

		//sleep all the process
		int status[3];//should change to vector. I don't know how
		int waitpidVector[3];//bad variable name
		for(int i=0; i<jobsVector[fgJobNum-1]->pidVector.size(); i++)
		{
			if((waitpidVector[i]=waitpid(jobsVector[fgJobNum-1]->pidVector[i], &status[i], WUNTRACED)) != jobsVector[fgJobNum-1]->pidVector[i])
			{
				//error handling
			}
			else if(WIFSTOPPED(status[i]))
			{
				//child wake up again. return to prepare new command
				return;
			}
		/*	else if(WIFSIGNALED(status[i]))//True if the process terminated due to receipt of a signal.
			{//remove the job from list
				std::cout << "deleting job list" << std::endl;
				if(std::find(jobsVector[fgJobNum-1]->pidVector.begin(),
														  jobsVector[fgJobNum-1]->pidVector.end(), 
														  waitpidVector[i])  						!=	jobsVector[fgJobNum-1]->pidVector.end())
				{
					delete jobsVector[fgJobNum-1];
					jobsVector.erase(jobsVector.begin() + fgJobNum-1);
				}
			}*/
			else
			{
				if(std::find(jobsVector[fgJobNum-1]->pidVector.begin(),
														  jobsVector[fgJobNum-1]->pidVector.end(), 
														  waitpidVector[i])  						!=	jobsVector[fgJobNum-1]->pidVector.end())
				{
					delete jobsVector[fgJobNum-1];
					jobsVector.erase(jobsVector.begin() + fgJobNum-1);
				}
				return;
			}
		}
	}
}

void commandProcedure(FSM* fsm)
{
	int pid0, pid1, pid2;
	int fd1[2], fd2[2];

	if(fsm->commandGrammarCheck() == false){
		return;
	}

	//wildcard expension
	for(int i=0; i<fsm->commandVector.size(); i++)
		for(int j=fsm->commandVector[i]->argVector.size()-1; j>=0; j--)
			if(strcspn(fsm->commandVector[i]->argVector[j].c_str(), "*") < strlen(fsm->commandVector[i]->argVector[j].c_str()))
				fsm->commandVector[i]->wildcardExpand(j);

	//testing
	/*std::cout << fsm->commandVector.size() << std::endl;
	for(int i=0; i<fsm->commandVector.size(); i++){
		std::cout << "cmd: " << fsm->commandVector[i]->command <<" "<<fsm->commandVector[i]->nextPipe << std::endl;
		for(int j=0; j<fsm->commandVector[i]->argVector.size(); j++){
			std::cout << "arg: " << fsm->commandVector[i]->argVector[j] << std::endl;
		}
	}*/
	//end testing

	//pipe pipe pipe
	if(fsm->commandVector.size()==1)//no pipe
	{
		if((pid0=fork()) == -1)
			{std::cout << "fork error" <<std::endl;	return;}
		if(pid0 == 0)//child, new process
			fsm->commandVector[0]->fdAssignment(NULL,NULL,0);

		//parent, wait child

		int status[3];//should change to vector. I don't know how
		if(waitpid(pid0, &status[0], WUNTRACED) != pid0){
			//kill all children and return
			//return;
		}
		else if(WIFEXITED(status[0]))
		{
			//child process exited normally
			return;
		}
		else if(WIFSTOPPED(status[0])){
			//child stop, parent resume
			std::vector<int> pidVector;
			pidVector.push_back(pid0);
			jobsVector.push_back(new Jobs(pidVector, fsm->commandVector[0]->cmdLine));
		}
		else{
			std::cout << "unknown jobs control error" << std::endl;
		}
	}
	else if(fsm->commandVector.size()==2)//1 pipe
	{
		pipe(fd1);

		if((pid0=fork()) == -1)
			{std::cout << "fork error" <<std::endl;	return;}
		if(pid0 == 0)
			fsm->commandVector[0]->fdAssignment(NULL, fd1,1);
		
		if((pid1=fork()) == -1)
			{std::cout << "fork error" <<std::endl;	return;}
		if(pid1 == 0)
			fsm->commandVector[1]->fdAssignment(fd1, NULL,1);

		//parent
		close(fd1[0]);
		close(fd1[1]);

		int status[3];//should change to vector. I don't know how
		if(waitpid(pid0, &status[0], WUNTRACED)!=pid0 || waitpid(pid1, &status[1], WUNTRACED)!=pid1){
			//kill all children and return
			//return;
		}
		else if(WIFEXITED(status[0]) || WIFEXITED(status[1]))
		{
			//child process exited normally
			return;
		}
		else if(WIFSTOPPED(status[0]) || WIFSTOPPED(status[1])){
			//child stop, parent resume
			//jobs control
			std::vector<int> pidVector;
			pidVector.push_back(pid0);
			pidVector.push_back(pid1);
			jobsVector.push_back(new Jobs(pidVector, fsm->commandVector[0]->cmdLine));
		}
		else{
			std::cout << "unknown jobs control error" << std::endl;
		}
	}
	else if(fsm->commandVector.size()==3)//2 pipes
	{
		pipe(fd1);
		pipe(fd2);

		if((pid0=fork()) == -1)
			{std::cout << "fork error" <<std::endl;	return;}
		if(pid0 == 0){
			close(fd2[0]);close(fd2[1]);
			fsm->commandVector[0]->fdAssignment(NULL, fd1,2);
		}

		if((pid1=fork()) == -1)
			{std::cout << "fork error" <<std::endl;	return;}
		if(pid1 == 0)
			fsm->commandVector[1]->fdAssignment(fd1, fd2,2);

		if((pid2=fork()) == -1)
			{std::cout << "fork error" <<std::endl;	return;}
		if(pid2 == 0){
			close(fd1[0]);close(fd1[1]);
			fsm->commandVector[2]->fdAssignment(fd2, NULL,2);
		}

		close(fd1[0]);
		close(fd1[1]);
		close(fd2[0]);
		close(fd2[1]);

		int status[3];//should change to vector. I don't know how
		if(waitpid(pid0, &status[0], WUNTRACED)!=pid0 || waitpid(pid1, &status[1], WUNTRACED)!=pid1 || waitpid(pid2, &status[2], WUNTRACED)!=pid2){
			//kill all children and return
			//return;
		}
		else if(WIFEXITED(status[0]) || WIFEXITED(status[1]) || WIFEXITED(status[2]))
		{
			//child process exited normally
			return;
		}
		else if(WIFSTOPPED(status[0]) || WIFSTOPPED(status[1]) || WIFSTOPPED(status[2])){
			//child stop, parent resume
			//jobs control
			std::vector<int> pidVector;
			pidVector.push_back(pid0);
			pidVector.push_back(pid1);
			pidVector.push_back(pid2);
			jobsVector.push_back(new Jobs(pidVector, fsm->commandVector[0]->cmdLine));
		}
		else{
			std::cout << "unknown jobs control error" << std::endl;
		}
	}
}

bool newLine()
{
	std::string cmdLine;
	std::vector<std::string> tokens;
	std::string buf;
	char cwd[1024];

	getcwd(cwd, sizeof(cwd));
	std::cout << "[3150 shell:"<<cwd<<"]$ ";
	std::getline(std::cin, cmdLine);

	std::stringstream ss(cmdLine);
	while(ss >> buf)
		tokens.push_back(buf);

	FSM* fsm = new FSM();
	//push the tokens to FSM one by one and assign state
	for(int i=0; i<tokens.size(); i++)
		fsm->pushToken(cmdLine, tokens[i]);

	//implementation, grammar check inside Procedure sub-routine
	bool error = false;
	if(fsm->builtinCommand.compare("")!=0)
		builtinProcedure(fsm);
	else
		commandProcedure(fsm);
		

	if (std::cin.eof())
		return false;
	else
		return true;//true for continue the loop
}

int main(void)
{
	signal(SIGINT,  SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	bool cont = true;
	while(cont)
	{
		cont = newLine();
	}

	std::cout << "[ Shell Terminated ]" << std::endl;

	return 0;
}