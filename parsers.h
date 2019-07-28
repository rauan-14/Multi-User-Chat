//
//  parsers.cpp
//  MultiuserChat
//
//  Created by Rauan Zhakypbek on 6/3/18.
//  Copyright © 2018 Rauan Zhakypbek. All rights reserved.
//
#ifndef __PARSERS__
#define __PARSERS__

#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

std::pair<int, std::string> getContentUntilNewLine(std::string s, int pos);
int getHeaderIndex(std::string s);
std::vector<std::string> parse(std::string message);
std::vector<std::string> parseByDelimeter(std::string s, std::string del, int max);

std::vector<std::string> parse(std::string message) {
    /* Headers mapping
     0 - Type
     1 - Who
     2 - Name
     3 - Room
     4 - Receivers
     5 - SleepTime
     6 - Content
    */
    std::vector<std::string> headers = parseByDelimeter(message, "\n", 7);
    return headers;
}

int getHeaderIndex(std::string cur) {
    if(cur == "Type")
        return 0;
    else if(cur == "Who")
        return 1;
    else if(cur == "Name")
        return 2;
    else if(cur == "Room")
        return 3;
    else if(cur == "Receivers")
        return 4;
    else if(cur == "SleepTime")
        return 5;
    else if(cur == "Content")
        return 6;
    else
        return -1;
}

std::vector<std::string> parseByDelimeter(std::string s, std::string del, int max){
    std::vector<std::string> strs;
    int cur = 1;
    while(s.size()){
        if(cur == max)
            break;
        int i = (int)s.find(del);
        if(i != std::string::npos){
            strs.push_back(s.substr(0,i));
            s = s.substr(i+del.size());
            if(s.size()==0) {
                strs.push_back(s);
                cur++;
            }
        }else{
            strs.push_back(s);
            s = "";
        }
    }
    return strs;
}

std::string constructMessage(std::vector<std::string> headers) {
    std::string response = "";
    for(int i = 0; i < 7; i++) {
        if(i != 6)
            response += headers[i] + "\n";
        else
            response += headers[i];
    }
    return response;
}

std::string zipArray(std::vector<std::string> strs) {
    std::string response = "";
    for(auto i = 0; i < strs.size(); i++) {
        if(i != 0)
            response += "#";
        response += strs[i];
    }
    return response;
}
std::string zipArray(std::vector<std::string> strs, std::string del) {
    std::string response = "";
    for(auto i = 0; i < strs.size(); i++) {
        if(i != 0)
            response += del;
        response += strs[i];
    }
    return response;
}

#endif


