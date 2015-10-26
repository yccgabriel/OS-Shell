class Command;

class FSM
{
	public://public variables
		std::vector<Command*> commandVector;
		std::string builtinCommand;
		std::vector<std::string> builtinArgVector;
	public://public methods
		FSM();
		void pushToken(std::string, std::string);
		bool builtinGrammarCheck();
		bool commandGrammarCheck();
	private://private variables
		enum State {READY,BUILTIN,BUILTINARG,COMMAND,ARG,PIPE};
		State state;
	private://private methods
		bool isBuiltin(std::string);
};