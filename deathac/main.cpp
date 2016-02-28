#include <iostream>
#include <cstdio>
#include <fstream>
#include <vector>
#include <map>
#include <string>

using namespace std;


static void openFailed(const char *fn) {
    cerr << "Open failed: " << fn << endl;
    exit(1);
}

int main(int argc, const char * argv[]) {
    if (argc != 3) {
        cerr << "Usage: deathac <infile> <outfile>" << endl;
        return 1;
    }
    
    vector<vector<string>> insns;
    
    ifstream autocode(argv[1]);
    if (autocode.fail())
        openFailed(argv[1]);
    
    string   line;
    while (getline(autocode, line)) {
        vector<string> insn;
        
        size_t i = 0;
        size_t d;
        do {
            string s;
            d = line.find(' ', i);
            if (d == s.npos)
                s = string(line, i);
            else
                s = string(line, i, d - i);
            
            if (s.length())
                insn.push_back(s);
            
            i = d + 1;
        } while (d != string::npos);
        
        if (insn.size())
        insns.push_back(insn);
    }
    
    if (autocode.fail() && !autocode.eof()) {
        cerr << "Read failed: " << argv[1] << endl;
        return 1;
    }
    
    autocode.close();
    
    map<string, size_t> labels;
    for (size_t i = 0, pc = 0; i < insns.size(); i++) {
        if (insns[i].size() == 1 && insns[i][0][0] == '!')
            labels[insns[i][0].substr(1)] = pc;
        else
            pc++;
    }
    
    ofstream output(argv[2]);
    if (output.fail())
        openFailed(argv[2]);
    
    for (auto &insn : insns) {
        if (insn[0][0] == '!')
            continue;
        
        output << insn[0];
        
        for (size_t i = 1; i < insn.size(); i++) {
            if (insn[i][0] == '$') {
                string label = insn[i].substr(1);
                
                if (labels.find(label) == labels.end()) {
                    cerr << "Use of undeclared label '" << label << "'." << endl;
                    remove(argv[2]);
                    return 1;
                }
                
                insn[i] = to_string(labels[label]);
            }
            
            output << " " << insn[i];
            if (output.fail()) {
                cerr << "Write failed: " << argv[2] << endl;
                remove(argv[2]);
                return 1;
            }
        }
        
        output << endl;
    }
    
    output.close();
    
    return 0;
}
