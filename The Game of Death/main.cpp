/*
 * Автор: Роман Жихаревич.
 * Остальные члены команды: Алексей Миронов, Игорь Богданов, Илья Чайковский, Илья Налётов.
 */

#include <cstdlib>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <csignal>
#include "game.hpp"

#if UI_USE_NCURSES
#include "ncui.hpp"
#else
#error "No UI backend selected!"
#endif

using std::size_t;
using std::string;
using std::to_string;


static void onSIGINT(int sig) {
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fputs("Usage: death <color1 color2 ...>\n"
              "The game will search for corresponding program files for each color like COLOR.dasm\n"
              "Allowed colors: green, red, yellow, blue.\n",
              stderr);
        return 1;
    }
    
    std::signal(SIGINT, onSIGINT);
    
    UIInit();
    std::atexit(UIQuit);
    
    UIDisplay gameDisplay(0, 0, -1, -1);
    
    Game game(&gameDisplay);
    
    for (int i = 1; i < argc; i++) {
        Race race;
        
        string colorString(argv[i]);
        if (colorString == "green")
            race.color = UIColorGreen;
        else if (colorString == "red")
            race.color = UIColorRed;
        else if (colorString == "yellow")
            race.color = UIColorYellow;
        else if (colorString == "blue")
            race.color = UIColorBlue;
        else
            game.fatal("Illegal color \"" + colorString + "\"!");
        
        string fn(colorString + ".dasm");
        std::ifstream code(fn);
        if (code.fail())
            game.fatal("Read failed: " + fn);
        
        string line;
        while (std::getline(code, line)) {
            std::vector<string> insn;
            size_t i = 0;
            while (true) {
                string s;
                size_t spc = line.find(' ', i);
                if (spc == s.npos)
                    s = string(line, i);
                else
                    s = string(line, i, spc - i);
                
                if (s.length())
                    insn.push_back(s);
                
                if (spc == s.npos)
                    break;
                
                i = spc + 1;
            }

            if (insn.size())
                race.insn.push_back(insn);
        }
        
        game.addRace(race);
    }
    
    try {
        game.start();
    } catch (GameExceptionRef exc) {
        game.fatal(GameExceptionString(exc));
    }
    
    game.log("=========================Finish==========================\n"
             "Results:");
    UIAttention();
    
    for (int i = 0; i < argc - 1; i++) {
        Race &race = game.raceWithIndex(i);
        
        if (race.color == UIColorBlue) {
            for (Cell &cell : race.cells)
                if (cell.weight > 0)
                    game.log("alive blue cell @ " + to_string(cell.x) + "," + to_string(cell.y));
        }
        
        string result = string("- ") + race.colorString() + ": ";
        
        if (race.extinct)
            result += "extinct after move " + to_string(race.extinctionDate) + ".";
        else {
            long weightSum = 0;
            for (Cell &cell : race.cells)
                if (cell.weight > 0)
                    weightSum += cell.weight;
            
            result += "alive with total biomass weight = " + to_string(weightSum) + ".";
        }
        
        game.log(result);
    }
    
    game.fatal("Stop.");
    
    return 0;
}
