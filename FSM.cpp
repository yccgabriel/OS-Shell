#include <iostream>
#include <string>
#include <vector>
#include <string.h>

#include "Command.h"
#include "FSM.h"

FSM::FSM()
{
	state = READY;
}

bool FSM::isBuiltin(std::string token)
{
	if(token.compare("cd") == 0)
		return true;
	if(token.compare("exit") == 0)
		return true;
	if(token.compare("fg") == 0)
		return true;
	if(token.compare("jobs") == 0)
		return true;

	return false;
}

bool FSM::builtinGrammarCheck()
{
	//check for invalid characters
	for(int i=0; i<builtinArgVector.size(); i++)
	{
		if(strcspn(builtinArgVector[i].c_str(), " ><|!‘’\"") < strlen(builtinArgVector[i].c_str()))
		{
			std::cout << "Error: invalid input command line" << std::endl;
			return false;
		}
	}
	if(builtinCommand=="exit" || builtinCommand=="jobs")
	{
		if(builtinArgVector.size()!=0)
		{
			std::cout << builtinCommand<<": wrong number of arguments" << std::endl;
			return false;
		}
	}
	else if(builtinCommand=="cd" || builtinCommand=="fg")
	{
		if(builtinArgVector.size()!=1)
		{
			std::cout << builtinCommand<<": wrong number of arguments" << std::endl;
			return false;
		}
	}
	//true == pass grammar check
	return true;
}

bool FSM::commandGrammarCheck()
{
	//currently cammandVector.size() <= 3
	if(commandVector.size() > 3)
	{
		std::cout << "Error: invalid input command line" << std::endl;
		return false;
	}
	for(int i=0; i<commandVector.size(); i++)
	{
		if(commandVector[i]->hasInvalidChar()==true)
		{
			std::cout << "Error: invalid input command line" << std::endl;
			return false;
		}
		//check for tailing pipe
		if(i==commandVector.size()-1 && commandVector[i]->nextPipe==true)
		{
			std::cout << "Error: invalid input command line" << std::endl;
			return false;
		}
	}
	//true == pass grammar check
	return true;
}

void FSM::pushToken(std::string cmdLine, std::string token)
{
	//change state to match with current token
	//initial state is READY
	switch(state)
	{
		case READY:
			if(FSM::isBuiltin(token) == true)
				state = BUILTIN;
			else
				state = COMMAND;
			break;
		case BUILTIN:
			state = BUILTINARG;
			break;
		case BUILTINARG:
			state = BUILTINARG;
			break;
		case COMMAND:
			if(token.compare("|")==0)
				state = PIPE;
			else
				state = ARG;
			break;
		case ARG:
			if(token.compare("|")==0)
				state = PIPE;
			else
				state = ARG;
			break;
		case PIPE:
			state = COMMAND;
			break;
		default:
			std::cout<< "error error error boom" << std::endl;
			break;
	}

	//now decide current state's action
	switch(state)
	{
		case READY:
			std::cout << "READY state should not reach" << std::endl;
			break;
		case BUILTIN:
			builtinCommand = token;
			break;
		case BUILTINARG:
			builtinArgVector.push_back(token);
			break;
		case COMMAND:
			if(commandVector.size() == 0)
				commandVector.push_back(new Command(cmdLine, token, false));//3rd parameter is prevPipe
			else
				commandVector.push_back(new Command(cmdLine, token, true));
			break;
		case ARG:
			commandVector.back()->argVector.push_back(token);
			break;
		case PIPE:
			commandVector.back()->nextPipe = true;
	}
}