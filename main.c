/**
 * ASCII packman game, have fun!
 * @author Max Luo
 * @version Oct 14, 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/**
 * Prints corresonding color of ghost # param.
 */
void print_color (int i) {
    if (i == 0) {
        printf(RED);
    } else if (i == 1) {
        printf(CYAN);
    } else if (i == 2) {
        printf(MAGENTA);
    } else if (i == 3) {
        printf(GREEN);
    }
}

/**
 * Inital board, no moving entities, * = wall.
 */
char grid[21][21]= {" ******************* ",
                    " *........*........* ",
                    " *o**.***.*.***.**o* ",
                    " *.................* ",
                    " *.**.*.*****.*.**.* ",
                    " *....*...*...*....* ",
                    " ****.*** * ***.**** ",
                    "    *.*       *.*    ",
                    "*****.* ***** *.*****",
                    "     .  *   *  .     ",
                    "*****.* ***** *.*****",
                    "    *.*       *.*    ",
                    " ****.* ***** *.**** ",
                    " *........*........* ",
                    " *.**.***.*.***.**.* ",
                    " *o.*..... .....*.o* ",
                    " **.*.*.*****.*.*.** ",
                    " *....*...*...*....* ",
                    " *.******.*.******.* ",
                    " *.................* ",
                    " ******************* "};

/**
 * No newline required for getchar().
 */
void disableBufferedInput() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);            // Get current terminal settings
    t.c_lflag &= ~ICANON;                   // Disable canonical mode
    t.c_lflag &= ~ECHO;                     // Disable echo
    tcsetattr(STDIN_FILENO, TCSANOW, &t);   // Apply the settings
}

/**
 * Ghost struct. Coords, color, facing left/right/killed, current movement direction.
 */
typedef struct ghost_s {
    int y;
    int x;
    int color;
    int face;
    int direction;
} ghost;

/**
 * toString for ghost struct.
 */
void gstr(ghost g) {
    print_color(g.color);
    printf("y:%d x:%d face:%d dir:%d" RESET, g.y, g.x, g.face, g.direction);
}

/**
 * Point gui struct to show +n points on ghost kill.
 */
typedef struct point_s {
    int y;
    int x;
    int amount;
} p_gui;

/**
 * Budget graphics engine.
 */
void print_board(int x, int y, int face, int score, ghost ghosts[], int power, p_gui pui){
    for (int i = 0; i < 25; i++) printf("\n"); // new frame of animation

    printf("SCORE: %d\n", score);
    for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 21; j++) {
            // +n points for eating ghost
            if (pui.amount != 0 && i == pui.y - 1 && j == pui.x - 1) {
                printf("+%d", pui.amount);
                j++;
                continue;
            }

            // print ghost with direction and color/scared white
            bool ahh = false;
            for (int k = 0; k < 4; k++) {
                ghost g = ghosts[k];                
                if (g.y == i && g.x == j) {
                    if (power == 0) {
                        print_color(g.color);
                    }
                    if (g.face == 0) {
                        printf("&~");
                    } else if (g.face == 1) {
                        printf("~&");
                    } else {
                        printf("XX");
                    }
                    printf(RESET);
                    ahh = true;
                    break;
                }
            }
            if (ahh) continue;

            // print player with direction
            if (i == y && j == x) {
                printf(YELLOW);
                if (face == 0) {
                    printf(" o");
                } else if (face == 1) {
                    printf(" v");
                } else if (face == 2) {
                    printf(" >");
                } else if (face == 3) {
                    printf(" ^");
                } else if (face == 4) {
                    printf(" <");
                }
                printf(RESET);
                continue;
            }

            // print board walls and consumables
            char c = grid[i][j];
            if (c == '*') {
                printf(BLUE "[]" RESET);
            } else if (c == '.') {
                printf(YELLOW " ." RESET);
            } else if (c == 'o') {
                printf(YELLOW " *" RESET);
            } else if (c == ' ') {
                printf("  ");
            }
        }
        printf("\n");
    }
};

/**
 * Validates if input is W,A,S,D, or 0.
 */
bool input_check(char c) {
    if (c != 'w' && c != 'a' && c != 's' && c != 'd' && c != '0') {
        return false;
    }
    return true;
};

/**
 * Checks if movement is valid and doesn't hit a wall.
 */
bool move_check(char c, int x, int y) {
    if (c == 'w') {
        return grid[y-1][x] != '*';
    }
    if (c == 'a') {
        return grid[y][MAX(0, x-1)] != '*';
    }
    if (c == 's') {
        return grid[y+1][x] != '*';
    }
    if (c == 'd') {
        return grid[y][MIN(20, x+1)] != '*';
    }
    return true;
};

/**
 * Eats pellet and scores.
 */
void eat_pellet(int x, int y, int *score) {
    if (grid[y][x] == '.') {
        grid[y][x] = ' ';
        *score += 10;
    }
};

/**
 * Eats boost and scores.
 */
bool eat_boost(int x, int y, int *score) {
    if (grid[y][x] == 'o') {
        grid[y][x] = ' ';
        *score += 50;
        return true;
    }
    return false;
};

/**
 * Checks if x-y is occupied by a ghost.
 */
int contact_ghost(int x, int y, ghost ghosts[]) {
    for (int i = 0; i < 4; i++) {
        ghost g = ghosts[i];
        if (x == g.x && y == g.y) {
            return i;
        }
    }
    return -1;
};

/**
 * Checks if ghost is in the ghost box.
 */
bool in_box(ghost g) {
    return g.y == 9 && g.x > 8 && g.x < 12;
}

/**
 * Ghost movement method. Scared ghosts move 1/2 speed. Ghosts in box dont move.
 * Ghosts without intersection move forward or turn clockwise or turn counter.
 * Ghosts with intersection pick a random turn thats not backwards. TODO: BFS?
 */
void move_ghosts(int x, int y, ghost ghosts[], int power) {
    if (power % 2 == 1) return;

    int dirs[4][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    for (int i = 0; i < 4; i++) {
        ghost *g = &ghosts[i];
        if (in_box(*g) == true) {
            continue;
        }
        
        int ways[4] = {0, 0, 0 ,0};
        int num_ways = 0;

        for (int d = 0; d < 4; d++) { // count number and direction of paths
            int yy = g->y + dirs[d][0];
            int xx = g->x + dirs[d][1];
            if (xx < 0 || xx > 20 || grid[yy][xx] != '*') {
                ways[d] = 1;
                num_ways++;
            }
        }

        if (num_ways < 3) { // if not at intersection
            int cw = (g->direction + 3) % 4;
            int ccw = (g->direction + 1) % 4;
            
            if (ways[g->direction] == 1) { // continue in direction
                g->x += dirs[g->direction][1];
                g->y += dirs[g->direction][0];
                if (g->x < 0) g->x = 20;
                if (g->x > 20) g->x = 0;
            } else if (ways[cw] == 1) { // look clockwise
                g->x += dirs[cw][1];
                g->y += dirs[cw][0];
                g->direction = cw;
            } else if (ways[ccw] == 1) { // look counterclockwise
                g->x += dirs[ccw][1];
                g->y += dirs[ccw][0];
                g->direction = ccw;
            }
        } else { // if at an intersection go random
            while (true) {
                int go = rand() % 4;
                if (go == (g->direction + 2) % 4) continue; // dont go backwards
                if (ways[go] == 1) {
                    g->x += dirs[go][1];
                    g->y += dirs[go][0];
                    g->direction = go;
                    break;
                }
            }
        }

        if (g->direction == 1) g->face = 0;
        if (g->direction == 3) g->face = 1;
    }
};

/**
 * Returns killed ghost to ghost box or outside if full.
 */
void boxify(ghost ghosts[], int idx) {
    ghost *move = &ghosts[idx];
    for (int i = 0; i < 3; i++) {
        bool occ = false;
        for (int j = 0; j < 4; j++) {
            ghost g = ghosts[j];
            if (g.x == i + 9 && g.y == 9) {
                occ = true;
                break;
            }
        }
        if (occ == false) {
            move->x = 9 + i;
            move->y = 9;
            move->face = 0;
            move->direction = 1;
            return;
        }
    }
    move->x = 10;
    move->y = 7;
    move->face = 0;
    move->direction = 1;
}

int main() {
    srand(time(0));
    disableBufferedInput();

    printf("\nPAC MAN!\n");
    printf("Move with WASD | 0 to quit\n\n");

    printf("[enter] to play\n");
    char input = '0';
    scanf("%c", &input);

    ghost ghosts[4];
    ghost g1 = {7, 10, 0, 0, 1};
    ghosts[0] = g1;
    ghost g2 = {9, 9, 1, 0, 1};
    ghosts[1] = g2;
    ghost g3 = {9, 10, 2, 0, 1};
    ghosts[2] = g3;
    ghost g4 = {9, 11, 3, 0, 1};
    ghosts[3] = g4;

    int face = 0; // 1-up-W, 2-left-A, 3-down-S, 4-left-D
    int y = 15;
    int x = 10;
    int score = 0;
    int power = 0;
    int power_pts = 200; 

    int counter = 1; // gameclock for deploying ghosts
    p_gui pui = {0, 0, 0}; // +n point gui when eating ghosts

    while (true) {
        print_board(x, y, face, score, ghosts, power, pui);
        pui.amount = 0; 

        // get input and move
        input = getchar();
        if (input_check(input) == false) {
            continue;
        }
        if (input == '0') {
            printf("\nBYE!\n\n");
            return 1;
        }
        if (move_check(input, x, y) == false) {
            continue;
        }
        if (input == 'w') {
            face = 1;
            y--;
        } else if (input == 'a') {
            face = 2;
            x--;
            if (x < 0) x = 20;
        } else if (input == 's') {
            face = 3; 
            y++;
        } else if (input == 'd') {
            face = 4;
            x++;
            if (x > 20) x = 0;
        }

        // eat pellets and boosts
        eat_pellet(x, y, &score);
        if (eat_boost(x, y, &score)) {
            power = 30;
        }       

        // check for ghost collision before and after they move
        for (int ii = 0; ii < 2; ii++) {
            int contact = contact_ghost(x, y, ghosts);
            if (contact != -1) {
                if (power == 0) {
                    ghosts[contact].face = 2;
                    print_board(x, y, face, score, ghosts, power, pui);
                    printf("\nGAME OVER!\n\n");
                    return 1;
                } else {
                    pui.x = x;
                    pui.y = y;
                    pui.amount = power_pts;
                    score += power_pts;
                    power_pts *= 2;
                    counter = 1;
                    boxify(ghosts, contact);
                }
            }

            if (ii == 0) move_ghosts(x, y, ghosts, power);
        }

        // deploy ghosts
        if (counter % 20 == 0) {
            for (int i = 0; i < 4; i++) {
                if (in_box(ghosts[i])) {
                    ghosts[i].y = 7;
                    break;
                }
            }
        }

        // decrease power
        power = MAX(0, power - 1);
        if (power == 0) {
            power_pts = 200;
        }
        counter++;
    }
}