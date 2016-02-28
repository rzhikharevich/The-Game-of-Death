#ifndef GAME_HPP
#define GAME_HPP


#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include "config.hpp"
#include "ncui.hpp"


#if defined(__clang__) || defined(__GNUC__)
#define NORETURN __attribute__((__noreturn__))
#else
#define NORETURN
#endif


class Game;
typedef struct Race Race;

typedef const char *const GameExceptionRef;
extern GameExceptionRef GameExceptionRaceNotFound;
extern GameExceptionRef GameExceptionBadInsn;
extern GameExceptionRef GameExceptionBadInsnFormat;
extern GameExceptionRef GameExceptionSegFault;
extern GameExceptionRef GameExceptionBadDirection;

static inline const char *GameExceptionString(GameExceptionRef exc) {
    return exc;
}


uint32_t GameRandom();
uint32_t GameRandomBelow(uint32_t i);


typedef enum {
    DirectionNorth,
    DirectionEast,
    DirectionSouth,
    DirectionWest
} Direction;
static const Direction DirectionMax = DirectionWest;

typedef enum {
    CellInsnRepEat,
    CellInsnRepGo,
    CellInsnRepStr
} CellInsnRep;

typedef struct Cell {
private:
    Cell *nearEnemy(Game &game, Race &race);
public:
    CellInsnRep rep    = CellInsnRepEat;
    int         repCnt = 0;
    
    int x;
    int y;
    
    long weight = 5;
    
    Direction direction = (Direction)(arc4random() % (DirectionMax + 1));
    const char *directionString();
    
    std::size_t pc = 0;
    
    void eat();
    void go(Game &game);
    void clon(Game &game, Race &race);
    void str(Game &game, Race &race);
    void left();
    void right();
    void back();
    void turn();
    void jg(int m, std::size_t addr);
    void jl(int m, std::size_t addr);
    void j(std::size_t addr);
    void je(Game &game, Race &race, std::size_t addr);
} Cell;


static const int RaceExtinctionDateNone = 0;

struct Race {
private:
    std::size_t nextCellIndex = 0;
public:
    UIColor color;
    const char *colorString();
    
    bool extinct = false;
    int  extinctionDate = RaceExtinctionDateNone;
    
    std::vector<std::vector<std::string>> insn;
    std::vector<std::string> fetchInsn(std::size_t &pc);
    
    std::vector<Cell> cells;
    Cell &nextCell();
    Cell &prevCell();
};


class Game {
private:
    UIDisplay *display;
    
    std::vector<Race> races;
    std::size_t nextRaceIndex = 0;
    Race &nextRace();
    
    void removeRaceWithColor(UIColor color);
    
    int logY = GameBoxHeight + 1;
    void logLine(std::string &string);
    
    void extinctionAlert(const char *colorString);
public:
    Game(UIDisplay *display);
    
    std::size_t raceCount() {return races.size();};
    void        addRace(Race &race);
    Race        &raceWithIndex(std::size_t index);
    Race        &raceWithColor(UIColor color);
    
    Cell *cellAt(int x, int y, UIColor *color = nullptr);
    
    void drawNewCell();
    
    void moveIfPossible(int &x, int &y, int dstX, int dstY);
    bool isVisitable(int x, int y);
    bool isLegal(int x, int y);
    bool isEmpty(int x, int y);
    void randomEmpty(int &x, int &y);
    
    void log(const char *msg);
    void log(const std::string &msg);
    
    void fatal(const char *msg) NORETURN;
    void fatal(const std::string &msg) NORETURN;
    
    Race &raceStep();
    void start();
};


#endif
