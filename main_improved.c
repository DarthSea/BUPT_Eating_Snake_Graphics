#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define MAP_SIZE 20
#define WALL '#'
#define EMPTY '.'
#define SNAKE_HEAD 'H'
#define SNAKE_BODY 'B'
#define FOOD 'F'
#define OBSTACLE 'O'
#define MAX_SNAKE_LEN 400
#define FOOD_SCORE 10
enum DIR { DIR_UP = 0, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
enum GAME_STATUS { OK = 0, KILL_BY_WALL, KILL_BY_SELF, KILL_BY_OBSTACLE, OVER };
typedef struct { int x; int y; } Pos;
typedef struct Snake_Node { Pos pos; struct Snake_Node* next_node; } SN, *pSN;
typedef struct Snake { pSN _pSnake; pSN _pFood; enum DIR _dir; enum GAME_STATUS _status; int len; int score; int stepCount; int growthN; } Snake, *pSnake;
char map[20][21];
pSN creatSnakeNode(int x, int y);
void readMap(pSnake ps);
void initSnake(pSnake ps);
void initGame(pSnake ps);
void checkCollision(pSnake ps, int x, int y, bool willGrow);
void moveSnake(pSnake ps, enum DIR nextDir);
void gameLoop(pSnake ps);
enum DIR decideDirection(pSnake ps);
int  bfs(pSnake ps, int sx, int sy, int gx, int gy, int blockTail);
int  safeMove(pSnake ps, enum DIR dir);
int  flood(pSnake ps, int sx, int sy, int grow);

int main() { Snake snake = { 0 }; initGame(&snake); gameLoop(&snake); return 0; }

pSN creatSnakeNode(int x, int y) {
	pSN n = (pSN)malloc(sizeof(SN)); if (!n) exit(-1);
	n->pos.x = x; n->pos.y = y; n->next_node = NULL; return n;
}

void readMap(pSnake ps) {
	for (int i = 0; i < 20; i++) scanf("%20s", map[i]);
	scanf("%d", &ps->growthN);
	for (int i = 0; i < 20; i++) for (int j = 0; j < 20; j++) {
		if (map[i][j] == SNAKE_HEAD) { ps->_pSnake->pos.x = i; ps->_pSnake->pos.y = j; }
		else if (map[i][j] == FOOD) ps->_pFood = creatSnakeNode(i, j);
	}
}

void initSnake(pSnake ps) {
	ps->_dir = DIR_UP; ps->score = 0; ps->_status = OK; ps->len = 1;
	int x = ps->_pSnake->pos.x, y = ps->_pSnake->pos.y; pSN tail = ps->_pSnake;
	int dr[8] = { -1,1,0,0,-1,-1,1,1 }, dc[8] = { 0,0,-1,1,-1,1,-1,1 };
	bool found;
	do { found = false;
		for (int d = 0; d < 8; d++) { int nr = x + dr[d], nc = y + dc[d];
			if (nr >= 0 && nr < 20 && nc >= 0 && nc < 20 && map[nr][nc] == SNAKE_BODY) {
				map[nr][nc] = 'X'; tail->next_node = creatSnakeNode(nr, nc);
				tail = tail->next_node; ps->len++; x = nr; y = nc; found = true; break; } }
	} while (found);
	for (pSN p = ps->_pSnake->next_node; p; p = p->next_node) map[p->pos.x][p->pos.y] = SNAKE_BODY;
	for (int i = 0; i < 20; i++) for (int j = 0; j < 20; j++)
		if (map[i][j] == SNAKE_BODY) { bool in = false;
			for (pSN p = ps->_pSnake->next_node; p; p = p->next_node)
				if (p->pos.x == i && p->pos.y == j) { in = true; break; }
			if (!in) map[i][j] = EMPTY; }
}

void initGame(pSnake ps) { ps->_pSnake = creatSnakeNode(0, 0); readMap(ps); initSnake(ps); }

void gameLoop(pSnake ps) {
	while (ps->_status != OVER) {
		enum DIR nd = decideDirection(ps);
		char dc; switch (nd) { case DIR_UP:dc='W';break; case DIR_DOWN:dc='S';break; case DIR_LEFT:dc='A';break; case DIR_RIGHT:dc='D';break; default:dc='W';break; }
		printf("%c\n%d\n", dc, ps->score); fflush(stdout);
		moveSnake(ps, nd);
		int f1, f2; if (scanf("%d %d", &f1, &f2) != 2) { ps->_status = OVER; break; }
		if (f1 == 100 && f2 == 100) { ps->_status = OVER; break; }
		else if (f1 == 20 && f2 == 20) {}
		else if (f1 > 0 && f1 < 19 && f2 > 0 && f2 < 19) { ps->_pFood->pos.x = f1; ps->_pFood->pos.y = f2; map[f1][f2] = FOOD; }
	}
	{for(int i=0;i<20;i++)printf("%.20s\n",map[i]);}printf("%d\n",ps->score);fflush(stdout);
}

void checkCollision(pSnake ps, int x, int y, bool willGrow) {
	pSN c = ps->_pSnake;
	while (c) { if (!willGrow && !c->next_node) break; if (x == c->pos.x && y == c->pos.y) { ps->_status = KILL_BY_SELF; return; } c = c->next_node; }
	if (x < 0 || x >= MAP_SIZE || y < 0 || y >= MAP_SIZE) { ps->_status = KILL_BY_WALL; return; }
	if (map[x][y] == WALL || map[x][y] == OBSTACLE) { ps->_status = (map[x][y] == WALL) ? KILL_BY_WALL : KILL_BY_OBSTACLE; return; }
}

void moveSnake(pSnake ps, enum DIR nd) {
	if ((nd == DIR_UP && ps->_dir == DIR_DOWN) || (nd == DIR_DOWN && ps->_dir == DIR_UP) || (nd == DIR_LEFT && ps->_dir == DIR_RIGHT) || (nd == DIR_RIGHT && ps->_dir == DIR_LEFT)) { ps->_status = KILL_BY_SELF; return; }
	ps->_dir = nd;
	int nx = ps->_pSnake->pos.x, ny = ps->_pSnake->pos.y;
	if (nd == DIR_UP) nx--; else if (nd == DIR_DOWN) nx++; else if (nd == DIR_LEFT) ny--; else if (nd == DIR_RIGHT) ny++;
	bool ate = (nx == ps->_pFood->pos.x && ny == ps->_pFood->pos.y), willGrow = ate || (ps->stepCount + 1 == ps->growthN);
	checkCollision(ps, nx, ny, willGrow); if (ps->_status != OK) return;
	pSN nh = creatSnakeNode(nx, ny); nh->next_node = ps->_pSnake; ps->_pSnake = nh; ps->len++;
	map[nh->next_node->pos.x][nh->next_node->pos.y] = SNAKE_BODY; map[nx][ny] = SNAKE_HEAD;
	if (ate) ps->score += FOOD_SCORE;
	ps->stepCount++; bool gbn = (ps->stepCount == ps->growthN), sg = ate || gbn;
	if (gbn) ps->stepCount = 0;
	if (!sg) { pSN c = ps->_pSnake; while (c->next_node->next_node) c = c->next_node;
		if (c->next_node->pos.x != nx || c->next_node->pos.y != ny) map[c->next_node->pos.x][c->next_node->pos.y] = EMPTY;
		free(c->next_node); c->next_node = NULL; ps->len--; }
}

/* ---- flood fill: count reachable cells (lightweight) ---- */
int flood(pSnake ps, int sx, int sy, int grow) {
	int v[20][20] = { 0 };
	for (int i = 0; i < 20; i++) for (int j = 0; j < 20; j++) if (map[i][j] == WALL || map[i][j] == OBSTACLE) v[i][j] = 1;
	pSN c = ps->_pSnake; while (c) { if (grow || c->next_node) v[c->pos.x][c->pos.y] = 1; c = c->next_node; }
	Pos q[400]; int f = 0, r = 0, cnt = 0;
	q[r].x = sx; q[r].y = sy; r++; v[sx][sy] = 1;
	int dr[4] = { -1,1,0,0 }, dc[4] = { 0,0,-1,1 };
	while (f < r) { Pos now = q[f++]; cnt++;
		for (int d = 0; d < 4; d++) { int nr = now.x + dr[d], nc = now.y + dc[d];
			if (nr >= 0 && nr < 20 && nc >= 0 && nc < 20 && !v[nr][nc]) { v[nr][nc] = 1; q[r].x = nr; q[r].y = nc; r++; } }
	}
	return cnt;
}

/* ---- single-step safety: BFS from new head to future tail ---- */
int safeMove(pSnake ps, enum DIR dir) {
	int hx = ps->_pSnake->pos.x, hy = ps->_pSnake->pos.y, nhx = hx, nhy = hy;
	switch (dir) { case DIR_UP:nhx--;break; case DIR_DOWN:nhx++;break; case DIR_LEFT:nhy--;break; case DIR_RIGHT:nhy++;break; }
	if (nhx < 0 || nhx >= 20 || nhy < 0 || nhy >= 20) return 0;
	if (map[nhx][nhy] == WALL || map[nhx][nhy] == OBSTACLE) return 0;
	bool ate = (nhx == ps->_pFood->pos.x && nhy == ps->_pFood->pos.y), wg = ate || (ps->stepCount + 1 == ps->growthN);
	pSN cur = ps->_pSnake;
	while (cur) { if (!wg && !cur->next_node) break; if (nhx == cur->pos.x && nhy == cur->pos.y) return 0; cur = cur->next_node; }
	pSN tail = ps->_pSnake; while (tail->next_node) tail = tail->next_node;
	Pos ft; if (wg) { ft.x = tail->pos.x; ft.y = tail->pos.y; }
	else { pSN c = ps->_pSnake; while (c->next_node && c->next_node->next_node) c = c->next_node; ft.x = c->pos.x; ft.y = c->pos.y; }
	if (nhx == ft.x && nhy == ft.y) return 1;
	int v[20][20] = { 0 };
	for (int i = 0; i < 20; i++) for (int j = 0; j < 20; j++) if (map[i][j] == WALL || map[i][j] == OBSTACLE) v[i][j] = 1;
	cur = ps->_pSnake; while (cur) { if (wg || cur->next_node) v[cur->pos.x][cur->pos.y] = 1; cur = cur->next_node; }
	v[nhx][nhy] = 0; v[ft.x][ft.y] = 0;
	Pos q[400]; int f = 0, r = 0; q[r].x = nhx; q[r].y = nhy; r++; v[nhx][nhy] = 1;
	int dr[4] = { -1,1,0,0 }, dc[4] = { 0,0,-1,1 };
	while (f < r) { Pos now = q[f++]; if (now.x == ft.x && now.y == ft.y) return 1;
		for (int d = 0; d < 4; d++) { int nr = now.x + dr[d], nc = now.y + dc[d];
			if (nr >= 0 && nr < 20 && nc >= 0 && nc < 20 && !v[nr][nc]) { v[nr][nc] = 1; q[r].x = nr; q[r].y = nc; r++; } } }
	return 0;
}

/* ---- unified BFS: shortest path, returns first step direction or -1 ---- */
int bfs(pSnake ps, int sx, int sy, int gx, int gy, int blockTail) {
	if (sx == gx && sy == gy) return -2;
	int v[MAP_SIZE][MAP_SIZE] = { 0 }, pr[MAP_SIZE][MAP_SIZE][2] = { 0 };
	for (int i = 0; i < MAP_SIZE; i++) for (int j = 0; j < MAP_SIZE; j++)
		if (map[i][j] == WALL || map[i][j] == OBSTACLE) v[i][j] = 1;
	bool wg = blockTail || (ps->stepCount + 1 == ps->growthN);
	pSN c = ps->_pSnake; while (c) { if (c->next_node || wg) v[c->pos.x][c->pos.y] = 1; c = c->next_node; }
	if (v[gx][gy]) return -1;
	Pos q[MAP_SIZE * MAP_SIZE]; int f = 0, r = 0; q[r].x = sx; q[r].y = sy; r++; v[sx][sy] = 1;
	int dr[4] = { -1,1,0,0 }, dc[4] = { 0,0,-1,1 };
	while (f < r) { Pos now = q[f++];
		if (now.x == gx && now.y == gy) { int cx = gx, cy = gy;
			while (pr[cx][cy][0] != sx || pr[cx][cy][1] != sy) { int tx = pr[cx][cy][0], ty = pr[cx][cy][1]; cx = tx; cy = ty; }
			if (cx < sx) return DIR_UP; else if (cx > sx) return DIR_DOWN; else if (cy < sy) return DIR_LEFT; else return DIR_RIGHT;
		}
		for (int d = 0; d < 4; d++) { int nr = now.x + dr[d], nc = now.y + dc[d];
			if (nr >= 0 && nr < MAP_SIZE && nc >= 0 && nc < MAP_SIZE && !v[nr][nc]) { v[nr][nc] = 1; pr[nr][nc][0] = now.x; pr[nr][nc][1] = now.y; q[r].x = nr; q[r].y = nc; r++; } }
	}
	return -1;
}

/* ---- main AI decision ---- */
enum DIR decideDirection(pSnake ps) {
	int hx = ps->_pSnake->pos.x, hy = ps->_pSnake->pos.y, fx = ps->_pFood->pos.x, fy = ps->_pFood->pos.y;
	pSN t = ps->_pSnake; while (t->next_node) t = t->next_node;
	int tx = t->pos.x, ty = t->pos.y;
	int dr[4] = { -1,1,0,0 }, dc[4] = { 0,0,-1,1 };
	enum DIR dirs[4] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
	int rev(int a) { return (dirs[a] == DIR_UP && ps->_dir == DIR_DOWN) || (dirs[a] == DIR_DOWN && ps->_dir == DIR_UP) || (dirs[a] == DIR_LEFT && ps->_dir == DIR_RIGHT) || (dirs[a] == DIR_RIGHT && ps->_dir == DIR_LEFT); }

	int dir2food = bfs(ps, hx, hy, fx, fy, 0);
	/* P1: BFS to food + safeMove + virtual check (conservative, one BFS) */
	if (dir2food >= 0 && safeMove(ps, (enum DIR)dir2food)) {
		if (bfs(ps, fx, fy, tx, ty, 1) >= -1) return (enum DIR)dir2food;
	}
	/* P2: BFS to food with tail blocked + safe + virtual */
	int dir2foodB = bfs(ps, hx, hy, fx, fy, 1);
	if (dir2foodB >= 0 && safeMove(ps, (enum DIR)dir2foodB)) {
		if (bfs(ps, fx, fy, tx, ty, 1) >= -1) return (enum DIR)dir2foodB;
	}
	/* P3: close food — skip virt check, just check flood area */
	if (dir2food >= 0 && safeMove(ps, (enum DIR)dir2food)) {
		int nhx = hx, nhy = hy;
		switch (dir2food) { case DIR_UP:nhx--;break; case DIR_DOWN:nhx++;break; case DIR_LEFT:nhy--;break; case DIR_RIGHT:nhy++;break; }
		int dist = abs(nhx - fx) + abs(nhy - fy);
		if (dist <= 3) {
			bool gw = (nhx == fx && nhy == fy) || (ps->stepCount + 1 == ps->growthN);
			if (flood(ps, nhx, nhy, gw) > ps->len + 10) return (enum DIR)dir2food;
		}
	}
	/* P4: BFS to tail — survival */
	int dir2tail = bfs(ps, hx, hy, tx, ty, 0);
	if (dir2tail >= 0 && safeMove(ps, (enum DIR)dir2tail)) return (enum DIR)dir2tail;
	dir2tail = bfs(ps, hx, hy, tx, ty, 1);
	if (dir2tail >= 0 && safeMove(ps, (enum DIR)dir2tail)) return (enum DIR)dir2tail;
	/* P5: safe direction closest to food (Manhattan) + flood area tiebreaker */
	{ int bd = -1, bdist = 99999, barea = -1;
	for (int d = 0; d < 4; d++) {
		if (rev(d)) continue;
		int nr = hx + dr[d], nc = hy + dc[d];
		if (nr < 0 || nr >= 20 || nc < 0 || nc >= 20) continue;
		if (map[nr][nc] == WALL || map[nr][nc] == OBSTACLE) continue;
		if (!safeMove(ps, dirs[d])) continue;
		int dist = abs(nr - fx) + abs(nc - fy);
		bool gw = (nr == fx && nc == fy) || (ps->stepCount + 1 == ps->growthN);
		if (dist < bdist || (dist == bdist && barea == -1)) {
			barea = flood(ps, nr, nc, gw); bdist = dist; bd = d;
		} else if (dist == bdist) {
			int a = flood(ps, nr, nc, gw); if (a > barea) { barea = a; bd = d; }
		}
	}
	if (bd != -1) return dirs[bd]; }
	/* P6: max flood area among safe moves */
	{ int bd = -1, ba = -1;
	for (int d = 0; d < 4; d++) {
		if (rev(d)) continue;
		int nr = hx + dr[d], nc = hy + dc[d];
		if (nr < 0 || nr >= 20 || nc < 0 || nc >= 20) continue;
		if (map[nr][nc] == WALL || map[nr][nc] == OBSTACLE) continue;
		if (!safeMove(ps, dirs[d])) continue;
		bool gw = (nr == fx && nc == fy) || (ps->stepCount + 1 == ps->growthN);
		int a = flood(ps, nr, nc, gw);
		if (a > ba) { ba = a; bd = d; }
	}
	if (bd != -1) return dirs[bd]; }
	/* P7: any non-collision direction */
	for (int d = 0; d < 4; d++) {
		if (rev(d)) continue;
		int nr = hx + dr[d], nc = hy + dc[d];
		if (nr < 0 || nr >= 20 || nc < 0 || nc >= 20) continue;
		if (map[nr][nc] == WALL || map[nr][nc] == OBSTACLE) continue;
		bool gw = (nr == fx && nc == fy) || (ps->stepCount + 1 == ps->growthN);
		pSN c = ps->_pSnake; bool hit = false;
		while (c) { if (!gw && !c->next_node) break; if (nr == c->pos.x && nc == c->pos.y) { hit = true; break; } c = c->next_node; }
		if (!hit) return dirs[d];
	}
	return ps->_dir;
}
