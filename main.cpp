#include <cstring>
#include <cstdio>
#include "Point.h"
#include "Strategy.h"

void send(const char *msg) {
    size_t len = strlen(msg);
    printf("%c%c%c%c%s", (unsigned char)(len >> 24), (unsigned char)(len >> 16), (unsigned char)(len >> 8), (unsigned char)(len), msg);
    fflush(stdout);
}

int main()
{
    int noX, noY;
    int lastX, lastY;
    int row, col;
    int* board; //chess board
    int* top; //available position
    send("ok"); // DO NOT REMOVE THIS LINE!

    scanf("%d %d %d %d", &row, &col, &noX, &noY);
    board = new int[row * col];
    top = new int[col];
    while(true)
    {
        scanf("%d %d", &lastX, &lastY);
        int top_size, board_size;
        scanf("%d", &top_size);
        for(int i = 0; i < top_size; i ++) scanf("%d", &top[i]);
        scanf("%d", &board_size);
        for(int i = 0; i < board_size; i ++) scanf("%d", &board[i]);

        Point* point = getPoint(row, col, top, board, lastX, lastY, noX, noY);
        if(point == nullptr)
        {
            send("-2 -2");
            break;
        }
        char msg[6];
        sprintf(msg, "%d %d", point->x, point->y);
        send(msg);
        clearPoint(point);
    }
    return 0;
}