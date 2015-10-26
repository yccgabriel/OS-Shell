class Command
{
	public:
		std::string cmdLine;
		std::string command;
		std::vector<std::string> argVector;
		bool prevPipe;
		bool nextPipe;
	public:
		Command(std::string, std::string, bool);
		bool hasInvalidChar();
		void fdAssignment(int *, int *, int);
		void wildcardExpand(int);

	private:
		
	private:
		void forkProcess();
};