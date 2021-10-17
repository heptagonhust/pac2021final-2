#include "sequence.h"
#include <string_view>

Sequence::Sequence(){
}

Sequence::Sequence(string_view seq){
    mStr = seq;
}

Sequence::Sequence(char* str){
    rawStr = str;
    mStr = string_view{str};
}

Sequence::~Sequence(){
    if (rawStr != nullptr) {
        delete [] rawStr;
    }
}
void Sequence::print(){
    std::cerr << mStr;
}

int Sequence::length(){
    return mStr.length();
}

Sequence Sequence::reverseComplement(){
    char *str = new char[mStr.length()];
    for(int c=0;c<mStr.length();c++){
        char base = mStr[c];
        switch(base){
            case 'A':
            case 'a':
                str[mStr.length()-c-1] = 'T';
                break;
            case 'T':
            case 't':
                str[mStr.length()-c-1] = 'A';
                break;
            case 'C':
            case 'c':
                str[mStr.length()-c-1] = 'G';
                break;
            case 'G':
            case 'g':
                str[mStr.length()-c-1] = 'C';
                break;
            default:
                str[mStr.length()-c-1] = 'N';
        }
    }
    return Sequence(str);
}

Sequence Sequence::operator~(){
    return reverseComplement();
}

// bool Sequence::test(){
//     Sequence s("AAAATTTTCCCCGGGG");
//     Sequence rc = ~s;
//     if (s.mStr != "AAAATTTTCCCCGGGG" ){
//         cerr << "Failed in reverseComplement() expect AAAATTTTCCCCGGGG, but get "<< s.mStr;
//         return false;
//     }
//     if (rc.mStr != "CCCCGGGGAAAATTTT" ){
//         cerr << "Failed in reverseComplement() expect CCCCGGGGAAAATTTT, but get "<< rc.mStr;
//         return false;
//     }
//     return true;
// }