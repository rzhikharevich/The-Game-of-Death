#include "ncui.hpp"
#include <cstring>

using std::memcpy;


static std::mutex ncursesMutex;


UIDisplay::UIDisplay(int x, int y, int width, int height, bool autoFlush) {
    this->x = x;
    this->y = y;
    
    if (width < 0)
        width = getmaxx(stdscr) - x;
    if (height < 0)
        height = getmaxy(stdscr) - y;
    
    this->width  = width;
    this->height = height;
    
    size   = width * height;
    screen = new chtype[size];
    update = new chtype[size];
    
    memset(screen, 0, size * sizeof(chtype));
    memset(update, 0, size * sizeof(chtype));
    
    this->autoFlush = !autoFlush;
    setAutoFlush(autoFlush);
}

UIDisplay::~UIDisplay() {
    setAutoFlush(false);
    autoFlushThread.join();
}

int UIDisplay::getWidth() {
    return width;
}

int UIDisplay::getHeight() {
    return height;
}

void UIDisplay::putChar(int x, int y, chtype ch) {
    if (x >= width || y >= height)
        throw "UIDisplay: out of bounds!";
    
    update[y * width + x] = ch;
}

void UIDisplay::putString(int x, int y, const char *string, chtype attr) {
    updateMutex.lock();
    
    int baseX = x;
    while (*string) {
        if (*string == '\n') {
            x = baseX;
            y++;
        } else {
            putChar(x, y, *string | attr);
            x++;
        }
        
        string++;
    }
    
    updateMutex.unlock();
}

void UIDisplay::putString(int x, int y, std::string &string, chtype attr) {
    putString(x, y, string.c_str(), attr);
}

void UIDisplay::eraseLine(int y) {
    updateMutex.lock();
    
    for (int x = 0; x < width; x++)
        update[y * width + x] = ' ';
    
    updateMutex.unlock();
}

void UIDisplay::copyLine(int dstY, int srcY) {
    updateMutex.lock();
    
    std::memcpy(update + dstY * width, update + srcY * width, sizeof(chtype) * width);
    
    updateMutex.unlock();
}

void UIDisplay::flush() {
    ncursesMutex.lock();
    updateMutex.lock();
    
    chtype *s = screen;
    chtype *u = update;
    for (int y1 = 0; y1 < height; y1++) {
        for (int x1 = 0; x1 < width; x1++, s++, u++) {
            if (*s != *u) {
                mvaddch(y + y1, x + x1, *u);
                *s = *u;
            }
        }
    }
    
    ncursesMutex.unlock();
    updateMutex.unlock();
    
    refresh();
}

void UIDisplay::setAutoFlush(bool value) {
    if (value && value != autoFlush)
        autoFlushThread = std::thread(&UIDisplay::flushLoop, this);
    
    autoFlush = value;
}

void UIDisplay::flushLoop() {
    while (autoFlush) {
        flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void UIInit() {
    initscr();

    nodelay(stdscr, TRUE);
    noecho();
    cbreak();
    curs_set(FALSE);
    clear();
    
    start_color();
    init_pair(UIColorGreen,  COLOR_GREEN,  COLOR_BLACK);
    init_pair(UIColorRed,    COLOR_RED,    COLOR_BLACK);
    init_pair(UIColorYellow, COLOR_YELLOW, COLOR_BLACK);
    init_pair(UIColorBlue,   COLOR_BLUE,   COLOR_BLACK);
}

void UIQuit() {
    endwin();
}

void UIBeep() {
    beep();
}

void UIFlash() {
    flash();
}

void UIAttention() {
    UIBeep();
    UIFlash();
}
