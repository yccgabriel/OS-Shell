class Jobs
{
	public:
		std::vector<int> pidVector;
		std::string		 cmdLine;
	public:
		Jobs(std::vector<int>, std::string);
		~Jobs();
};