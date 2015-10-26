#include <vector>
#include <iostream>

#include "Jobs.h"

Jobs::Jobs(std::vector<int> pidVector, std::string cmdLine)
{
	this->pidVector = pidVector;
	this->cmdLine = cmdLine;
}
Jobs::~Jobs()
{

}