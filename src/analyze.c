#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include "pacman.h"
#include "gameloop.h"
#include "player.h"
#include "ghosts.h"
#include "board.h"
#include "render.h"
#include "pills.h"

#define MAX_DEPTH 10
#define CHECK_DEPTH 0
#define SHOW_DEPTH 2
#define MAX_SCORE 999999
#define ALPHA 0.8

inline void fprintf_sep(FILE *, int);

#define MAPSIZE 672
int bMap[672];
int width;
#define BMAP(i, j) bMap[i + j*width]

/* Driver for the Search Agent */
int Pac_AnalyzeGameState(GAME_STATE gs, int depth)
{

	FILE * outfile = fopen("analyze_output", "a");
	fprintf(outfile, "Analyzing (%d, %d)\n", gs.Player.Pos.x, gs.Player.Pos.y);
	
	width = gs.iMapWidth;

	int key = AnalyzeIter(gs, 0, outfile);

	fprintf(outfile, "Key: %c\n", key);

	fprintf(outfile, "\n----\n\n");
	fclose(outfile);

	return key;
}

/* Search Iterations. Uses DFS. */
int AnalyzeIter(GAME_STATE gs, int depth, FILE * outfile)
{

	if (depth == MAX_DEPTH) {
		return GameStateHeuristic(&gs);
	}

	if (depth < SHOW_DEPTH) {
		fprintf_sep(outfile, depth);
		fprintf(outfile, "(%d, %d)\n", gs.Player.Pos.x, gs.Player.Pos.y);
	}

	int score_Left = 0;
	int score_Right = 0;
	int score_Up = 0;
	int score_Down = 0;
	
	// Left
	BOOL bLeft = isValidMove(&gs, eDIR_Left);
	if (bLeft) {
		GAME_STATE gs_Left = gs;
		GameStateUpdate(&gs_Left, PACKEY_LEFT);
		if (gameEnd(&gs_Left)) score_Left = endScore(&gs_Left);
		else score_Left = AnalyzeIter(gs_Left, depth + 1, outfile);
		if (depth < SHOW_DEPTH) {
			fprintf_sep(outfile, depth+1);
			fprintf(outfile, "L: %d\n", score_Left);
		}
	} else {
		if (depth < SHOW_DEPTH) {
			fprintf_sep(outfile, depth+1);
			fprintf(outfile, "L: NA\n");
		}
	}
	
	// Right
	BOOL bRight = isValidMove(&gs, eDIR_Right);
	if (bRight) {
		GAME_STATE gs_Right = gs;
		GameStateUpdate(&gs_Right, PACKEY_RIGHT);
		if (gameEnd(&gs_Right)) score_Right = endScore(&gs_Right);
		else score_Right = AnalyzeIter(gs_Right, depth + 1, outfile);
		if (depth < SHOW_DEPTH) {
			fprintf_sep(outfile, depth+1);
			fprintf(outfile, "R: %d\n", score_Right);
		}
	} else {
		if (depth < SHOW_DEPTH) {
			fprintf_sep(outfile, depth+1);
			fprintf(outfile, "R: NA\n");
		}
	}
	
	// Up
	BOOL bUp = isValidMove(&gs, eDIR_Up);
	if (bUp) {
		GAME_STATE gs_Up = gs;
		GameStateUpdate(&gs_Up, PACKEY_UP);
		if (gameEnd(&gs_Up)) score_Up = endScore(&gs_Up);
		else score_Up = AnalyzeIter(gs_Up, depth + 1, outfile);
		if (depth < SHOW_DEPTH) {
			fprintf_sep(outfile, depth+1);
			fprintf(outfile, "U: %d\n", score_Up);
		}
	} else {
		if (depth < SHOW_DEPTH) {
			fprintf_sep(outfile, depth+1);
			fprintf(outfile, "U: NA\n");
		}
	}
	
	// Down
	BOOL bDown = isValidMove(&gs, eDIR_Down);
	if (bDown) {
		GAME_STATE gs_Down = gs;
		GameStateUpdate(&gs_Down, PACKEY_DOWN);
		if (gameEnd(&gs_Down)) score_Down = endScore(&gs_Down);
		else score_Down = AnalyzeIter(gs_Down, depth + 1, outfile);
		if (depth < SHOW_DEPTH) {
			fprintf_sep(outfile, depth+1);
			fprintf(outfile, "D: %d\n", score_Down);
		}
	} else {
		if (depth < SHOW_DEPTH) {
			fprintf_sep(outfile, depth+1);
			fprintf(outfile, "D: NA\n");
		}
	}
	
	if (depth) {
		return (int) ((float) gs.Player.iScore + ALPHA*maxScore(score_Left, score_Right, score_Up, score_Down));
	} else {
		int max_Score = maxScore(score_Left, score_Right, score_Up, score_Down);
		if (max_Score == score_Left) return PACKEY_LEFT;
		else if (max_Score == score_Right) return PACKEY_RIGHT;
		else if (max_Score == score_Up) return PACKEY_UP;
		else return PACKEY_DOWN;
	}
}

/* Used to check if valid move */
char getNextChar(GAME_STATE *gs_p, tDir dir)
{
	uint x, y;
	char c;

	x = gs_p->Player.Pos.x; y = gs_p->Player.Pos.y;
	switch(dir) {
		case	eDIR_Left:	x--; break;
		case	eDIR_Right:	x++; break;
		case	eDIR_Up:	y--; break;
		case	eDIR_Down:	y++; break;
	}
	x = x % gs_p->iMapWidth;
	y = y % gs_p->iMapHeight;
	
	if (Pac_GetMap(gs_p, x, y, &c))
		return c;
	else
		return '\0';
}

BOOL isValidMove(GAME_STATE *gs_p, tDir dir)
{
	char c = getNextChar(gs_p, dir);
	return Pac_IsOpenArea(c);
}

/* Update the GAME_STATE */
int GameStateUpdate(GAME_STATE *gs_p, int keypress)
{
	Pac_UpdatePlayer(gs_p, 0.2, keypress);
	Pac_CheckPlayerVsGhosts(gs_p);
	Pac_UpdateGhosts(gs_p, 0.2);
	Pac_CheckPlayerVsGhosts(gs_p);
	Pac_UpdatePills(gs_p, 0.2);
}

/* Pretty Printing */
inline void fprintf_sep(FILE * outfile, int depth)
{
	int i;
	for (i = 0; i < depth; ++i) fprintf(outfile, "| ");
}

/* Goal State? */
inline BOOL gameEnd(GAME_STATE *gs_p)
{
	if ((gs_p->iDotsLeft == 0) || (gs_p->Player.bDead)) return TRUE;
}

inline int endScore(GAME_STATE *gs_p)
{
	if (gs_p->iDotsLeft == 0)	return MAX_SCORE;
	else if (gs_p->Player.bDead)	return -MAX_SCORE;
}

/* Max of four elements.. */
inline int maxScore(int a, int b, int c, int d)
{
	if (a > b) b = a;
	if (d > c) c = d;
	if (b > c) return b; else return c;
}

/* Heuristics functions and weights */
inline int GameStateHeuristic(GAME_STATE *gs_p)
{
	int total = 0;
	
	int wScore = (int) ((float) 1 * gs_p->Player.iScore);
	total += wScore;

	// wLives
	// total += 10000 * gs_p->Player.iLives;

	// Because of a depth of 10, we don't need to stay away from ghosts
	// Maybe when they're actually intelligent...
	
	// wMinDist //TODO
	// total += minDotDist(gs_p, gs_p->Player.Pos.x, gs_p->Player.Pos.y);

	int wMinApproxDist;
	if (gs_p->iDotsLeft > 15) {
		// Manhattan
		wMinApproxDist = (int) ((float) 100.0/(float)(minManhattanDist(gs_p, gs_p->Player.Pos.x, gs_p->Player.Pos.y)+1));
	} else {
		// Euclidean
		wMinApproxDist = (int) ((float) 100.0/(float)(minEuclideanDist(gs_p, gs_p->Player.Pos.x, gs_p->Player.Pos.y)+1));
	}

	clearbMap();
	int wSurr = (int) ((float) 0.5 * checkSurr(gs_p, gs_p->Player.Pos.x, gs_p->Player.Pos.y, 0));
	total += wSurr;

	int wDotsLeft = (int) ((float) 1000.0/(float)gs_p->iDotsLeft);
	
	total += (wMinApproxDist * wDotsLeft);

	return total;
}

/* Score the surroundings based on features */
inline int checkSurr(GAME_STATE *gs_p, uint x, uint y, int depth)
{
	if (depth == CHECK_DEPTH) return 0;

	int score = 0;

	x = x % gs_p->iMapWidth;
	y = y % gs_p->iMapHeight;
	
	BMAP(x,y) = 1;
	
	char c;

	Pac_GetMap(gs_p, x, y, &c);
	
	switch (c) {
		case '.': score += 1; break;
		case 'P': score -= 100; break;
		// case ' ': score += 0; break;
		// case '*': score += 5;
		// case '$': score += 10; break;
		case '#': return 0;
	}

	if (!BMAP(x,y+1)) score += checkSurr(gs_p, x, y+1, depth+1);
	if (!BMAP(x+1,y)) score += checkSurr(gs_p, x+1, y, depth+1);
	if (!BMAP(x,y-1)) score += checkSurr(gs_p, x, y-1, depth+1);
	if (!BMAP(x-1,y)) score += checkSurr(gs_p, x-1, y, depth+1);
	
	return (CHECK_DEPTH-depth+1)*(score);
}

/* Iterative deepening search. Too slow for my laptop, unfortunately */
inline int minDotDist(GAME_STATE *gs_p, uint x, uint y)
{
	int depth;

	for (depth = 0; ;++depth) {
		clearbMap();
		if (depthLimitedSearch(gs_p, x, y, depth)) return depth;
	}
}

inline int depthLimitedSearch(GAME_STATE *gs_p, uint x, uint y, int depth)
{
	x = x % gs_p->iMapWidth;
	y = y % gs_p->iMapHeight;
	
	BMAP(x,y) = 1;
	
	char c;
	Pac_GetMap(gs_p, x, y, &c);
	switch (c) {
		case '.': return 1;
		case '#': return 0;
	}

	if (depth == 0) return 0;
	if (!BMAP(x,y+1)) {if (depthLimitedSearch(gs_p, x, y+1, depth-1)) return 1;}
	if (!BMAP(x+1,y)) {if (depthLimitedSearch(gs_p, x+1, y, depth-1)) return 1;}
	if (!BMAP(x,y-1)) {if (depthLimitedSearch(gs_p, x, y-1, depth-1)) return 1;}
	if (!BMAP(x-1,y)) {if (depthLimitedSearch(gs_p, x-1, y, depth-1)) return 1;}
	
	return 0;
}

/* Minimum Manhattan Distance to a pellet */
inline int minManhattanDist(GAME_STATE *gs_p, uint x, uint y)
{
	int i, j;
	char c = '.';

	int dist, minDist;

	for (i = 0; i < gs_p->iMapWidth; ++i) {
		for (j = 0; j < gs_p->iMapHeight; ++j) {
			if (gs_p->Map[i + width*j] == c) {
				dist = manhattanDist(i, j, x, y);
				if (dist < minDist) minDist = dist;
			}
		}
	}

	return minDist;
}

inline int manhattanDist(int x1, int y1, int x2, int y2)
{
	int dist = 0;

	if (x1 > x2) dist += x1-x2;
	else dist += x2-x1;

	if (y1 > y2) dist += y1-y2;
	else dist += y2-y1;

	return dist;
}

/* Minimum Euclidean Distance to a pellet */
inline int minEuclideanDist(GAME_STATE *gs_p, uint x, uint y)
{
	int i, j;
	char c = '.';

	int dist, minDist;

	for (i = 0; i < gs_p->iMapWidth; ++i) {
		for (j = 0; j < gs_p->iMapHeight; ++j) {
			if (gs_p->Map[i + width*j] == c) {
				dist = euclideanDist(i, j, x, y);
				if (dist < minDist) minDist = dist;
			}
		}
	}

	return minDist;
}

inline int euclideanDist(int x1, int y1, int x2, int y2)
{
	float dist = (float) (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
	return (int) dist;
}

/* Boolean Map */
inline void clearbMap()
{
	memset(bMap, 0, sizeof(int)*MAPSIZE);
}

inline int verifyCleanbMap()
{
	int i;

	for (i = 0; i < MAPSIZE; ++i) {
		if (bMap[i] != 0) fprintf(stderr, "bMap not clean!\n");
	}
}
