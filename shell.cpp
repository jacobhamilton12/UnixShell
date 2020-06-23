#include<iostream>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bits/stdc++.h> 

using namespace std;
string trim (string input){ //clears outer whitespace
    int i=0;
    while (i < input.size() && input [i] == ' ')
        i++;
    if (i < input.size())
        input = input.substr (i);
    else{
        return "";
    }
    
    i = input.size() - 1;
    while (i>=0 && input[i] == ' ')
        i--;
    if (i >= 0)
        input = input.substr (0, i+1);
    else
        return "";
    
    return input;
    

}

vector<string> split (string line, string separator=" "){
    vector<string> result;
    bool quotes = false;
    size_t found;
    while (line.size()){
        if(!quotes)
            found = line.find(separator);
        quotes = false;
        size_t sing1 = line.find("\'");
        size_t sing2 = line.find("\'", sing1+1);
        size_t doub1 = line.find("\"");
        size_t doub2 = line.find("\"", doub1+1);
        if (found == string::npos){
            string lastpart = trim (line);
            if (lastpart.size()>0){
                result.push_back(lastpart);
            }
            break;
        }
        if(found > sing1 && found < sing2 && sing1 != string::npos){
            found = line.find(separator, found+1);
            quotes = true;
            continue;
        }
        if(found > doub1 && found < doub2 && doub1 != string::npos){
            found = line.find(separator, found+1);
            quotes = true;
            continue;
        }
        string segment = trim (line.substr(0, found));
        //cout << "line: " << line << "found: " << found << endl;
        line = line.substr (found+1);

        //cout << "[" << segment << "]"<< endl;
        if (segment.size() != 0) 
            result.push_back (segment);

        
        //cout << line << endl;
    }
    return result;
}

char** vec_to_char_array (vector<string> parts){
    char ** result = new char * [parts.size() + 1]; // add 1 for the NULL at the end
    for (int i=0; i<parts.size(); i++){
        // allocate a big enough string
        result [i] = new char [parts [i].size() + 1]; // add 1 for the NULL byte
        strcpy (result [i], parts[i].c_str());
    }
    result [parts.size()] = NULL;
    return result;
}


void execute (string command){
    vector<string> argstrings = split (command, " "); // split the command into space-separated parts
    for(int i = 0; i < argstrings.size(); i++){
        if(argstrings[i] == "\'"){
            argstrings.erase(argstrings.begin()+i);
            while(i < argstrings.size() -1 && argstrings[i+1] != "\'"){
                argstrings[i] += " " + argstrings[i+1];
                argstrings.erase(argstrings.begin()+i+1);
            }
            argstrings.erase(argstrings.begin()+i+1);
        }else if(argstrings[i].substr(0,1) == "\'"){
            argstrings[i] = argstrings[i].substr(1,argstrings[i].size()-1);
            if(argstrings[i].substr(argstrings[i].size()-1, 1) == "\'"){
                argstrings[i] = argstrings[i].substr(0, argstrings[i].size()-1);
                break;
            }
            while(i < argstrings.size() -1 && argstrings[i+1].substr(argstrings[i+1].size()-1, 1) != "\'"){
                argstrings[i] += " " + argstrings[i+1];
                argstrings.erase(argstrings.begin()+i+1);
            }
            argstrings[i] += " " + argstrings[i+1].substr(0,argstrings[i+1].size()-1);
            argstrings.erase(argstrings.begin()+i+1);
        }
        if(argstrings[i] == "\""){
            argstrings.erase(argstrings.begin()+i);
            while(i < argstrings.size() -1 && argstrings[i+1] != "\""){
                argstrings[i] += " " + argstrings[i+1];
                argstrings.erase(argstrings.begin()+i+1);
            }
            argstrings.erase(argstrings.begin()+i+1);
        }else if(argstrings[i].substr(0,1) == "\""){
            argstrings[i] = argstrings[i].substr(1,argstrings[i].size()-1);
            if(argstrings[i].substr(argstrings[i].size()-1, 1) == "\""){
                argstrings[i] = argstrings[i].substr(0, argstrings[i].size()-1);
                break;
            }
            while(i < argstrings.size() -1 && argstrings[i+1].substr(argstrings[i+1].size()-1, 1) != "\""){
                argstrings[i] += " " + argstrings[i+1];
                argstrings.erase(argstrings.begin()+i+1);
            }
            argstrings[i] += " " + argstrings[i+1].substr(0,argstrings[i+1].size()-1);
            argstrings.erase(argstrings.begin()+i+1);
        }
    }
    char** args = vec_to_char_array (argstrings);// convert vec<string> into an array of char*
    execvp (args[0], args);
}

int main (){
    vector<int> backgroundProcesses;
    while (true){ // repeat this loop until the user presses Ctrl + C
        string commandline;//get from STDIN, e.g., "ls  -la |   grep Jul  | grep . | grep .cpp" 
        getline(cin, commandline);
        cin.clear();
        int stdin = dup(0);
        int stdout = dup(1);
        // split the command by the "|", which tells you the pipe levels
        bool background = false;
        int numBackground = 0;
        if(commandline.substr(commandline.size()-1) == "&"){
            commandline = commandline.substr(0,commandline.size()-1);
            background = true;
            numBackground++;
        }
        vector<string> tparts = split (commandline, "|");
        vector<string> outstream = split(tparts[tparts.size()-1], ">");
        string out = outstream[outstream.size()-1];
        tparts[tparts.size()-1] = outstream[0];
        bool has_output = outstream.size() == 2;
        //TODO defuncs happen with multiple background processes
        // for each pipe, do the following:
        for (int i=0; i<tparts.size(); i++){
            // make pipe
            int fd[2];
            pipe(fd);
            int pid = fork();
			if (pid == 0){ //out child process
                // redirect output to the next level
                // unless this is the last level
                if(tparts[i].find("<") != string::npos){
                    vector<string> instream = split(tparts[i], "<");
                    if(instream.size() > 1){
                        string in = instream[1];
                        tparts[i] = instream[0];
                        int infile = open(in.c_str(), ios::in);
                        dup2(infile, 0);
                        close(infile);
                    }
                }
                if (i < tparts.size() - 1){
                    dup2(fd[1], 1);// redirect STDOUT to fd[1], so that it can write to the other side
                    close (fd[1]);   // STDOUT already points fd[1], which can be closed
                }else if(has_output){ //handles output to file
                    int outfile = open(out.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    dup2(outfile, 1);
                    close(outfile);
                }
                //execute function that can split the command by spaces to 
                // find out all the arguments, see the definition
                execute (tparts[i]); // this is where you execute
            }else{ //in
                if(!background){// wait for the child process
                    wait(0);    
                }        
                else{
                    backgroundProcesses.push_back(pid);
                }
				// then do other redirects
                dup2(fd[0],0);
                close(fd[1]);
            }
        }
        //if(!background)
          //  wait(0); 
        int w = -1;
        for(int i : backgroundProcesses)
            w = waitpid(i, 0, WNOHANG);
            if(w > 0){
                waitpid(w, 0, 0);
                remove(backgroundProcesses.begin(), backgroundProcesses.end(), w);
            }
        background = false;
        dup2(stdin, 0);
        dup2(stdout, 1);
        close(stdin);
        close(stdout);
        commandline.clear();
    }
}