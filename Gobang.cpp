#include <vector>
#include "Define.h"
#include "Square.h"
#include "ClientSocket.h"
#include "Gobang.h"
#include <windows.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <cmath>
using namespace std;

#define random(x) (rand()%x)
#define ROWS 15
#define COLS 15
#define ROUNDS 2

Square board[ROWS][COLS];
int ownColor = -1, oppositeColor = -1;
int cl = -1, rl = -1;
char lastmsg[16] = { "" };

void authorize(const char *id, const char *pass) {
	connectServer();
	std::cout << "Authorize " << id << std::endl;
	char msgBuf[BUFSIZE];
	memset(msgBuf, 0, BUFSIZE);
	msgBuf[0] = 'A';
	memcpy(&msgBuf[1], id, 9);
	memcpy(&msgBuf[10], pass, 6);
	int rtn = sendMsg(msgBuf);
	// printf("Authorize Return %d\n", rtn);
	if (rtn != 0) printf("Authorized Failed\n");
}

void gameStart() {
	/*char id[12], passwd[10];
	std::cout << "ID: " << std::endl;
	std::cin >> id;
	std::cout << "PASSWD: " << std::endl;
	std::cin >> passwd;*/
	authorize(ID, PASSWORD);
	std::cout << "Game Start" << std::endl;
	for (int round = 0; round < ROUNDS; round++) {
		roundStart(round);
		oneRound();
		roundOver(round);
	}
	gameOver();
	close();
}

void gameOver() {
	std::cout << "Game Over" << std::endl;
}

void roundStart(int round) {
	std::cout << "Round " << round << " Ready Start" << std::endl;
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS; c++) {
			board[r][c].reset();
		}
	}
	memset(lastmsg, 0, sizeof(lastmsg));
	int rtn = recvMsg();
	if (rtn != 0) return;
	if (strlen(recvBuf) < 2)
		printf("Authorize Failed\n");
	else
		printf("Round start received msg %s\n", recvBuf);
	switch (recvBuf[1]) {
	case 'B':
		ownColor = 1;
		oppositeColor = 2;
		rtn = sendMsg("BB");
		if (rtn != 0) return;
		break;
	case 'W':
		ownColor = 2;
		oppositeColor = 1;
		rtn = sendMsg("BW");
		std::cout << "Send BW" << rtn << std::endl;
		if (rtn != 0) return;
		break;
	default:
		printf("Authorized Failed\n");
		break;
	}
}

void oneRound() {
	int DIS_FREQ = 5, STEP = 1;
	switch (ownColor) {
	case 1:
		while (STEP < 10000) {

			if (STEP != 1 && (STEP - 1) % DIS_FREQ == 0) {
				int ret = observe();       // self disappeared
				if (ret >= 1) break;
				else if (ret != -8) {
					std::cout << "ERROR: Not Self(BLACK) Disappeared" << std::endl;
				}
			}
			step();                        // take action, send message

			if (observe() >= 1) break;     // receive RET Code
										   // saveChessBoard();
			if (STEP != 1 && (STEP - 1) % DIS_FREQ == 0) {
				int ret = observe();       // see white disappear
				if (ret >= 1) break;
				else if (ret != -9) {
					std::cout << ret << " ERROR: Not White Disappeared" << std::endl;
				}
			}

			if (observe() >= 1) break;    // see white move
			STEP++;
			// saveChessBoard();
		}
		printf("One Round End\n");
		break;
	case 2:
		while (STEP < 10000) {

			if (STEP != 1 && (STEP - 1) % DIS_FREQ == 0) {
				int ret = observe();       // black disappeared
				if (ret >= 1) break;
				else if (ret != -8) {
					std::cout << "ERROR: Not Black Disappeared" << std::endl;
				}
			}
			if (observe() >= 1) break;    // see black move

			if (STEP != 1 && (STEP - 1) % DIS_FREQ == 0) {
				int ret = observe();      // self disappeared
				if (ret >= 1) break;
				else if (ret != -9) {
					std::cout << "ERROR: Not Self Disappeared" << std::endl;
				}
			}

			step();                        // take action, send message
			if (observe() >= 1) break;     // receive RET Code
										   // saveChessBoard();
			STEP++;
		}
		printf("One Round End\n");
		break;
	default:
		break;
	}
}

void roundOver(int round) {
	std::cout << "Round " << round << " Over" << std::endl;
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS; c++) {
			board[r][c].reset();
		}
	}
	ownColor = oppositeColor = -1;
}

void lastMsg() {
	printf(lastmsg);
	puts("");
}

int observe() {
	int rtn = 0;
	int recvrtn = recvMsg();
	if (recvrtn != 0) return 1;
	printf("receive msg %s\n", recvBuf);
	switch (recvBuf[0]) {
	case 'R':   // return messages
	{
		switch (recvBuf[1]) {
		case '0':    // valid step
			switch (recvBuf[2]) {
			case 'P':   // update chessboard
			{
				int desRow = (recvBuf[3] - '0') * 10 + recvBuf[4] - '0';
				int desCol = (recvBuf[5] - '0') * 10 + recvBuf[6] - '0';
				board[desRow][desCol].color = 1 + recvBuf[7] - '0';
				board[desRow][desCol].empty = false;
				memcpy(lastmsg, recvBuf, strlen(recvBuf));
				rl = desRow;
				cl = desCol;
				break;
			}
			case 'D':   // Disappeared
			{
				int desRow = (recvBuf[3] - '0') * 10 + recvBuf[4] - '0';
				int desCol = (recvBuf[5] - '0') * 10 + recvBuf[6] - '0';
				board[desRow][desCol].color = 0;
				board[desRow][desCol].empty = true;
				if (recvBuf[7] - '0' == 0)  // black disappear
					rtn = -8;
				else
					rtn = -9;
				memcpy(lastmsg, recvBuf, strlen(recvBuf));
				break;
			}
			case 'N':   // R0N: enemy wrong step
			{
				break;
			}
			}
			break;
		case '1':
			std::cout << "Error -1: Msg format error\n";
			rtn = -1;
			break;
		case '2':
			std::cout << "Error -2: Coordinate error\n";
			rtn = -2;
			break;
		case '4':
			std::cout << "Error -4: Invalid step\n";
			rtn = -4;
			break;
		default:
			std::cout << "Error -5: Other error\n";
			rtn = -5;
			break;
		}
		break;
	}
	case 'E':
	{
		switch (recvBuf[1]) {
		case '0':
			// game over
			rtn = 2;
			break;
		case '1':
			// round over
			rtn = 1;
		default:
			break;
		}
		break;
	}
	}
	return rtn;
}

void putDown(int row, int col) {
	char msg[6];
	memset(msg, 0, sizeof(msg));
	msg[0] = 'S';
	msg[1] = 'P';
	msg[2] = '0' + row / 10;
	msg[3] = '0' + row % 10;
	msg[4] = '0' + col / 10;
	msg[5] = '0' + col % 10;
	board[row][col].color = ownColor;
	board[row][col].empty = false;
	lastMsg();
	printf("put down (%c%c, %c%c)\n", msg[2], msg[3], msg[4], msg[5]);
	sendMsg(msg);
}

void noStep() {
	sendMsg("SN");
	printf("send msg %s\n", "SN");
}

void saveChessBoard() {

}


#define DEPTH 3
int nin[9];
int rm, cm, rn, cn;
int rt, ct;
int mect[ROWS][ROWS], opct[ROWS][ROWS];
int getscore(int r, int c, bool isme = true);
void rtnrow(Situ & sit, int r, int c, int dir);
void getdir(int r, int c, int dir);
//int nega(int depth, int alpha, int beta, int r, int c);

vector<int> judgeboard() {
	rm = 0, cm = 0, rn = 0, cn = 0;
	for (int r = 0; r < ROWS; ++r)
		for (int c = 0; c < ROWS; ++c) {
			if (board[r][c].empty) {
				mect[r][c] = getscore(r, c);
				if (mect[r][c] > mect[rm][cm]) {
					rm = r; cm = c;
				}
				/*else if (mect[r][c] == mect[rm][cm])
					if (((r - 7) * (r - 7) + (c - 7) * (c - 7)) < ((rm - 7) * (rm - 7) + (cm - 7) * (cm - 7))) {
						rm = r; cm = c;
					}*/
				opct[r][c] = getscore(r, c, false);
				if (opct[r][c] > opct[rn][cn]) {
					rn = r; cn = c;
				}
				/*else if(opct[r][c] == opct[rn][cn])
					if (((r - 7) * (r - 7) + (c - 7) * (c - 7)) < ((rn - 7) * (rn - 7) + (cn - 7) * (cn - 7))) {
						rn = r; cn = c;
					}*/
			}
			else {
				mect[r][c] = -50;
				opct[r][c] = -50;
			}
		}
	vector<int> res;
	if (opct[rn][cn] >= mect[rm][cm]) {
		rm = rn; cm = cn;
		res.push_back(opct[rn][cn]);
	}
	else res.push_back(mect[rm][cm]);
	res.push_back(rm); res.push_back(cm);
	return res;
}

int getscore(int r, int c, bool isme) {
	if (!isme) {
		int temp = ownColor; ownColor = oppositeColor; oppositeColor = temp;
	}
	Situ sit;
	int score;
	for (int dir = 0; dir < 4; ++dir) {
		rtnrow(sit, r, c, dir);
	}
	if (sit.win > 0) score = 100000;
	else if (sit.live4 > 0 || sit.sleep4 > 1 || (sit.sleep4 > 0 && sit.live3 > 0)) score = 10000;
	else if (sit.sleep4 > 0) score = 5000;
	else if (sit.live3 > 1) score = 1000;
	else if (sit.live3 > 0 && sit.sleep3 > 0) score = 500;
	else if (sit.live3 > 0) score = 200;
	else if (sit.live2 > 1) score = 100;
	else if (sit.sleep3 > 0) score = 50;
	else if (sit.sleep2 > 0 && sit.live2 > 0) score = 10;
	else if (sit.live2 > 0) score = 5;
	else if (sit.sleep2 > 0) score = 3;
	else score = -5;
	if (!isme) {
		int temp = ownColor; ownColor = oppositeColor; oppositeColor = temp;
	}
	score = score * 10 - abs(7 - r) - abs(7 - c);
	return score;
}

void rtnrow(Situ & sit, int r, int c, int dir) {
	for (int i = 0; i < 9; ++i)
		nin[i] = 0;
	getdir(r, c, dir);
	int count = 1;
	int left = -1, right = -1;
	for (int i = 3; i > -1; --i) {
		if (nin[i] == ownColor) ++count;
		else {
			if (nin[i] == oppositeColor) left = -1 - i;
			else left = i;
			break;
		}
	}
	for (int i = 5; i < 9; ++i) {
		if (nin[i] == ownColor) ++count;
		else {
			if (nin[i] == oppositeColor) right = -1 - i;
			else right = i;
			break;
		}
	}
	/*bool noleft = false, noright = false;
	for (int i = 3; i > -1; --i) {
		if (nin[i] == ownColor) ++count;
		else if (nin[i] == oppositeColor) {
			noleft = true;
			break;
		}
	}
	for (int i = 5; i < 9; ++i) {
		if (nin[i] == ownColor) ++count;
		else if (nin[i] == oppositeColor) {
			noright = true;
			break;
		}
	}
	*/
	switch (count) {
	case 5: sit.win++; break;
	case 4: {
		if (left < 0 && right < 0) sit.die++;
		else if (left < 0 || right < 0) sit.sleep4++;
		else sit.live4++;
	} break;
	case 3: {
		if (left < 0 && right < 0) sit.die++;
		else if (left < 0) {
			if(nin[right + 1] == ownColor) sit.sleep4++;
			else if (nin[right + 1] == oppositeColor) sit.die++;
			else sit.sleep3++;
		}
		else if (right < 0) {
			if(nin[left - 1] == ownColor) sit.sleep4++;
			else if (nin[left - 1] == oppositeColor) sit.die++;
			else sit.sleep3++;
		}
		else sit.live3++;
	} break;
	case 2: {
		if (left < 0 && right < 0) sit.die++;
		else if (left < 0) {
			if (nin[right + 1] == ownColor) {
				if (nin[right + 2] == ownColor) sit.sleep4++;
				else if (nin[right + 2] == oppositeColor) sit.die++;
				else sit.sleep3++;
			}
			else if (nin[right + 1] == oppositeColor) sit.die++;
			else if (nin[right + 2] == ownColor) sit.sleep3++;
			else if (nin[right + 2] == oppositeColor) sit.die++;
			else sit.sleep2++;
		}
		else if (right < 0) {
			if (nin[left - 1] == ownColor) {
				if (nin[left - 2] == ownColor) sit.sleep4++;
				else if (nin[left - 2] == oppositeColor) sit.die++;
				else sit.sleep3++;
			}
			else if (nin[left - 1] == oppositeColor) sit.die++;
			else if (nin[left - 2] == ownColor) sit.sleep3++;
			else if (nin[left - 2] == oppositeColor) sit.die++;
			else sit.sleep2++;
		}
		else {
			if (nin[left - 1] == oppositeColor && nin[right + 1] == oppositeColor) sit.die++;
			else if (nin[left - 1] == oppositeColor) {
				if (nin[right + 2] == oppositeColor) sit.die++;
				else if (nin[right + 2] == ownColor) sit.live3++;
				else sit.live2++;
			}
			else if (nin[right + 1] == oppositeColor) {
				if (nin[left - 2] == oppositeColor) sit.die++;
				else if (nin[left - 2] == ownColor) sit.live3++;
				else sit.live2++;
			}
			else sit.live2++;
		}
	} break;
	case 1: {
		if (nin[left] < 0 && nin[right] < 0) sit.die++;
		else if (nin[left] < 0 || nin[right] < 0) sit.sleep1++;
		else sit.live1++;
	} break;
	default: sit.win++; break;
	}
}

void getdir(int r, int c, int dir) {
	switch (dir) {
	case 0: {
		for (int i = -4; i < 5; ++i) {
			if (r + i < 0 || r + i > 14)
				nin[i + 4] = oppositeColor;
			else nin[i + 4] = board[r + i][c].color;
		}
	}
			break;
	case 1: {
		for (int i = -4; i < 5; ++i) {
			if (r + i < 0 || r + i > 14 || c + i < 0 || c + i > 14)
				nin[i + 4] = oppositeColor;
			else nin[i + 4] = board[r + i][c + i].color;
		}
	}
			break;
	case 2: {
		for (int i = -4; i < 5; ++i) {
			if (c + i < 0 || c + i > 14)
				nin[i + 4] = oppositeColor;
			else nin[i + 4] = board[r][c + i].color;
		}
	}
			break;
	case 3: {
		for (int i = -4; i < 5; ++i) {
			if (r - i < 0 || r - i > 14 || c + i < 0 || c + i > 14)
				nin[i + 4] = oppositeColor;
			else nin[i + 4] = board[r - i][c + i].color;
		}
	}
			break;
	}
	nin[4] = ownColor;
}


bool iswin(int r, int c) {
	if (getscore(r, c) == 100000) return true;
	else if (getscore(r, c, false) == 100000) return true;
	else return false;
}

/*


int negascout(int depth, int alpha, int beta, int r, int c) {
	bool isme = !(DEPTH - depth) % 2;
	if (iswin(r, c)) {
		if (isme) return INT_MAX;
		else return INT_MIN;
	}
	if (depth == 0)
		if (isme) return getscore(r, c);
		else return -getscore(r, c, false);
	if (isme) {
		for (int i = 0; i < ROWS; ++i)
			for (int j = 0; j < ROWS; ++j)
				if (board[i][j].empty) {
					if (alpha >= beta) return alpha;
					int res = nega(depth, alpha, beta, r, c);
					if (res > alpha) alpha = res;
				}
		return alpha;
	}
	else {
		for(int i = 0; i < ROWS; ++i)
			for(int j = 0; j < ROWS; ++j)
				if (board[i][j].empty) {
					if (alpha >= beta) return alpha;
					int res = nega(depth, alpha, beta, r, c);
					if (res < beta) beta = res;
				}
		return beta;
	}
}

int nega(int depth, int alpha, int beta, int r, int c) {
	preput(depth - 1, r, c);
	int res = negascout(depth - 1, alpha, beta, r, c);
	prerev(r, c);
	return res;
}


vector<int> abget() {
	int r, c;
	int sc;
	for(int i = 0; i < ROWS; ++i)
		for(int j = 0; j < ROWS; ++j)
			if (board[i][j].empty) {
				r = i; c = j;
				sc = nega(DEPTH + 1, INT_MIN, INT_MAX, r, c);
				goto QUICK;
			}
	QUICK:
	for(int i = 0; i < ROWS; ++i)
		for(int j = 0; j < ROWS; ++j)
			if (board[i][j].empty) {
				int temp = nega(DEPTH + 1, INT_MIN, INT_MAX, i, j);
				if (sc <= temp) {
					r = i; c = j;
					sc = temp;
				}
			}
	return { r, c };
}

*/

void preput(bool isme, int r, int c) {
	//if ((DEPTH - depth) % 2 == 0 ? true : false) board[r][c].color = ownColor;
	//else board[r][c].color = oppositeColor;
	board[r][c].color = isme ? ownColor : oppositeColor;
	board[r][c].empty = false;
}

void prerev(int r, int c) {
	board[r][c].reset();
}


struct Pos {
	int r;
	int c;
	int sc;
};

//Pos sz(Pos pos, int depth) {
//	Pos meH, meL, opH, opL;
//	meH.sc = INT_MIN; opH.sc = INT_MIN;
//	meL.sc = INT_MAX; opL.sc = INT_MAX;
//	int scores[ROWS][ROWS];
//	bool isme = !(depth % 2);
//	preput(isme, pos.r, pos.c);
//	for (int i = 0; i < ROWS; ++i)
//		for (int j = 0; j < ROWS; ++j) {
//			if (board[i][j].empty) {
//				mect[i][j] = getscore(i, j, isme);
//				opct[i][j] = getscore(i, j, !isme);
//				if (mect[i][j] >= meH.sc) {
//					meH.sc = mect[i][j]; meH.r = i; meH.c = j;
//				}
//				else if (mect[i][j] <= meL.sc) {
//					meL.sc = mect[i][j]; meL.r = i; meL.c = j;
//				}
//				if (opct[i][j] >= opH.sc) {
//					opH.sc = opct[i][j]; opH.r = i; opH.c = j;
//				}
//				else if (opct[i][j] <= opL.sc) {
//					opL.sc = opct[i][j]; opL.r = i; opL.c = j;
//				}
//			}
//			else {
//				mect[i][j] = INT_MIN;
//				opct[i][j] = INT_MIN;
//			}
//		}
//
//}

int sz(Pos pos, int depth) {
	bool isme = !(depth % 2);
	int mme = 0, mop = 0;
	preput(isme, pos.r, pos.c);
	if (iswin(pos.r, pos.c)) {
		if (isme) return INT_MIN;
		else return INT_MAX;
	}
	if (depth == DEPTH) {
		prerev(pos.r, pos.c);
		return getscore(pos.r, pos.c, isme);
	}
	for (int i = 0; i < ROWS; ++i)
		for (int j = 0; j < ROWS; ++j) {
			if (board[i][j].empty) {
				mect[i][j] = getscore(i, j, isme);
				opct[i][j] = getscore(i, j, !isme);
				if (mect[i][j] >= mme) {
					mme = mect[i][j]; rm = i; cm = j;
				}
				if (opct[i][j] >= mop) {
					mop = opct[i][j]; rn = i; cn = j;
				}
			}
		}
	if (isme) {
		if (mop >= 500) {

		}
	}
}

void step() {
	for (int i = -1; i < 15; ++i) printf("%2d ", i);
		printf("\n");
	for (int r = 0; r < ROWS; ++r) {
		printf("%2d", r);
		for (int c = 0; c < ROWS; ++c) {
			printf("%2d ", board[r][c].color);
		}
		printf("\n");
	}
	/*judgeboard();
	putDown(rm, cm);*/
	Pos cur; cur.r = rl; cur.c = cl;
	cur.sc = getscore(rl, cl, false);
	sz(cur, 0);
}
