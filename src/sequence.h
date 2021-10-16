#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string_view>

using namespace std;

class Sequence{
public:
    Sequence();
    Sequence(string_view seq);
    Sequence(char* str);
    ~Sequence();
    void print();
    int length();
    Sequence reverseComplement();

    Sequence operator~();

    // static bool test();

public:
    string_view mStr;
    char* rawStr = nullptr;
};

#endif