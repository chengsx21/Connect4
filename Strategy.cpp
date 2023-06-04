#include <iostream>
#include <unistd.h>

#include <cmath>
#include <cfloat>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "Judge.h"
#include "Point.h"
#include "Strategy.h"
using namespace std;

#define PLAYER_HE 1
#define PLAYER_ME 2
#define PLAYER_SUM 3
#define CONNECT_DANGER 3
#define EPSILON 1e-5
#define MAX_NODES 1000000
#define MAX_TIME 1.8
#define C 1

double startTime = 0;
int no_X;
int no_Y;
int Glob_M;
int Glob_N;
int Glob_Sum;

/*
    复制 top 数组
*/
void copyTop(int* toTop, int* fromTop) {
    for (int i = 0; i < Glob_N; ++i)
        toTop[i] = fromTop[i];
}

/*
    复制 board 数组
*/
void copyBoard(int** toBoard, int** fromBoard) {
    for (int i = 0; i < Glob_M; ++i) {
        toBoard[i] = new int[Glob_N];
        for (int j = 0; j < Glob_N; ++j)
            toBoard[i][j] = fromBoard[i][j];
    }
}

/*
    UCT 节点类
*/
class UCTNode {
public:
    UCTNode* parent = NULL;         // 父亲节点
    UCTNode** child;                // 孩子节点
    double win = 0.0;               // 节点胜率
    int visit = 0;                  // 节点访问次数
    int feasible_counts = 0;        // 可扩展列数
    int x = -1;                     // 最近落子坐标
    int y = -1;                     // 最近落子坐标
    int player = PLAYER_HE;         // 最近落子玩家
    int* feasible = NULL;           // 可扩展列
    int* tmp_top;                   // 列顶坐标
    int** tmp_board;                // 棋盘坐标

    /*
        构造函数
    */
    UCTNode(int** board, int* top, int x = -1, int y = -1, int player = PLAYER_HE, UCTNode* parent = NULL) {
        this->x = x;
        this->y = y;
        this->player = player;
        this->parent = parent;
        this->tmp_top = new int[Glob_N];
        this->tmp_board = new int* [Glob_M];
        copyTop(this->tmp_top, top);
        copyBoard(this->tmp_board, board);
        child = new UCTNode* [Glob_N];
        for (int i = 0; i < Glob_N; ++i)
            child[i] = NULL;
        feasible = new int[Glob_N];
        for (int i = 0; i < Glob_N; ++i)
            if (top[i])
                feasible[feasible_counts++] = i;
    }

    /*
        检查当前节点下一步是否有一步获胜的走法
    */
    int cornerCheck(int player) {
        int y = 0;
        for (y = 0; y < Glob_N; ++y) {
            if (tmp_top[y] > 0) {
                tmp_board[tmp_top[y] - 1][y] = PLAYER_SUM - player;
                if (isWin(player, tmp_top[y] - 1, y, Glob_M, Glob_N, tmp_board)) {
                    tmp_board[tmp_top[y] - 1][y] = 0;
                    return y;
                }
                tmp_board[tmp_top[y] - 1][y] = 0;
            }
        }
        for (y = 0; y < Glob_N; ++y) {
            if (tmp_top[y] > 0) {
                tmp_board[tmp_top[y] - 1][y] = player;
                if (isWin(PLAYER_SUM - player, tmp_top[y] - 1, y, Glob_M, Glob_N, tmp_board)) {
                    tmp_board[tmp_top[y] - 1][y] = 0;
                    return y;
                }
                tmp_board[tmp_top[y] - 1][y] = 0;
            }
        }
        return -1;
    }

    /*
        检查对方是否有一步获胜的走法
    */
    bool isWin(int player, const int x, const int y, const int M, const int N, int *const *board) {
        if (player == PLAYER_ME)
            return userWin(x, y, M, N, board);
        else 
            return machineWin(x, y, M, N, board);
    }

    /*
        析构函数 - 这里必须释放所有的子节点, 防止 MLE
    */
    ~UCTNode() {
        delete[] tmp_top;
        delete[] feasible;
        for (int i = 0; i < Glob_M; ++i)
            delete[] tmp_board[i];
        delete[] tmp_board;
        for (int i = 0; i < Glob_N; ++i)
            if (child[i])
                delete child[i];
        delete[] child;
    }
};

/*
    UCT 类
*/
class UCT {
public:
    UCTNode* root;

    /*
        构造函数
    */
    UCT(int** board, const int* top) {
        int* _top = new int[Glob_N];
        for (int i = 0; i < Glob_N; ++i)
            _top[i] = top[i];
        root = new UCTNode(board, _top);
        delete[] _top;
    }

    /*
        判断游戏是否结束
    */
    bool isGameOver(UCTNode* node) {
        bool gameTie = isTie(Glob_N, node->tmp_top);
        bool gameWin = (node->player == 2) && machineWin(node->x, node->y, Glob_M, Glob_N, node->tmp_board);
        bool gameLose = (node->player == 1) && userWin(node->x, node->y, Glob_M, Glob_N, node->tmp_board);
        return gameTie || gameWin || gameLose;
    }

    /*
        选择当前节点默认的下一步走法
    */
    UCTNode* defaultSolution() {
        double value = -FLT_MAX;
        UCTNode* default_solution = NULL;
        for (int i = 0; i < Glob_N; ++i) {
            if (root->child[i]) {
                double val = (3 - 2 * root->player) * double(root->child[i]->win) / root->child[i]->visit;
                if (val > value) {
                    default_solution = root->child[i];
                    value = val;
                }
            }
        }
        return default_solution;
    }

    /*
        选择当前节点最优的子扩展节点
    */
    UCTNode* bestSolution(UCTNode* node) {
        double value = -FLT_MAX;
        UCTNode* best_solution = NULL;
        for (int i = 0; i < Glob_N; ++i) {
            if (node->child[i]) {
                double logN = C * sqrt(2 * log(double(node->visit)) / node->child[i]->visit);
                double val = (3 - 2 * node->player) * double(node->child[i]->win) / node->child[i]->visit + logN;
                if (val > value) {
                    best_solution = node->child[i];
                    value = val;
                }
            }
        }
        return best_solution;
    }

    /*
        回溯更新节点收益信息
    */
    void backUp(UCTNode* node, double gain) {
        while (node) {
            node->visit++;
            node->win += gain;
            gain = (gain > 1) ? (gain - 1) : ( (gain < -1) ? (gain + 1) : gain);
            node = node->parent;
        }
    }

    /*
        检查是否有一步获胜的走法
    */
    UCTNode* dangerCornerCheck() {
        int count = 0;
        int y = root->cornerCheck(root->player);
        if (y != -1) {
            root->tmp_top[y]--;
            int x = root->tmp_top[y];
            root->tmp_board[x][y] = root->player;
            // 如果落子点上方是不可落子点, 则再次减一
            // ! 因为这一步没有注意导致胜率低了好多
            if (no_X == x - 1 && no_Y == y)
                root->tmp_top[y]--;
            root->child[y] = new UCTNode(root->tmp_board, root->tmp_top, x, y, PLAYER_SUM - root->player, root);
            return root->child[y];
        }
        return NULL;
    }

    /*
        扩展一个节点
    */
    UCTNode* expand(UCTNode* node) {
        int* top = new int[Glob_N];
        int** board = new int* [Glob_M];
        copyTop(top, node->tmp_top);
        copyBoard(board, node->tmp_board);

        int index = rand() % (node->feasible_counts);
        int y = node->feasible[index];
        top[y]--;
        int x = top[y];
        board[x][y] = PLAYER_SUM - node->player;
        // 如果落子点上方是不可落子点, 则再次减一
        // ! 因为这一步没有注意导致胜率低了好多
        if (x - 1 == no_X && y == no_Y)
            top[y]--;
        node->child[y] = new UCTNode(board, top, x, y, PLAYER_SUM - node->player, node);
        node->feasible_counts--;
        int swap_item = node->feasible[index];
        node->feasible[index] = node->feasible[node->feasible_counts];
        node->feasible[node->feasible_counts] = swap_item;
        delete[] top;
        for (int i = 0; i < Glob_M; ++i)
            delete[] board[i];
        delete[] board;
        return node->child[y];
    }

    /*
        获得收益值
    */
    int getGain(int count, int player, int x, int y, int** board, int* top) {
        if ((player == 2) && machineWin(x, y, Glob_M, Glob_N, board))
            return (count <= 5) ? 6 - count : 1;
        else if ((player == 1) && userWin(x, y, Glob_M, Glob_N, board))
            return (count <= 5) ? count - 6 : -1;
        else if (isTie(Glob_N, top))
            return 0;
        else
            return -6;
    }

    /*
        选择下一个节点进行扩展
    */
    UCTNode* treePolicy(UCTNode* node) {
        bool gameOver = false;
        while (true) {
            if (!node->parent)
                gameOver = false;
            else if (isGameOver(node) || gameOver)
                break;
            if (node->feasible_counts > 0)
                return expand(node);
            else
                node = bestSolution(node);
        }
        return node;
    }

    /*
        模拟一次游戏, 返回结果收益值
    */
    double defaultPolicy(UCTNode* node) {
        int* top = new int[Glob_N];
        int** board = new int* [Glob_M];
        copyTop(top, node->tmp_top);
        copyBoard(board, node->tmp_board);
        int player = node->player;
        int x = node->x;
        int y = node->y;
        int count = 0;
        double gain = getGain(++count, player, x, y, board, top);
        while (fabs(gain + 6) < EPSILON) {
            player = PLAYER_SUM - player, y = 0;
            while (true) {
                int index = rand() % Glob_Sum, index_sum = 0;
                for (int i = 0; i < Glob_N; ++i) {
                    index_sum += (i <= (Glob_N - 1) / 2) ? (i + 1) : Glob_N - i;
                    if (index_sum > index) {
                        y = i;
                        break;
                    }
                }
                if (top[y] != 0)
                    break;
            }
            top[y]--, x = top[y];
            board[x][y] = player;
            // 如果落子点上方是不可落子点, 则再次减一
            // ! 因为这一步没有注意导致胜率低了好多
            if (x - 1 == no_X && y == no_Y)
                top[y]--;
            gain = getGain(++count, player, x, y, board, top);
        }
        delete[] top;
        for (int i = 0; i < Glob_M; ++i)
            delete[] board[i];
        delete[] board;
        return gain;
    }

    /*
        析构函数 - 释放树根, 才能释放所有子节点, 防止 MLE
    */
    ~UCT() {
        delete this->root;
    }
};


/*
	策略函数接口, 该函数被对抗平台调用, 每次传入当前状态, 要求输出你的落子点, 该落子点必须是一个符合游戏规则的落子点, 不然对抗平台会直接认为你的程序有误
	
	input:
		为了防止对对抗平台维护的数据造成更改, 所有传入的参数均为 const 属性
		M, N : 棋盘大小 M - 行数 N - 列数 均从 0 开始计,  左上角为坐标原点, 行用 x 标记, 列用 y 标记
		top : 当前棋盘每一列列顶的实际位置. e.g. 第 i 列为空,则 _top[i] == M, 第 i 列已满,则 _top[i] == 0
		_board : 棋盘的一维数组表示, 为了方便使用, 在该函数刚开始处, 我们已经将其转化为了二维数组 board
				你只需直接使用 board 即可, 左上角为坐标原点, 数组从 [0][0] 开始计 (不是 [1][1])
				board[x][y] 表示第 x 行、第 y 列的点(从 0 开始计)
				board[x][y] == 0 / 1 / 2 分别对应 (x, y) 处 无落子 / 有用户的子 / 有程序的子, 不可落子点处的值也为 0
		lastX, lastY : 对方上一次落子的位置, 你可能不需要该参数, 也可能需要的不仅仅是对方一步的
				落子位置, 这时你可以在自己的程序中记录对方连续多步的落子位置, 这完全取决于你自己的策略
		noX, noY : 棋盘上的不可落子点 (注: 我们传入的 top 已经替你处理了不可落子点, 也就是说如果某一步
				所落的子的上面恰是不可落子点, 那么 UI 工程中的代码就已经将该列的 top 值又进行了一次减一操作, 
				所以在你的代码中也可以根本不使用 noX 和 noY 这两个参数, 完全认为 top 数组就是当前每列的顶部即可,
				当然如果你想使用 lastX, lastY 参数, 有可能就要同时考虑 noX 和 noY 了)
		以上参数实际上包含了当前状态 (M N _top _board) 以及历史信息 (lastX lastY), 你要做的就是在这些信息下给出尽可能明智的落子点
	output:
		你的落子点 Point
*/
extern "C" Point* getPoint(const int M, const int N, const int* top, const int* _board, 
        const int lastX, const int lastY, const int noX, const int noY) {
	startTime = clock() / CLOCKS_PER_SEC;
    no_X = noX;
    no_Y = noY;
    Glob_M = M;
    Glob_N = N;
    Glob_Sum = (Glob_N % 2) ? (Glob_N / 2 + 1) * (Glob_N / 2 + 1) : (Glob_N / 2  + 1) * (Glob_N / 2);

	int x = -1, y = -1; // 最终将你的落子点存到 x, y 中
	int** board = new int* [M];
	for (int i = 0; i < M; i++) {
		board[i] = new int[N];
		for (int j = 0; j < N; j++)
			board[i][j] = _board[i * N + j];
	}

	/*
		根据你自己的策略来返回落子点, 也就是根据你的策略完成对 x, y 的赋值
	*/

	UCT* UCTtree = new UCT(board, top);
    UCTNode* node = UCTtree->dangerCornerCheck();
    // 如果没有危险点, 进行 UCT 搜索
    if (node == NULL) {
        int count = 0;
        // 限制搜索时间和搜索节点数
        while ((count++ < MAX_NODES) && (clock() / CLOCKS_PER_SEC - startTime < MAX_TIME)) {
            node = UCTtree->treePolicy(UCTtree->root);
            double gain = UCTtree->defaultPolicy(node);
            UCTtree->backUp(node, gain);
        }
        node = UCTtree->defaultSolution();
    }
    y = node->y;
    x = node->x;
    delete UCTtree;

	clearArray(M, N, board);
	return new Point(x, y);
}

/*
	getPoint 函数返回的 Point 指针是在本 so 模块中声明的, 为避免产生堆错误. 应在外部调用本 so 中的
	函数来释放空间, 而不应该在外部直接 delete
*/
extern "C" void clearPoint(Point* p) {
	delete p;
	return;
}

/*
	清除 top 和 board 数组
*/
void clearArray(int M, int N, int** board) {
	for (int i = 0; i < M; i++)
		delete[] board[i];
	delete[] board;
}
