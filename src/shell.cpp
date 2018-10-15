#include "shell.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>

std::vector<char*> parseArgument(std::string command){
    std::vector<char*> commandLineArguments;
    std::stringstream ss(command);
    std::string tmp;

    while(ss >> tmp){
        commandLineArguments.push_back(strdup(tmp.c_str()));
    }
    commandLineArguments.push_back(nullptr);
    return commandLineArguments;
}

void Shell::run(){
    std::vector<std::string> commandHistory;
    std::string command;
    bool exitProgram = false;
    while(!exitProgram){
        bool execCommand = true;
        std::cout << "[cmd]: ";
        std::getline(std::cin, command);

        if(command == "ptime"){
            execCommand = false;
        }
        else if(command == "history"){
            execCommand = false;
            std::cout << "-- Command History --\n\n";
            for(unsigned int i = 0; i < commandHistory.size(); i++){
                std::cout << i+1 << " : " << commandHistory[i] << "\n";
            }
            std::cout << "\n";

        }
        else if(command.substr(0, 1) == "^"){
            execCommand = false;
            if (command.substr(1, 1) != " "){
                std::cout << "Invalid format, should be \'^ #\'\n\n";
            }
            else{
                unsigned int wantedCommand = atoi(command.substr(2, command.length() -2).c_str());
                if(wantedCommand > commandHistory.size()){
                    std::cout << "Error: invalid request\n\n";
                }
                else{
                    execCommand = true;
                    command = commandHistory[wantedCommand-1];
                }
            }
        }
        else if(command == "exit"){
            execCommand = false;
            exitProgram = true;
        }
        else if (execCommand){
            std::vector<char*> cmd = parseArgument(command);
            pid_t pid = fork();
            if (pid != 0){
                wait(NULL);
            }
            else{
                int retval = execvp(cmd[0], cmd.data());
                if(retval == -1){
                    std::cout << "\nError: The command " << command << " is not valid\n\n";
                    exit(0);
                }
            }
        }
        commandHistory.push_back(command);
    }
}