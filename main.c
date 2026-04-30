/*
 * MIT License
 *
 * Copyright (c) 2026 Lorenzo Pegorari (@LorenzoPegorari)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/** @file main.c */


/* C89 standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* libncurses */
#include <ncurses.h>


#define ALIVE 1
#define DEAD  0

#define TIME_OUT_DEFAULT 100

#define MODE_NULL  0
#define MODE_CHAR  1
#define MODE_BW    2
#define MODE_COLOR 3

#define MODE_CHAR_SYMBOL '*'

#define CTRL_KEY(k) ((k) & 0x1f)

/* Two-step macro (with extra level of indirection) to allow the preprocessor
   to expand the macros before they are converted to strings */
#define CGOL_STR_HELPER(x) #x
#define CGOL_STR(x) CGOL_STR_HELPER(x)

#define CGOL_VER_MAJOR 1
#define CGOL_VER_MINOR 0
#define CGOL_VER_PATCH 0

#define CGOL_VER CGOL_STR(CGOL_VER_MAJOR) "." CGOL_STR(CGOL_VER_MINOR) "." CGOL_STR(CGOL_VER_PATCH)


typedef struct cell_tag {
    int state_old;
    int state;
    int living_neighbors;
} cell_t;


static int x_ssum(int x1, int x2);
static int y_ssum(int y1, int y2);
static int count_living_neighbors(cell_t **grid, int x, int y);
static void update_grid(cell_t **grid);
static void compute_next_frame(cell_t **grid);
static int draw_frame(cell_t **grid, int mode);


int main(int argc, char* argv[]) {
    cell_t **grid;
    int i, x, y;
    int ch;
    int err;
    int timeout_val;
    int mode;


    timeout_val = TIME_OUT_DEFAULT;
    mode = MODE_NULL;
    /* Handle arguments */
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            fprintf(stdout, "Usage: %s [-v | --version] [-h | --help] <file-path>\n", argv[0]);
            fprintf(stdout, "\nOptions:\n");
            fprintf(stdout, "  -h               = print this info\n");
            fprintf(stdout, "  --help\n");
            fprintf(stdout, "  -v               = print application's version\n");
            fprintf(stdout, "  --version\n");
            fprintf(stdout, "  -t <uint>        = define timeout time in ms (default is '%i' ms)\n", TIME_OUT_DEFAULT);
            fprintf(stdout, "  --timeout <uint>\n");
            fprintf(stdout, "  --char           = uses char '%c' for alive cells\n", MODE_CHAR_SYMBOL);
            fprintf(stdout, "                     this is the default mode for terminals without colors\n");
            fprintf(stdout, "  --bw             = uses color white for alive cells\n");
            fprintf(stdout, "  --color          = uses colors for alive cells (depending on alive neighbors)\n");
            fprintf(stdout, "                     this is the default mode for terminals with colors\n");
            fprintf(stdout, "\nUsable commands:\n");
            fprintf(stdout, "  W | UP    = move up one cell\n");
            fprintf(stdout, "  S | DOWN  = move down one cell\n");
            fprintf(stdout, "  A | LEFT  = move left one cell\n");
            fprintf(stdout, "  D | RIGHT = move right one cell\n");
            fprintf(stdout, "  E         = turn on/off selected cell\n");
            fprintf(stdout, "  ENTER     = start game\n");
            fprintf(stdout, "  CTRL+Q    = quit\n");
            exit(EXIT_SUCCESS);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            fprintf(stdout, "%s version %s\n", argv[0], CGOL_VER);
            exit(EXIT_SUCCESS);
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
            ++i;
            if (i >= argc) {
                fprintf(stderr, "Value of argument '%s' is missing!\n", argv[i - 1]);
                exit(EXIT_FAILURE);
            }
            timeout_val = atoi(argv[i]);
            if (timeout_val < 0) {
                fprintf(stderr, "Value '%i' of argument '%s' can't be negative!\n", timeout_val, argv[i - 1]);
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "--char") == 0) {
            if (mode != MODE_NULL) {
                fprintf(stderr, "Options '--char', '--bw' and '--color' are incompatible! Choose only one!\n");
                exit(EXIT_FAILURE);
            }
            mode = MODE_CHAR;
        } else if (strcmp(argv[i], "--bw") == 0) {
            if (mode != MODE_NULL) {
                fprintf(stderr, "Options '--char', '--bw' and '--color' are incompatible! Choose only one!\n");
                exit(EXIT_FAILURE);
            }
            mode = MODE_BW;
        } else if (strcmp(argv[i], "--color") == 0) {
            if (mode != MODE_NULL) {
                fprintf(stderr, "Options '--char', '--bw' and '--color' are incompatible! Choose only one!\n");
                exit(EXIT_FAILURE);
            }
            mode = MODE_COLOR;
        } else {
            fprintf(stderr, "Unrecognized argument '%s'!\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }


    /* Initialize term in "curses mode".
       If errors occur, initscr() writes an appropriate error message to
       standard error and exits. If successful, returns a pointer to stdscr. */
    (void)initscr();

    /* Check if terminal supports colors, and if so enable them */
    if (has_colors() == FALSE) {
        switch (mode) {
            case MODE_CHAR:
                break;

            case MODE_NULL:
                mode = MODE_CHAR;
                break;

            case MODE_BW:
            case MODE_COLOR:
                endwin();
                fprintf(stderr, "Your terminal does not support colors!\n");
                exit(EXIT_FAILURE);
        }
    } else {
        switch (mode) {
            case MODE_CHAR:
                break;

            case MODE_NULL:
                mode = MODE_COLOR;
                __attribute__ ((fallthrough));  /* GCC specific: signal explicit fallthrough */

            case MODE_BW:
            case MODE_COLOR:
                if (start_color() == ERR) {
                    endwin();
                    fprintf(stderr, "Error when enabling colors!\n");
                    exit(EXIT_FAILURE);
                }
                err = 0;
                err |= init_pair(1, COLOR_BLACK, COLOR_BLUE);
                err |= init_pair(2, COLOR_BLACK, COLOR_CYAN);
                err |= init_pair(3, COLOR_BLACK, COLOR_GREEN);
                err |= init_pair(4, COLOR_BLACK, COLOR_YELLOW);
                err |= init_pair(5, COLOR_BLACK, COLOR_RED);
                err |= init_pair(6, COLOR_BLACK, COLOR_MAGENTA);
                err |= init_pair(7, COLOR_BLACK, COLOR_WHITE);
                err |= init_pair(8, COLOR_BLACK, COLOR_WHITE);
                err |= init_pair(9, COLOR_BLACK, COLOR_WHITE);
                if (err == ERR) {
                    endwin();
                    fprintf(stderr, "Error when setting color pairs!\n");
                    exit(EXIT_FAILURE);
                }
                break;
        }
    }

    /* Set timeout for all `ncurses` input function blocks.
       After `delay` milliseconds the function fails if there is still no input. */
    timeout(timeout_val);

    /* Set terminal to "raw mode".
       In "raw mode", control characters like suspend (CTRL-Z), interrupt and
       quit (CTRL-C) are directly passed to the program without generating a
       signal.
       In "cbreak mode", control characters like suspend (CTRL-Z), interrupt
       and quit (CTRL-C) are interpreted as any other character by the terminal
       driver. */
    if (raw() == ERR) {
        endwin();
        fprintf(stderr, "Error when setting 'raw mode'!\n");
        exit(EXIT_FAILURE);
    }

    /* Disable "Echo mode" */
    if (noecho() == ERR) {
        endwin();
        fprintf(stderr, "Error when setting 'noecho mode'!\n");
        exit(EXIT_FAILURE);
    }

    /* Translate "Carriage Return" to "Newline" */
    if (nl() == ERR) {
        endwin();
        fprintf(stderr, "Error when setting Carriage Return translation to Newline!\n");
        exit(EXIT_FAILURE);
    }

    /* Enable the reading of function keys like F1, F2, arrow keys, etc. */
    if (keypad(stdscr, TRUE) == ERR) {
        endwin();
        fprintf(stderr, "Error when enabling ability to read function keys!\n");
        exit(EXIT_FAILURE);
    }


    /* Allocate grid memory */
    grid = (cell_t **)calloc(COLS, sizeof(cell_t *));
    for (x = 0; x < COLS; ++x) {
        grid[x] = (cell_t *)calloc(LINES, sizeof(cell_t));
        for (y = 0; y < LINES; ++y) {
            grid[x][y].state_old = DEAD;
            grid[x][y].state = DEAD;
            grid[x][y].living_neighbors = 0;
        }
    }


    /* Ask user for the initial positions */
    x = COLS / 2;
    y = LINES / 2;
    do {
        update_grid(grid);
        if (draw_frame(grid, mode)) {
            endwin();
            fprintf(stderr, "Error when drawing next game frame!\n");
            for (x = 0; x < COLS; ++x)
                free(grid[x]);
            free(grid);
            exit(EXIT_FAILURE);
        }

        /* Move cursor to (x,y) */
        if (move(y, x) == ERR) {
            endwin();
            fprintf(stderr, "Error when moving cursor to (x=%d,y=%d)!\n", x, y);
            for (x = 0; x < COLS; ++x)
                free(grid[x]);
            free(grid);
            exit(EXIT_FAILURE);
        }

        /* Handle keypresses */
        ch = getch();
        switch (ch) {
            case KEY_LEFT:
            case 'a':
            case 'A':
                x = x_ssum(x, -1);
                break;

            case KEY_RIGHT:
            case 'd':
            case 'D':
                x = x_ssum(x, 1);
                break;

            case KEY_UP:
            case 'w':
            case 'W':
                y = y_ssum(y, -1);
                break;

            case KEY_DOWN:
            case 's':
            case 'S':
                y = y_ssum(y, 1);
                break;

            case 'e':
            case 'E':
                grid[x][y].state = (grid[x][y].state == DEAD) ? ALIVE : DEAD;
                break;

            case CTRL_KEY('q'):
                endwin();
                for (x = 0; x < COLS; ++x)
                    free(grid[x]);
                free(grid);
                exit(EXIT_SUCCESS);
        }
    } while (ch != '\n' && ch != KEY_ENTER);

    /* Try to hide cursor before game begins.
       If the terminal doesn't allow the cursor to become invisible, ERR will
       be returned. In this case just keep the application going with the
       cursor showing. */
   curs_set(0);

    /* Start game */
    do {
        compute_next_frame(grid);
        update_grid(grid);
        if (draw_frame(grid, mode)) {
            endwin();
            fprintf(stderr, "Error when drawing next game frame!\n");
            for (x = 0; x < COLS; ++x)
                free(grid[x]);
            free(grid);
            exit(EXIT_FAILURE);
        }
    } while (getch() != CTRL_KEY('q'));


    /* Release allocate memory for `grid` */
    for (x = 0; x < COLS; ++x)
        free(grid[x]);
    free(grid);

    /* Exit terminal "curses mode" */
    if (endwin() == ERR) {
        fprintf(stderr, "Error when exiting 'curses mode'!\n");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}


/**
 * Safe sum x coordinates.
 * Will wrap back if the sum of the x coordinates goes out of range.
 */
static int x_ssum(int x1, int x2) {
    int ret;

    ret = x1 + x2;
    if (ret >= COLS)
        ret %= COLS;
    else if (ret < 0) {
        ret %= COLS;
        ret += COLS;
    }

    return ret;
}


/**
 * Safe sum y coordinates.
 * Will wrap back if the sum of the y coordinates goes out of range.
 */
static int y_ssum(int y1, int y2) {
    int ret;

    ret = y1 + y2;
    if (ret >= LINES)
        ret %= LINES;
    else if (ret < 0){
        ret %= LINES;
        ret += LINES;
    }

    return ret;
}


/**
 * Return the number of living neighbors of the cell inside `grid` at the given
 * coordinates `x` and `y`.
 */
int count_living_neighbors(cell_t **grid, int x, int y) {
    int xo, yo, sum;

    sum = 0;
    for (xo = -1; xo <= 1; ++xo) {
        for (yo = -1; yo <= 1; ++yo) {
            if (xo == 0 && yo == 0)
                continue;
            if (grid[x_ssum(x, xo)][y_ssum(y, yo)].state == ALIVE)
                ++sum;
        }
    }

    return sum;
}


/**
 * Update the given `grid`.
 * All cells `.state_old` and `.living_neighbors` will be updated based on
 * their `.state`.
 */
static void update_grid(cell_t **grid) {
    int x, y;

    for (x = 0; x < COLS; ++x) {
        for (y = 0; y < LINES; ++y) {
            grid[x][y].living_neighbors = count_living_neighbors(grid, x, y);
            grid[x][y].state_old = grid[x][y].state;
        }
    }
}


/**
 * Compute new `.state` values of all cells inside the given `grid`.
 * This essentially means that the next frame of the game will be calculated.
 */
static void compute_next_frame(cell_t **grid) {
    int x, y;
    cell_t *cell;

    for (x = 0; x < COLS; ++x) {
        for (y = 0; y < LINES; ++y) {
            cell = &grid[x][y];
            /* Birth */
            if (cell->state_old == DEAD && cell->living_neighbors == 3)
                cell->state = ALIVE;
            /* Survival */
            else if (cell->state_old == ALIVE &&
                     (cell->living_neighbors == 2 || cell->living_neighbors == 3))
                cell->state = ALIVE;
            /* Overcrowding / Loneliness */
            else
                cell->state = DEAD;
        }
    }
}


/**
 * Draw the current frame of the game.
 * The current frame is taken from the `.state` values of all cells inside the
 * given `grid`.
 */
static int draw_frame(cell_t **grid, int mode) {
    int x, y, color;

    if (mode == MODE_CHAR) {
        for (x = 0; x < COLS; ++x) {
            for (y = 0; y < LINES; ++y) {
                if (grid[x][y].state_old == ALIVE)
                    mvaddch(y, x, MODE_CHAR_SYMBOL);
                else
                    mvaddch(y, x, ' ');
            }
        }
    } else if (mode == MODE_BW) {
        for (x = 0; x < COLS; ++x) {
            for (y = 0; y < LINES; ++y) {
                if (grid[x][y].state_old == ALIVE) {
                    color = COLOR_PAIR(9);
                    attron(color);
                    mvaddch(y, x, ' ');
                    attroff(color);
                } else
                    mvaddch(y, x, ' ');
            }
        }
    } else if (mode == MODE_COLOR) {
        for (x = 0; x < COLS; ++x) {
            for (y = 0; y < LINES; ++y) {
                if (grid[x][y].state_old == ALIVE) {
                    color = COLOR_PAIR(grid[x][y].living_neighbors + 1);
                    attron(color);
                    mvaddch(y, x, ' ');
                    attroff(color);
                } else
                    mvaddch(y, x, ' ');
            }
        }
    } else
        return -1;

    if (refresh() == ERR)
        return -1;
    return 0;
}
