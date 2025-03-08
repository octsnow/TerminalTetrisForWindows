#include <stdio.h>
#include <conio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>

#define FIELD_WIDTH 10
#define FIELD_HEIGHT 20
#define MINO_NUM 7
#define MINO_SIZE 4
#define MIN_COLOR_CODE 31
#define FALL_INTERVAL CLOCKS_PER_SEC
#define LINE_ANIMATION_INTERVAL (CLOCKS_PER_SEC / 8)
#define LINE_ANIMATION_NUM 4
#define INITIAL_MINO_X 3
#define INITIAL_MINO_Y 0

#define BLOCK_TYPE_FLAG_LINE 0x10
#define BLOCK_TYPE_FLAG_GHOST 0x8
//#define BLOCK_TYPE_MASK_ANIMATION 0x30
//#define BLOCK_TYPE_MASK_COLOR 0x7
#define BLOCK_TYPE_GET_COLOR(x) (x & 0x7)
#define BLOCK_TYPE_GET_GHOST(x) (x & 0x8)
#define BLOCK_TYPE_GET_LINE(x) (x >> 4 & 0x1)
#define BLOCK_TYPE_NONE 0x0

#define STATE_FLAG_SHOW_LINE 0x1
#define STATE_FLAG_ANIMATE_LINE 0x2
#define STATE_MASK_ANIMATE_LINE_COUNT 0xC
#define STATE_GET_SHOW_LINE(x) (x & 0x1)
#define STATE_GET_ANIMATE_LINE(x) (x >> 1 & 0x1)
#define STATE_GET_ANIMATE_LINE_COUNT(x) (x >> 2 & 0x3)
#define STATE_SET_ANIMATE_LINE_COUNT(x, y) ((x & ~STATE_MASK_ANIMATE_LINE_COUNT) | (y & 0x3) << 2)

#define COLOR_RED       (FOREGROUND_INTENSITY | FOREGROUND_RED)
#define COLOR_GREEN     (FOREGROUND_INTENSITY | FOREGROUND_GREEN)
#define COLOR_BLUE      (FOREGROUND_INTENSITY | FOREGROUND_BLUE)
#define COLOR_YELLOW    (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN)
#define COLOR_MAGENTA   (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE)
#define COLOR_CYAN      (FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define COLOR_GRAY      (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define COLOR_WHITE     (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

typedef int BOOL;
typedef unsigned char BLOCK_TYPE;
typedef unsigned char STATE;

WORD color_list[7] = {
    COLOR_CYAN,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_RED,
    COLOR_BLUE,
    COLOR_GRAY,
    COLOR_MAGENTA,
};

int minos[MINO_NUM][MINO_SIZE * MINO_SIZE] = {
    // I
    { 0, 0, 0, 0,
      1, 1, 1, 1,
      0, 0, 0, 0,
      0, 0, 0, 0, },

    // O
    { 0, 0, 0, 0,
      0, 1, 1, 0,
      0, 1, 1, 0,
      0, 0, 0, 0, },

    // S
    { 0, 0, 0, 0,
      0, 1, 1, 0,
      1, 1, 0, 0,
      0, 0, 0, 0, },

    // Z
    { 0, 0, 0, 0,
      1, 1, 0, 0,
      0, 1, 1, 0,
      0, 0, 0, 0, },

    // J
    { 0, 0, 0, 0,
      1, 1, 1, 0,
      0, 0, 1, 0,
      0, 0, 0, 0, },

    // L
    { 0, 0, 0, 0,
      1, 1, 1, 0,
      1, 0, 0, 0,
      0, 0, 0, 0, },

    // T
    { 0, 0, 0, 0,
      1, 1, 1, 0,
      0, 1, 0, 0,
      0, 0, 0, 0, },
};

void HideCursor() {
    HANDLE hStdOut = NULL;
    CONSOLE_CURSOR_INFO curInfo;

    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleCursorInfo(hStdOut, &curInfo);
    curInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hStdOut, &curInfo);
}

void DisplayCursor() {
    HANDLE hStdOut = NULL;
    CONSOLE_CURSOR_INFO curInfo;

    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleCursorInfo(hStdOut, &curInfo);
    curInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hStdOut, &curInfo);
}

void MoveCursor(HANDLE console_handle, int x, int y, BOOL is_absolute_x, BOOL is_absolute_y) {
    CONSOLE_SCREEN_BUFFER_INFO ci;
    COORD cursor_pos;

    GetConsoleScreenBufferInfo(console_handle, &ci);
    cursor_pos = ci.dwCursorPosition;

    if(is_absolute_x) {
        cursor_pos.X = x;
    } else {
        cursor_pos.X += x;
    }

    if(is_absolute_y) {
        cursor_pos.Y = y;
    } else {
        cursor_pos.Y += y;
    }

    SetConsoleCursorPosition(console_handle, cursor_pos);
}

void Init(BLOCK_TYPE *field) {
    for(int y = 0; y < FIELD_HEIGHT; y++) {
        for(int x = 0; x < FIELD_WIDTH; x++) {
            field[y * FIELD_WIDTH + x] = BLOCK_TYPE_NONE;
        }
    }

    for(int x = 0; x < FIELD_WIDTH + 2; x++) {
        printf("==");
    }
    printf("\n");
    for(int y = 0; y < FIELD_HEIGHT; y++) {
        printf("||");
        for(int x = 0; x < FIELD_WIDTH; x++) {
            printf("  ");
        }
        printf("||\n");
    }
    for(int x = 0; x < FIELD_WIDTH + 2; x++) {
        printf("==");
    }

    HideCursor();
    srand(time(NULL));
}

void Term(HANDLE console_handle, CONSOLE_SCREEN_BUFFER_INFO console_info) {
    SetConsoleTextAttribute(console_handle, console_info.wAttributes);
    DisplayCursor();
}

void ClearScreen(HANDLE console_handle) {
    MoveCursor(console_handle, 2, -FIELD_HEIGHT, TRUE, FALSE);
}

void Draw(HANDLE console_handle, STATE state, BLOCK_TYPE *field) {
    for(int y = 0; y < FIELD_HEIGHT; y++) {
        for(int x = 0; x < FIELD_WIDTH; x++) {
            BLOCK_TYPE block_type = field[y * FIELD_WIDTH + x];
            BLOCK_TYPE block_color = BLOCK_TYPE_GET_COLOR(block_type);
            if(block_color == BLOCK_TYPE_NONE) {
                printf("  ");
            } else if(BLOCK_TYPE_GET_LINE(block_type)) {
                SetConsoleTextAttribute(console_handle, COLOR_WHITE);
                printf(STATE_GET_SHOW_LINE(state) ? "++" : "  ");
            } else {
                SetConsoleTextAttribute(console_handle, color_list[block_color - 1]);
                printf(BLOCK_TYPE_GET_GHOST(block_type) ? "XX" : "[]");
            }
        }
        MoveCursor(console_handle, 2, 1, TRUE, FALSE);
    }
}

BOOL CheckValidMino(BLOCK_TYPE *field, int *mino_buffer, int x, int y) {
    for(int _y = 0; _y < MINO_SIZE; _y++) {
        for(int _x = 0; _x < MINO_SIZE; _x++) {
            if(mino_buffer[_y * MINO_SIZE + _x]) {
                int block_x = x + _x;
                int block_y = y + _y;
                BLOCK_TYPE block_type = field[block_y * FIELD_WIDTH + block_x];

                if(block_x < 0 || block_x >= FIELD_WIDTH) {
                    return FALSE;
                }
                if(block_y < 0 || block_y >= FIELD_HEIGHT) {
                    return FALSE;
                }
                if(BLOCK_TYPE_GET_GHOST(block_type) == 0x0
                && BLOCK_TYPE_GET_COLOR(block_type) != BLOCK_TYPE_NONE) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

void SetMino(
        BLOCK_TYPE *field,
        int *mino_buffer,
        int mino_type,
        int x, int y) {

    for(int _y = 0; _y < MINO_SIZE; _y++) {
        for(int _x = 0; _x < MINO_SIZE; _x++) {
            if(mino_buffer[_y * MINO_SIZE + _x]) {
                field[(y + _y) * FIELD_WIDTH + (x + _x)] = mino_type + 1;
            }
        }
    }
}

BOOL Move(
    BLOCK_TYPE *field,
    int *mino_buffer,
    int *x, int *y,
    int dx, int dy) {

    int new_x = *x + dx;
    int new_y = *y + dy;

    if(CheckValidMino(field, mino_buffer, new_x, new_y)) {
        *x = new_x;
        *y = new_y;

        return TRUE;
    }

    return FALSE;
}


void HardDrop(
        BLOCK_TYPE *field,
        int *mino_buffer,
        int *x, int *y) {

        while(Move(field, mino_buffer, x, y, 0, 1));
}

BOOL Clockwise(
        BLOCK_TYPE *field,
        int *mino_buffer,
        int mino_type,
        int x, int y) {

    if(mino_type == 1) return TRUE;
    int new_mino_buffer[MINO_SIZE * MINO_SIZE];

    // initialization
    for(int i = 0; i < MINO_SIZE * MINO_SIZE; i++) {
        new_mino_buffer[i] = 0;
    }

    // rotation
    if(mino_type == 0) {
        for(int y = 0; y < MINO_SIZE; y++) {
            for(int x = 0; x < MINO_SIZE; x++) {
                new_mino_buffer[y * MINO_SIZE + x] = mino_buffer[(MINO_SIZE - x - 1) * MINO_SIZE + y];
            }
        }
    } else {
        for(int y = 0; y < MINO_SIZE - 1; y++) {
            for(int x = 0; x < MINO_SIZE - 1; x++) {
                new_mino_buffer[y * MINO_SIZE + x] = mino_buffer[(MINO_SIZE - x - 2) * MINO_SIZE + y];
            }
        }
    }

    if(CheckValidMino(field, new_mino_buffer, x, y)) {
        memcpy_s(
                mino_buffer,
                sizeof(int) * MINO_SIZE * MINO_SIZE,
                new_mino_buffer,
                sizeof(int) * MINO_SIZE * MINO_SIZE);

        return TRUE;
    }

    return FALSE;
}

BOOL CounterClockwise(
        BLOCK_TYPE *field,
        int *mino_buffer,
        int mino_type,
        int x, int y) {

    if(mino_type == 1) return TRUE;
    int new_mino_buffer[MINO_SIZE * MINO_SIZE];

    // initialization
    for(int i = 0; i < MINO_SIZE * MINO_SIZE; i++) {
        new_mino_buffer[i] = 0;
    }

    // rotation
    if(mino_type == 0) {
        for(int y = 0; y < MINO_SIZE; y++) {
            for(int x = 0; x < MINO_SIZE; x++) {
                new_mino_buffer[y * MINO_SIZE + x] = mino_buffer[x * MINO_SIZE + (MINO_SIZE - y - 1)];
            }
        }
    } else {
        for(int y = 0; y < MINO_SIZE - 1; y++) {
            for(int x = 0; x < MINO_SIZE - 1; x++) {
                new_mino_buffer[y * MINO_SIZE + x] = mino_buffer[x * MINO_SIZE + (MINO_SIZE - y - 2)];
            }
        }
    }

    if(CheckValidMino(field, new_mino_buffer, x, y)) {
        memcpy_s(
                mino_buffer,
                sizeof(int) * MINO_SIZE * MINO_SIZE,
                new_mino_buffer,
                sizeof(int) * MINO_SIZE * MINO_SIZE);

        return TRUE;
    }

    return FALSE;
}

void SetCurrentMino(
    BLOCK_TYPE *field,
    int *mino_buffer,
    int mino_type,
    int x, int y) {

    int gx = x, gy = y;
    HardDrop(field, mino_buffer, &gx, &gy);
    for(int _y = 0; _y < MINO_SIZE; _y++) {
        for(int _x = 0; _x < MINO_SIZE; _x++) {
            if(mino_buffer[_y * MINO_SIZE + _x]) {
                field[(gy + _y) * FIELD_WIDTH + (gx + _x)] = BLOCK_TYPE_FLAG_GHOST | (mino_type + 1);
                field[(y + _y) * FIELD_WIDTH + (x + _x)] = mino_type + 1;
            }
        }
    }
}

void ResetCurrentMino(
    BLOCK_TYPE *field,
    int *mino_buffer,
    int mino_type,
    int x, int y) {

    for(int _y = 0; _y < MINO_SIZE; _y++) {
        for(int _x = 0; _x < MINO_SIZE; _x++) {
            if(mino_buffer[_y * MINO_SIZE + _x]) {
                field[(y + _y) * FIELD_WIDTH + (x + _x)] = BLOCK_TYPE_NONE;
            }
        }
    }

    int gx = x, gy = y;
    HardDrop(field, mino_buffer, &gx, &gy);
    for(int _y = 0; _y < MINO_SIZE; _y++) {
        for(int _x = 0; _x < MINO_SIZE; _x++) {
            if(mino_buffer[_y * MINO_SIZE + _x]) {
                field[(gy + _y) * FIELD_WIDTH + (gx + _x)] = BLOCK_TYPE_NONE;
            }
        }
    }

}

int NextMinoType(uint8_t *bag) {
    if((*bag & 0x7F) == 0x7F) {
        *bag = 0x0;
    }
    int new_mino_type;

    while(1) {
        uint8_t flag;
        new_mino_type = rand() % 7;
        flag = 1 << new_mino_type;

        if(*bag ^ (*bag | flag)) {
            *bag |= flag;
            break;
        }
    }

    return new_mino_type;
}

// if full line exists then return true, otherwise return false
BOOL CheckFullLine(BLOCK_TYPE *field) {
    BOOL result = FALSE;
    for(int y = FIELD_HEIGHT - 1; y >= 0; y--) {
        BOOL is_full = TRUE;
        for(int x = 0; x < FIELD_WIDTH ; x++) {
            if(field[y * FIELD_WIDTH + x] == BLOCK_TYPE_NONE) {
                is_full = FALSE;
                break;
            }
        }

        if(is_full) {
            for(int x = 0; x < FIELD_WIDTH ; x++) {
                field[y * FIELD_WIDTH + x] |= BLOCK_TYPE_FLAG_LINE;
            }
            result = TRUE;
        }
    }

    return result;
}


void RemoveFullLine(BLOCK_TYPE *field) {
    for(int y = FIELD_HEIGHT - 1; y >= 0; y--) {
        if(BLOCK_TYPE_GET_LINE(field[y * FIELD_WIDTH])) {
            for(int _y = y; _y > 0; _y--) {
                for(int x = 0; x < FIELD_WIDTH; x++) {
                    field[_y * FIELD_WIDTH + x] = field[(_y - 1) * FIELD_WIDTH + x];
                }
            }
            /*
            for(int x = 0; x < FIELD_WIDTH; x++) {
                field[x] = BLOCK_TYPE_NONE;
            }
            */
            y++;
        }
    }
}

void SetNextMino(int *mino_buffer, uint8_t *bag, int *mino_type, int *x, int *y) {
    *mino_type = NextMinoType(bag);

    memcpy_s(
            mino_buffer,
            sizeof(int) * MINO_SIZE * MINO_SIZE,
            minos[*mino_type],
            sizeof(int) * MINO_SIZE * MINO_SIZE);

    *x = INITIAL_MINO_X;
    *y = INITIAL_MINO_Y;
}

int main() {
    HANDLE console_handle;
    CONSOLE_SCREEN_BUFFER_INFO saved_ci;
    BOOL is_end = FALSE;
    BLOCK_TYPE field[FIELD_WIDTH * FIELD_HEIGHT];
    STATE state = 0x0;
    int mino_buffer[MINO_SIZE * MINO_SIZE];
    uint8_t bag = 0x0;
    int current_mino_type;
    int current_x;
    int current_y;
    clock_t last_time = 0;
    clock_t time_count = 0;

    console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(console_handle, &saved_ci);
    Init(field);

    SetNextMino(
            mino_buffer,
            &bag,
            &current_mino_type,
            &current_x, &current_y);

    last_time = clock();
    while(!is_end) {
        ClearScreen(console_handle);

        if(STATE_GET_ANIMATE_LINE(state)) {
            Draw(console_handle, state, field);
            if(time_count >= LINE_ANIMATION_INTERVAL) {
                unsigned char animate_count;

                time_count -= LINE_ANIMATION_INTERVAL;
                state ^= STATE_FLAG_SHOW_LINE;

                animate_count = STATE_GET_ANIMATE_LINE_COUNT(state);
                animate_count++;

                if(animate_count < LINE_ANIMATION_NUM) {
                    state = STATE_SET_ANIMATE_LINE_COUNT(state, animate_count);
                } else {
                    state &= ~STATE_FLAG_ANIMATE_LINE;
                    RemoveFullLine(field);
                    SetNextMino(
                            mino_buffer,
                            &bag,
                            &current_mino_type,
                            &current_x, &current_y);
                    if(!CheckValidMino(field, mino_buffer, current_x, current_y)) {
                        is_end = TRUE;
                    }
                    time_count = 0;
                    last_time = clock();
                }
            }
        } else {
            SetCurrentMino(field, mino_buffer, current_mino_type, current_x, current_y);
            Draw(console_handle, state, field);
            ResetCurrentMino(field, mino_buffer, current_mino_type, current_x, current_y);

            if(time_count >= FALL_INTERVAL) {
                time_count -= FALL_INTERVAL;
                if(!Move(field, mino_buffer, &current_x, &current_y, 0, 1)) {
                    SetMino(field, mino_buffer, current_mino_type, current_x, current_y);
                    if(CheckFullLine(field)) {
                        state |= STATE_FLAG_ANIMATE_LINE;
                        state |= STATE_FLAG_SHOW_LINE;
                        state = STATE_SET_ANIMATE_LINE_COUNT(state, 0x0);
                    } else {
                        SetNextMino(
                                mino_buffer,
                                &bag,
                                &current_mino_type,
                                &current_x, &current_y);
                        if(!CheckValidMino(field, mino_buffer, current_x, current_y)) {
                            is_end = TRUE;
                        }
                    }
                    time_count = 0;
                    last_time = clock();
                }
            }
        }
        clock_t current_time = clock();
        time_count += current_time - last_time;
        last_time = current_time;

        if(_kbhit()) {
            switch(_getch()) {
            case 0x1b: // ESC
                is_end = TRUE;
                break;

            case 'a': // left
                Move(field, mino_buffer, &current_x, &current_y, -1, 0);
                break;

            case 'd': // right
                Move(field, mino_buffer, &current_x, &current_y, 1, 0);
                break;

            case 's': // down
                Move(field, mino_buffer, &current_x, &current_y, 0, 1);
                break;

            case 'w': // hard drop
                HardDrop(field, mino_buffer, &current_x, &current_y);
                time_count = FALL_INTERVAL;
                break;

            case 'j': // clockwise
                Clockwise(field, mino_buffer, current_mino_type, current_x, current_y);
                break;

            case 'k': // counterclockwise
                CounterClockwise(field, mino_buffer, current_mino_type, current_x, current_y);
                break;
            }
        }
    }

    Term(console_handle, saved_ci);

    return 0;
}
