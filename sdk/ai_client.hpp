#pragma once

#include <time.h>
#include <string>
#include <cstring>
#include <iostream>
#include "../Point.h"
#include "../Strategy.h"

typedef unsigned int ui;

class AI_Client
{
    private:
    Point* point;
    Point* Banned_point;
    Point* Last_move;
    int row, col;
    int* board; //chess board
    int* top; //available position

    void sendLen(size_t len) {
        unsigned char lenb[4];
        lenb[0] = (unsigned char)(len);
        lenb[1] = (unsigned char)(len >> 8);
        lenb[2] = (unsigned char)(len >> 16);
        lenb[3] = (unsigned char)(len >> 24);
        for (int i = 0; i < 4; i++)
            printf("%c", lenb[3 - i]);
    }
    void send()
    {
        char msg[6];
        size_t len = sprintf(msg, "%d %d", point->x, point->y);
        sendLen(len);
        std::cout << msg;
        std::cout.flush();
    }
    public:
    void run()
    {
        sendLen(2);
        std::cout << "ok";
        std::cout.flush();
        scanf("%d %d %d %d", &row, &col, &Banned_point->x, &Banned_point->y);
        board = new int[row * col];
        top = new int[col];
        while(true)
        {
            scanf("%d %d", &Last_move->x, &Last_move->y);
            int top_size, board_size;
            scanf("%d", &top_size);
            for(int i = 0; i < top_size; i ++) scanf("%d", &top[i]);
            scanf("%d", &board_size);
            for(int i = 0; i < board_size; i ++) scanf("%d", &board[i]);

            point = getPoint(row, col, top, board, Last_move->x, Last_move->y, Banned_point->x, Banned_point->y);
            if(point == nullptr)
            {
                point = new Point(-2, -2);
                send();
                clearPoint(point);
                break;
            }
            send();
            clearPoint(point);
        }
    }
    AI_Client()
    {
        point = nullptr;
        Banned_point = new Point(-1, -1);
        Last_move = new Point(-1, -1);
    }
    ~AI_Client()
    {
        if(board) delete[] board;
        if(top) delete[] top;
        if(point) clearPoint(point);
        if(Banned_point) delete Banned_point;
        if(Last_move) delete Last_move;
    }
};
