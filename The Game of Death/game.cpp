#include "game.hpp"
#include <chrono>
#include <algorithm>
#include <set>
#include <cstring>
#include <csignal>

#ifndef _WIN32
#include <unistd.h>
#endif

using std::size_t;
using std::vector;
using std::string;
using std::stoi;
using std::stoul;
using std::to_string;


GameExceptionRef GameExceptionRaceNotFound  = "Race not found!";
GameExceptionRef GameExceptionBadInsn       = "Bad instruction!";
GameExceptionRef GameExceptionBadInsnFormat = "Bad instruction format!";
GameExceptionRef GameExceptionSegFault      = "Cell segmentation fault!";
GameExceptionRef GameExceptionBadDirection  = "Bad direction!";


uint32_t GameRandom() {
    return arc4random();
}

uint32_t GameRandomBelow(uint32_t i) {
    return arc4random_uniform(i);
}


static_assert(DirectionNorth == 0, "DirectionNorth is not 0");
static_assert(DirectionEast  == 1, "DirectionEast is not 1");
static_assert(DirectionSouth == 2, "DirectionSouth is not 2");
static_assert(DirectionWest  == 3, "DirectionWest is not 3");

const char *Cell::directionString() {
    switch (direction) {
        case DirectionNorth:
            return "north";
        case DirectionEast:
            return "east";
        case DirectionSouth:
            return "south";
        case DirectionWest:
            return "west";
    }
    
    throw GameExceptionBadDirection;
}

static void movePosInDirection(int &x, int &y, Direction dir) {
    switch (dir) {
        case DirectionNorth:
            y--;
            break;
        case DirectionEast:
            x++;
            break;
        case DirectionSouth:
            y++;
            break;
        case DirectionWest:
            x--;
            break;
    }
}

Cell *Cell::nearEnemy(Game &game, Race &race) {
    Direction scanDir = direction;
    for (int i = 0; i < 4; i++) {
        int scanX = x;
        int scanY = y;
        movePosInDirection(scanX, scanY, scanDir);
        movePosInDirection(scanX, scanY, (Direction)(((int)scanDir + 3) % (DirectionMax + 1)));
        
        Direction nDir = (Direction)(((int)scanDir + 1) % 4);
        for (int j = 0; j < 3; j++) {
            UIColor color;
            Cell *enemy = game.cellAt(scanX, scanY, &color);
            if (enemy && color != race.color)
                return enemy;
            
            movePosInDirection(scanX, scanY, nDir);
        }
        
        scanDir = (Direction)(((int)scanDir + 1) % (DirectionMax + 1));
    }
    
    return nullptr;
}

void Cell::eat() {
    weight++;
}

void Cell::go(Game &game) {
    if (--weight <= 0)
        return;
    
    int dstX = x;
    int dstY = y;
    movePosInDirection(dstX, dstY, direction);
    game.moveIfPossible(x, y, dstX, dstY);
}

void Cell::clon(Game &game, Race &race) {
    if ((weight -= 10) <= 0)
        return;
    
    int dstX = x;
    int dstY = y;
    movePosInDirection(dstX, dstY, direction);
    
    if (!game.isLegal(dstX, dstY))
        return;
    
    Cell *healCell = game.cellAt(dstX, dstY);
    if (healCell) {
        if (healCell->weight <= 0)
            game.fatal("bad heal");
        
        healCell->weight += 2;
    } else {
        Cell newCell;
        newCell.x = dstX;
        newCell.y = dstY;
        
        race.cells.push_back(newCell);
        
        game.drawNewCell();
    }
}

void Cell::str(Game &game, Race &race) {
    if (--weight <= 0)
        return;
    
    Cell *enemy = nearEnemy(game, race);
    if (enemy)
        enemy->weight -= GameRandomBelow(3 + (uint32_t)weight / 2);
}

void Cell::left() {
    direction = (Direction)(((int)direction + 3) % (DirectionMax + 1));
}

void Cell::right() {
    direction = (Direction)(((int)direction + 1) % (DirectionMax + 1));
}

void Cell::back() {
    direction = (Direction)((int)direction + (direction > DirectionEast? -2 : 2));
}

void Cell::turn() {
    direction = (Direction)(GameRandomBelow(DirectionMax + 1));
}

void Cell::jg(int m, size_t addr) {
    if (weight > m)
        pc = addr;
}

void Cell::jl(int m, size_t addr) {
    if (weight < m)
        pc = addr;
}

void Cell::j(size_t addr) {
    pc = addr;
}

void Cell::je(Game &game, Race &race, size_t addr) {
    if (nearEnemy(game, race))
        pc = addr;
}

const char *Race::colorString() {
    switch (color) {
        case UIColorGreen:
            return "Green";
        case UIColorRed:
            return "Red";
        case UIColorYellow:
            return "Yellow";
        case UIColorBlue:
            return "Blue";
    }
}

vector<string> Race::fetchInsn(size_t &pc) {
    if (pc >= insn.size())
        throw GameExceptionSegFault;
    
    return insn[pc++];
}

Cell &Race::nextCell() {
    if (nextCellIndex >= cells.size())
        nextCellIndex = 0;
    
    return cells[nextCellIndex++];
}

Cell &Race::prevCell() {
    if (nextCellIndex)
        return cells[nextCellIndex - 1];
    
    return cells.back();
}


Game::Game(UIDisplay *display) {
    this->display = display;
    
    for (int x = 0; x < GameBoxWidth; x++)
        display->putChar(x, GameBoxHeight, '=');
    for (int y = 0; y < GameBoxHeight + 1; y++)
        display->putChar(GameBoxWidth, y, '|');
}

Race &Game::nextRace() {
    if (nextRaceIndex >= races.size())
        nextRaceIndex = 0;
    
    return races[nextRaceIndex++];
}

void Game::removeRaceWithColor(UIColor color) {
    for (auto i = races.begin(); i < races.end(); i++)
        if (i->color == color) {
            races.erase(i);
            break;
        }
    
    throw GameExceptionRaceNotFound;
}

void Game::addRace(Race &race) {
    races.push_back(race);
    
    for (int i = 0; i < GameInitialPopulation; i++) {
        Cell cell;
        randomEmpty(cell.x, cell.y);
        
        display->putChar(cell.x, cell.y, CellCharacter | UIAttrForColor(race.color));
        
        races[races.size() - 1].cells.push_back(cell);
    }
    
    bool *test = new bool[GameBoxWidth * GameBoxHeight];
    memset(test, false, GameBoxWidth * GameBoxHeight * sizeof(bool));
    
    for (auto &race : races) {
        for (auto &cell : race.cells) {
            if (test[cell.y * GameBoxWidth + cell.x])
                fatal("bug!");
            
            test[cell.y * GameBoxWidth + cell.x] = true;
        }
    }
}

Race &Game::raceWithIndex(size_t index) {
    if (index >= races.size())
        throw GameExceptionRaceNotFound;
    
    return races[index];
}

Race &Game::raceWithColor(UIColor color) {
    for (size_t i = 0; i < races.size(); i++)
        if (races[i].color == color)
            return races[i];
    
    throw GameExceptionRaceNotFound;
}

Cell *Game::cellAt(int x, int y, UIColor *color) {
    for (auto &race : races)
        for (auto &cell : race.cells)
            if (cell.x == x && cell.y == y &&
                cell.weight > 0) {
                if (color)
                    *color = race.color;
                return &cell;
            }
    
    return nullptr;
}

void Game::drawNewCell() {
    Race &race = races[nextRaceIndex - 1];
    Cell &cell = race.prevCell();
    display->putChar(cell.x, cell.y, CellCharacter | UIAttrForColor(race.color));
}

void Game::moveIfPossible(int &x, int &y, int dstX, int dstY) {
    if (isVisitable(dstX, dstY)) {
        x = dstX;
        y = dstY;
    }
}

bool Game::isVisitable(int x, int y) {
    return isLegal(x, y) && isEmpty(x, y);
}

bool Game::isLegal(int x, int y) {
    if (x < 0 || x >= GameBoxWidth ||
        y < 0 || y >= GameBoxHeight)
        return false;
    
    return true;
}

bool Game::isEmpty(int x, int y) {
    for (Race &race : races)
        for (Cell &cell : race.cells)
            if (cell.weight > 0 &&
                cell.x == x && cell.y == y)
                return false;
    
    return true;
}

void Game::randomEmpty(int &x, int &y) {
    do {
        x = GameRandomBelow(GameBoxWidth);
        y = GameRandomBelow(GameBoxHeight);
    } while (!isVisitable(x, y));
}

void Game::logLine(string &string) {
    int y = logY;
    int h = display->getHeight();
    
    if (y < h)
        logY++;
    else {
        y--;
        
        display->eraseLine(GameBoxHeight + 1);
        for (int y = GameBoxHeight + 2; y < h; y++)
            display->copyLine(y - 1, y);
        display->eraseLine(h - 1);
    }
    
    display->putString(0, y, string);
}

void Game::log(const char *msg) {
    while (true) {
        string line;
        
        const char *lf = std::strchr(msg, '\n');
        if (lf)
            line = string(msg, lf - msg);
        else
            line = string(msg);
        
        logLine(line);
        
        if (!lf)
            break;
        
        msg = lf + 1;
    }
}

void Game::log(const string &string) {
    log(string.c_str());
}

void Game::fatal(const char *msg) {
    log(msg);
    
    //display->setAutoFlush(false);
    
    while (true)
#ifndef _WIN32
        pause();
#else
        std::this_thread::yield();
#endif
}

void Game::fatal(const string &string) {
    fatal(string.c_str());
}

Race &Game::raceStep() {
    Race &race = nextRace();
    if (race.extinct)
        return race;
    
    Cell &cell = race.nextCell();
    for (size_t i = 1; cell.weight <= 0; i++) {
        if (i >= race.cells.size()) {
            race.extinct = true;
            return race;
        }
        
        cell = race.nextCell();
    }

    int prevX = cell.x;
    int prevY = cell.y;
    
    if (cell.repCnt) {
        for (int i = 0; i < cell.repCnt; i++)
            switch (cell.rep) {
                case CellInsnRepEat:
                    cell.eat();
                    break;
                case CellInsnRepGo:
                    cell.go(*this);
                    break;
                case CellInsnRepStr:
                    cell.str(*this, race);
                    break;
            }
        
        cell.repCnt--;
    } else {
        for (int i = 0;; i++) {
            
            
            if (i >= 30) {
                cell.weight -= 5;
                break;
            }
            
            const vector<string> insn = race.fetchInsn(cell.pc);
            
            bool
            isEat   = false,
            isGo    = false,
            isClon  = false,
            isStr   = false,
            isLeft  = false,
            isRight = false,
            isBack  = false,
            isTurn  = false,
            isJg    = false,
            isJl    = false,
            isJ     = false,
            isJe    = false;
            
            if (!(isEat   = insn[0] == "eat")   &&
                !(isGo    = insn[0] == "go")    &&
                !(isClon  = insn[0] == "clon")  &&
                !(isStr   = insn[0] == "str")   &&
                !(isLeft  = insn[0] == "left")  &&
                !(isRight = insn[0] == "right") &&
                !(isBack  = insn[0] == "back")  &&
                !(isTurn  = insn[0] == "turn")  &&
                !(isJg    = insn[0] == "jg")    &&
                !(isJl    = insn[0] == "jl")    &&
                !(isJ     = insn[0] == "j")     &&
                !(isJe    = insn[0] == "je"))
                throw GameExceptionBadInsn;
            
            if (isEat   ||
                isGo    ||
                isStr   ||
                isLeft  ||
                isRight) {
                cell.repCnt = 1;
                
                if (insn.size() == 2) {
                    if (insn[1] == "r") {
                        if (!isEat &&
                            !isGo)
                            throw GameExceptionBadInsnFormat;
                        
                        cell.repCnt = GameRandomBelow(6);
                    } else {
                        try {
                            cell.repCnt = stoi(insn[1], nullptr, 0);
                        } catch (...) {
                            cell.repCnt = -1;
                        }
                        
                        if (cell.repCnt < 2 || cell.repCnt > 99)
                            throw GameExceptionBadInsnFormat;
                    }
                } else if (insn.size() > 2)
                    throw GameExceptionBadInsnFormat;
                
                if (cell.repCnt) {
                    if (isEat) {
                        cell.rep = CellInsnRepEat;
                        cell.eat();
                    } else if (isGo) {
                        cell.rep = CellInsnRepGo;
                        cell.go(*this);
                    } else if (isStr) {
                        cell.rep = CellInsnRepStr;
                        cell.str(*this, race);
                    } else if (isLeft) {
                        for (int j = 0; j < cell.repCnt && i < 30; j++)
                            cell.left();
                    } else
                        cell.right();
                    
                    cell.repCnt--;
                }
            } else if (isClon)
                cell.clon(*this, race);
            else if (isBack)
                cell.back();
            else if (isTurn) {
                if (insn.size() != 2 ||
                    insn[1] != "r")
                    throw GameExceptionBadInsnFormat;
                
                cell.turn();
            } else if (isJg ||
                       isJl) {
                if (insn.size() != 3)
                    throw GameExceptionBadInsnFormat;
                
                int m;
                size_t addr;
                try {
                    m    = stoi(insn[1], nullptr, 0);
                    addr = stoul(insn[2], nullptr, 0);
                } catch (...) {
                    throw GameExceptionBadInsnFormat;
                }
                
                if (isJg)
                    cell.jg(m, addr);
                else
                    cell.jl(m, addr);
            } else if (isJ ||
                       isJe) {
                if (insn.size() != 2)
                    throw GameExceptionBadInsnFormat;
                
                size_t addr;
                try {
                    addr = stol(insn[1], nullptr, 0);
                } catch (...) {
                    throw GameExceptionBadInsnFormat;
                }
                
                if (isJ)
                    cell.j(addr);
                else
                    cell.je(*this, race, addr);
            }
            
            if (isEat  ||
                isGo   ||
                isClon ||
                isStr)
                break;
        }
    }
    
    if (cell.x != prevX ||
        cell.y != prevY) {
        display->putChar(prevX, prevY, ' ');
        display->putChar(cell.x, cell.y, CellCharacter | UIAttrForColor(race.color));
    }
    
    return race;
}

void Game::extinctionAlert(const char *colorString) {
    log(string("[ATTENTION] ") + colorString + string(" race extinct!"));
    UIAttention();
}

void Game::start() {
    bool cont = true;
    for (int i = 0; i < GameMoveNumber && cont; i++) {
        cont = false;
        
        for (size_t j = 0; j < races.size(); j++)
            raceStep();
        
        for (Race &race : races) {
            if (!race.extinct) {
                cont = true;
            } else if (race.extinctionDate == RaceExtinctionDateNone) {
                extinctionAlert(race.colorString());
                race.extinctionDate = i + 1;
            }
        }
        
        if (GameStepDelay)
            std::this_thread::sleep_for(std::chrono::milliseconds(GameStepDelay));
    }
}
