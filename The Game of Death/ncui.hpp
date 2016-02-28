#ifndef NCUI_HPP
#define NCUI_HPP


#include <string>
#include <thread>
#include <mutex>
#include <ncurses.h>


typedef chtype UIChar;

typedef enum {
    UIColorGreen = 1,
    UIColorRed,
    UIColorYellow,
    UIColorBlue
} UIColor;

typedef enum {
    UIColorAttrGreen  = COLOR_PAIR(UIColorGreen),
    UIColorAttrRed    = COLOR_PAIR(UIColorRed),
    UIColorAttrYellow = COLOR_PAIR(UIColorYellow),
    UIColorAttrBlue   = COLOR_PAIR(UIColorBlue)
} UIColorAttr;

static inline UIColorAttr UIAttrForColor(UIColor color) {
    return (UIColorAttr)COLOR_PAIR(color);
}


class UIDisplay {
private:
    int x;
    int y;
    
    int width;
    int height;
    
    int    size;
    chtype *screen;
    chtype *update;
    
    bool        autoFlush;
    std::thread autoFlushThread;
    void        flushLoop();
public:
    UIDisplay(int x, int y, int width, int height, bool autoFlush = true);
    ~UIDisplay();
    
    std::mutex updateMutex;
    
    int  getWidth();
    int  getHeight();
    
    void putChar(int x, int y, chtype ch);
    void putString(int x, int y, const char *string, chtype attr = 0);
    void putString(int x, int y, std::string &string, chtype attr = 0);
    
    void eraseLine(int y);
    void copyLine(int dstY, int srcY);
    
    void flush();
    void setAutoFlush(bool value);
};


void UIInit();
void UIQuit();
void UIBeep();
void UIFlash();
void UIAttention();


#endif
