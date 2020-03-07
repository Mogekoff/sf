#include "iNTER.h"

iNTER::iNTER(const int argc, char* argv[]) {
	Print("[SYSTEM} Initialize iNTER...");
	
	Status = INITCODE;
	CurrentDirectory = argv[0];
	CurrentDirectory=CurrentDirectory.substr(0,CurrentDirectory.find_last_of('\\')+1);

	AddFunction("pwd", "Print current directory", [=]() {
		Print(CurrentDirectory);
		});

	AddFunction("exit", "Exit from program", [=]() {
		cout << '\b';
		Status = EXITCODE;
		});

	AddFunction("version", "Print current version of program", [=]() {
		Print("Current version of iNTER is " + VERSION);
		});

	AddFunction("help", "Print this list of all available commands", [=]() {
		string List = "List of available commands:\n";
		for (auto i : Functions)
			List += i.first + 
							[i] {
								string str="\t\t\t\t\t\t\t\t"; 
								for(int j = 0; j <= i.first.size()/8; j++, str.pop_back());
								return str;
							}() 
							+ i.second.first + '\n';
			List.pop_back();
			Print(List);
		});

	AddFunction("clear", "Clear terminal", [=]() {
		cout << "\033[2J\033[1;1H>";
		});

	AddFunction("exec", "Execute typed command by system shell. EXAMPLE: \"exec dir\"", [=]() {
		string Temp;
		if (CurrentParameters.empty()) {
			Print("[ERROR} No parameters were provided");
			return;
		}
		for(auto str : CurrentParameters)
			Temp += str + " ";
		Temp.pop_back();
		
		system((CurrentDirectory + Temp).c_str());
		Print(">");
		});

	signal(SIGINT, iNTER::SigintHandler);
	
}

int iNTER::Interpreter(istream& Stream, string Input) {
	Status = RUNTIME;
	thread ExThread(&iNTER::Executor, this);

	
	string Command,
		   Name,
		   Arguments;
	
	
	while (!Stream.eof() && getline(Stream, Input)) {
		Log('>' + Input);
		Print(">");

		auto Commentary = Input.find('#');
		if(Commentary!=-1)
			Input = Input.substr(0,Commentary);

		if (Input.empty()) {
			Print(">");
			continue;
		}

		if(Input.back()!=';')
			Input.push_back(';');

		for (auto EndOfCommand = Input.find(';'); EndOfCommand != -1 && !Input.empty(); ) {
			while(Input.front()==' ' || Input.front()=='\t' || Input.front() == '\n')
				Input.replace(0,1,"");
			EndOfCommand = Input.find(';');

			Command = Input.substr(0, EndOfCommand);
			
			auto Temp = Command.find(' ');
			if(Temp!=-1) {
				Name = Command.substr(0, Temp);
				Arguments = Command.substr(Temp+1, EndOfCommand);
				vector<string> Parameters;
				string Buffer;
				stringstream ss(Arguments);
				while (ss >> Buffer)
					Parameters.push_back(Buffer);
				ExecuteCommand(Name, Parameters);
			}
			else {
				Name = Command.substr(0,EndOfCommand);
				Arguments.clear();
				ExecuteCommand(Name);
			}
			

			Input = Input.substr(EndOfCommand + 1, Input.back());
			
			if (Name == "exit") {
				this_thread::sleep_for(1ms);
				
				if(!Input.empty())
					Print("[WARNING} More commands were entered, but exit command was executed.");

				if (!Queue.empty())
					Print("[SYSTEM} Waiting for the execution of the remaining commands...");

				ExThread.join();

				Print("[SYSTEM} Exiting..."); cout << '\b';
				Log("Program exited with code " + to_string(EXITCODE));

				return EXITCODE;
			}
		}
	}
	ExecuteCommand("exit");
	ExThread.join();
	Print("[SYSTEM} Script successfully executed.");
	return EXITCODE;
}

void iNTER::Executor() {
	while (Status == RUNTIME)
		while (!Queue.empty()) {
			try {
				CurrentParameters=Queue.front().second;
				Queue.front().first();
			}
			catch (...) {
				iNTER::Print("[ERROR} An error was generated by the executable command. The command can't be executed.");
				}
			Queue.pop();
		}
}

void iNTER::Print(string Str) {
	if (Str == ">")
		cout << "\b>";
	else {
		string Output = Time() + Str;
		cout << '\b' << Output << endl << '>';
		Log(Output);
	}
}

void iNTER::Log(string Str) {
	ofstream File("Log.txt", ios_base::app);
	File << Str << endl;
	File.close();
}

void iNTER::AddFunction(string CmdName, string CmdDescription, function<void()> CmdFunction) {
	Functions[CmdName] = { CmdDescription, CmdFunction };
}

void iNTER::ExecuteCommand(string Cmd, vector<string> Parameters) {
	auto Iterator = Functions.find(Cmd);

	if (Iterator != Functions.end())
		Queue.push({ Iterator->second.second, Parameters });
	else
		Print("[SYSTEM} Command \'" + Cmd + "\' is not found. Type 'help' to see a list of available commands.");
}


string iNTER::Time() {
	time_t Temp = time(NULL);
	tm* Time = localtime(&Temp);
	return '[' + to_string(Time->tm_hour) + ':' + to_string(Time->tm_min) + ':' + to_string(Time->tm_sec) + "] ";
}

void iNTER::SigintHandler(int Signal) {
	Log('[' + to_string(Signal) + "] Terminated by user pressing CTLR+C");
}

void iNTER::SigabrtHandler(int Signal) {
	Log('[' + to_string(Signal) + "] Terminated by abort()");
}


int iNTER::GetArgInt(unsigned NumberOfArgument) {
	if (NumberOfArgument >= CurrentParameters.size()) {
		Print("[ERROR} The " + to_string(NumberOfArgument+1) + " argument was requested, but only "+ to_string(CurrentParameters.size()) +" were introduced. Returned -1");
		return -1;
	}

	int Result;
	try {
		Result = stoi(CurrentParameters[NumberOfArgument]);
	}
	catch (...) {
		Print("[ERROR} Can't transform '" + CurrentParameters[NumberOfArgument] + "' to integer number. Returned -1");
		return -1;
	}
	return Result;
}

string iNTER::GetArgStr(unsigned NumberOfArgument) {
	if (NumberOfArgument > CurrentParameters.size()) {
		Print("[ERROR} The " + to_string(NumberOfArgument+1) + " argument was requested, but only " + to_string(CurrentParameters.size()) + " were introduced. Returned 'ERROR'");
		return "ERROR";
	}
	return CurrentParameters[NumberOfArgument];
}

void iNTER::ExecuteScript(string Dir) {
	Print("[SYSTEM} '" +Dir + "' script execution begins");
	ifstream Script(Dir, ios::in);
	string Command;
	if (Script.is_open()) {
		getline(Script, Command);
		if (Command != "#iNTER") {
			Print("[ERROR} Missing '#iNTER' directive in beginning of script");
			return;
		}
		else
			Interpreter(Script, Command);
	} else 
		Print("[ERROR} Unable to open file of script");
}