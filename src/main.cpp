#include <iostream>
#include <vector>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/time.h>
#include <iomanip>
#include <chrono>

void sigint(int signal __attribute__((unused))){
}

int changeDirectory(char* path){
    int retval = chdir(path);
    if (retval != 0){
        std::cerr << "Error please specify valid path\n";
    }
    return retval;
}

void handleCD(std::string& command){
    if(command.length() == 2){
        std::cerr << "Please specify path\n";
    }
    else{
        std::string path = command.substr(3, command.length() - 3);
        char *cpath = strdup(path.c_str());
        changeDirectory(cpath);
        free(cpath);
    }
}

void handlePipes(std::string command){
    std::vector<std::vector<char*>> allCmds;
    std::vector<char*> cmd;
    std::stringstream ss(command);
    std::string temp;
    while (ss >> temp){
        if (temp == "|"){
            cmd.push_back(nullptr);
            allCmds.push_back(cmd);
            cmd.clear();
        }
        else{
            cmd.push_back(strdup(temp.c_str()));
        }
    }
    cmd.push_back(nullptr);
    allCmds.push_back(cmd);

    int numOfPipes = allCmds.size() -1;
    int fd[2 * numOfPipes];
    for(int i = 0; i < numOfPipes; i++){
        if(pipe(fd + i * 2) != 0){
            std::cerr << "pipe() failed because: " << strerror(errno) << std::endl;
            exit(1);
        }
    }

    int j = 0;
    for(unsigned int i = 0; i < allCmds.size(); i++){
        pid_t pid = fork();
        if (pid == 0){
            // if its not the last command child outputs to next
            if (i != allCmds.size() -1){
                dup2(fd[i * 2 + 1], STDOUT_FILENO);
            }

            //if its not the first command child gets input from previous
            if (i != 0){
                dup2(fd[(i - 1) * 2], STDIN_FILENO);
            }

            for(int i = 0; i< 2 * numOfPipes; i++){
                close(fd[i]);
            }
            int retval = execvp(allCmds[i][0], allCmds[i].data());
            if (retval == -1){
                std::cerr << cmd[0] << " failed because " << strerror(errno) << "\n";
                exit(1);
            }
        }
        j+=2;
    }

    for(int i = 0; i < 2 * numOfPipes; i++){
        close(fd[i]);
    }

    int status;
    for(int i = 0; i < numOfPipes + 1; i++){
        wait(&status);
    }
}

double handleExec(std::string command){
    std::vector<char*> cmd;
    std::stringstream ss(command);
    std::string temp;

    while(ss >> temp){
        cmd.push_back(strdup(temp.c_str()));
    }
    cmd.push_back(nullptr);

    auto start = std::chrono::high_resolution_clock::now();
    pid_t pid = fork();
    if (pid == 0){
        int retval = execvp(cmd[0], cmd.data());
        if (retval == -1){
            std::cerr << cmd[0] << " failed because " << strerror(errno) << "\n";
            exit(1);
        }
    }
    else{
        int status;
        wait(&status);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    for(unsigned int i = 0; i < cmd.size(); i++){
        free(cmd[i]);
    }
    return total.count();
}

std::string handleHistory(std::vector<std::string>& history, std::string command){
    if (command.substr(1, 1) != " "){
        std::cerr << "Invalid format, should be \'^ #\' \n";
        return command;
    }
    else{
        unsigned int wantedCommand = std::stoi(command.substr(2, command.length() - 2));
        if(wantedCommand > history.size()){
            std::cerr << "Error: invalid request\n";
            return command;
        }
        else{
            return history[wantedCommand-1];
        }
    }
}

void livingTime(timeval start, timeval end){
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

void runningTime(float totalTime){
    int hours = 0;
    int minutes = 0;
    while(totalTime >= 3600){
        hours++;
        totalTime-=3600;
    }
    while(totalTime >= 60){
        minutes++;
        totalTime-= 60;
    }
    std::cout << std::setfill('0') << std::setw(2) << hours << ":" << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0') << std::setw(2) << totalTime << "\n";
}

void prettyPrintPtime(double pTimeCount){
    std::cout << std::fixed;
    std::cout << std::setprecision(4);
    std::cout << "Time spent executing child processes: " << pTimeCount << " seconds\n";
}

void printDirs(std::vector<std::string>& dirs){
    for(int i = dirs.size() - 1; i >= 0; i--){
        std::cout << dirs[i] << " ";
    }
    std::cout << "\n";
}

void printHistory(std::vector<std::string>& commandHistory){
    std::cout << "-- Command History --\n\n";
    for(unsigned int i = 0; i < commandHistory.size(); i++){
        std::cout << i+1 << " : " << commandHistory[i] << "\n";
    }
    std::cout << "\n";
}

std::string printWorkingDirectory(){
    char wd[FILENAME_MAX];
    char* dir = getcwd(wd, FILENAME_MAX);
    return std::string(dir);
}

void handlePopD(std::vector<std::string>& directoryHistory){
    if (directoryHistory.size() == 1){
        std::cerr << "Directory stack empty\n";
    }
    else{
        directoryHistory.pop_back();
        std::string newDir = directoryHistory[directoryHistory.size() - 1];
        char* cpath = strdup(newDir.c_str());
        changeDirectory(cpath);
        printDirs(directoryHistory);
        free(cpath);
    }
}

void handlePushD(std::string command, std::vector<std::string>& directoryHistory){
    if (command.length() == 5){
        std::cerr << "Please specify directory\n";
    }
    else{
        std::string path = command.substr(6, command.length() - 6);
        char* cpath = strdup(path.c_str());
        if(changeDirectory(cpath) == 0){
            directoryHistory.push_back(printWorkingDirectory());
            printDirs(directoryHistory);
        }
        free(cpath);
    }
}

int main() {
    signal(SIGINT, sigint);
    struct timeval start, end;
    gettimeofday(&start, NULL);
    std::vector<std::string> commandHistory;
    std::vector<std::string> directoryHistory(1);
    std::string command;
    double pTimeCount = 0.0;
    clock_t startClock;

    startClock = clock();

    while (true){
        directoryHistory[directoryHistory.size() -1] = printWorkingDirectory();
        std::cout << "[" << printWorkingDirectory() << ":] ";
        std::getline(std::cin, command);

        if(command.substr(0, 1) == "^"){
            command = handleHistory(commandHistory, command);
        }
        if (command == "exit"){
            break;
        }
        else if (command == "ptime"){
            prettyPrintPtime(pTimeCount);
        }
        else if (command == "history"){
            printHistory(commandHistory);
        }
        else if (command.substr(0, 2) == "cd"){
            handleCD(command);
        }
        else if (command.substr(0, 3) == "pwd" && command.length() == 3){
            std::cout << printWorkingDirectory() << std::endl;
        }
        else if (command == "dirs"){
            printDirs(directoryHistory);
        }
        else if (command.substr(0, 5) == "pushd"){
            handlePushD(command, directoryHistory);
        }
        else if (command.substr(0, 4) == "popd"){
            handlePopD(directoryHistory);
        }
        else if (command == "living_time"){
            livingTime(start, end);
        }
        else if (command == "running_time"){
            float totalClock = clock() - startClock;
            float runningTotal = (float)totalClock / CLOCKS_PER_SEC;
            runningTime(runningTotal);
        }
        else if (command.find("|") != std::string::npos){
            handlePipes(command);
        }
        else{
            pTimeCount += handleExec(command);
        }
        commandHistory.push_back(command);
    }
    return 0;
}