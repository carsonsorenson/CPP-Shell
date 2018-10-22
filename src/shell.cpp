#include "shell.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/time.h>
#include <iomanip>

/*
    int wstatus;
    wait(&wstatus);  // parent process waits here

    if (WEXITSTATUS(wstatus) != 0) {
    }
*/

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

int changeDirectory(const char* path){
    int retval = chdir(path);
    if (retval != 0){
        std::cerr << "Error please specify valid path\n";
    }
    return retval;
}

std::string printWorkingDirectory(){
    char wd[FILENAME_MAX];
    char* dir = getcwd(wd, FILENAME_MAX);
    return std::string(dir);
}

void printDirs(std::vector<std::string>& dirs){
    for(int i = dirs.size() - 1; i >= 0; i--){
        std::cout << dirs[i] << " ";
    }
    std::cout << "\n";
}

void Shell::run(){
    struct timeval start, end;
    gettimeofday(&start, NULL);
    std::vector<std::string> commandHistory;
    std::vector<std::string> directoryHistory(1);
    std::string command;
    while(true){
        directoryHistory[directoryHistory.size() - 1] = printWorkingDirectory();
        std::cout << "[cmd]: ";
        std::getline(std::cin, command);

        if(command.substr(0, 1) == "^"){
            if (command.substr(1, 1) != " "){
                std::cout << "Invalid format, should be \'^ #\'\n\n";
            }
            else{
                unsigned int wantedCommand = atoi(command.substr(2, command.length() -2).c_str());
                if(wantedCommand > commandHistory.size()){
                    std::cout << "Error: invalid request\n\n";
                }
                else{
                    command = commandHistory[wantedCommand-1];
                }
            }
        }

        if(command == "ptime"){
        }
        else if(command == "history"){
            std::cout << "-- Command History --\n\n";
            for(unsigned int i = 0; i < commandHistory.size(); i++){
                std::cout << i+1 << " : " << commandHistory[i] << "\n";
            }
            std::cout << "\n";

        }
        else if(command.substr(0, 2) == "cd"){
            if (command.length() == 2){
                std::cerr << "Please specify path\n";
            }
            else{
                std::string path = command.substr(3, command.length() - 3);
                const char *cpath = path.c_str();
                changeDirectory(cpath);
            }
        }
        else if(command.substr(0, 3) == "pwd" && command.length() == 3){
            std::cout << printWorkingDirectory() << std::endl;
        }
        else if (command.substr(0, 4) == "dirs"){
            printDirs(directoryHistory);
        }
        else if (command.substr(0, 5) == "pushd"){
            if (command.length() == 5){
                std::cerr << "Please specify directory\n";
            }
            else{
                std::string path = command.substr(6, command.length() - 6);
                const char* cpath = path.c_str();
                if (changeDirectory(cpath) == 0){
                    directoryHistory.push_back(printWorkingDirectory());
                    printDirs(directoryHistory);
                }
            }
        }
        else if (command.substr(0, 4) == "popd"){
            if (directoryHistory.size() == 1){
                std::cerr << "Directory stack empty\n";
            }
            else{
                directoryHistory.pop_back();
                std::string newDir = directoryHistory[directoryHistory.size() - 1];
                const char* cpath = newDir.c_str();
                changeDirectory(cpath);
                printDirs(directoryHistory);
            }
        }
        else if (command == "living_time"){
            gettimeofday(&end, NULL);
            int seconds = (end.tv_sec - start.tv_sec);
            int hours = 0;
            int minutes = 0;
            while(seconds >= 3600){
                hours++;
                seconds-=3600;
            }
            while(seconds >= 60){
                minutes++;
                seconds-= 60;
            }
            std::cout << std::setfill('0') << std::setw(2) << hours << ":" << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0') << std::setw(2) << seconds << "\n";
        }
        else if(command == "exit"){
            break;
        }
        else{
            std::vector<char*> cmd = parseArgument(command);
            pid_t pid = fork();
            if (pid != 0){
                int status;
                wait(&status);
                if (WEXITSTATUS(status)){
                    std::cerr << "Child process had this problem: " << strerror(WEXITSTATUS(status)) << std::endl;
                }
            }
            else{
                int retval = execvp(cmd[0], cmd.data());
                if(retval == -1){
                    std::cerr << cmd[0] << " did something wrong\n";
                    exit(1);
                }
            }
        }
        commandHistory.push_back(command);
    }
}