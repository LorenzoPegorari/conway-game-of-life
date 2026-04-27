/* C89 standard */
#include <stdlib.h>

/* libncurses */
#include <ncurses.h>


#define CTRL_KEY(k) ((k) & 0x1f)

#define ALIVE 1
#define DEAD  0

#define TIME_OUT 100


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
static void draw_frame(cell_t **grid);


int main(int argc, char* argv[]) {
    cell_t **grid;
    int i, x, y;
    int ch;
    int err;


    /* Initialize term in "curses mode".
       If errors occur, initscr() writes an appropriate error message to
       standard error and exits. If successful, returns a pointer to stdscr. */
    (void)initscr();

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

    /* Translate `Carriage Return` to `Newline` */
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

    /* Set timeout for all `ncurses` input function blocks.
       After `delay` milliseconds the function fails if there is still no input. */
    timeout(TIME_OUT);

    /* Check if terminal supports colors, and if so enable them */
    if (has_colors() == FALSE) {
        endwin();
        fprintf(stderr, "Your terminal does not support colors!\n");
        exit(EXIT_FAILURE);
    }
    if (start_color() == ERR) {
        endwin();
        fprintf(stderr, "Error when setting 'noecho mode'!\n");
        exit(EXIT_FAILURE);
    }
    err = 0;
    err |= init_pair(1, COLOR_BLACK, COLOR_BLUE);
    err |= init_pair(2, COLOR_BLACK, COLOR_CYAN);
    err |= init_pair(3, COLOR_BLACK, COLOR_GREEN);
    err |= init_pair(4, COLOR_BLACK, COLOR_MAGENTA);
    err |= init_pair(5, COLOR_BLACK, COLOR_RED);
    err |= init_pair(6, COLOR_BLACK, COLOR_YELLOW);
    err |= init_pair(7, COLOR_BLACK, COLOR_WHITE);
    err |= init_pair(8, COLOR_BLACK, COLOR_WHITE);
    err |= init_pair(9, COLOR_BLACK, COLOR_WHITE);
    if (err == ERR) {
        endwin();
        fprintf(stderr, "Error when setting color pairs!\n");
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


    x = COLS / 2;
    y = LINES / 2;
    do {
        update_grid(grid);
        draw_frame(grid);
        move(y, x);

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
        }
    } while (ch != '\n' && ch != KEY_ENTER);

    curs_set(0);

    do {
        compute_next_frame(grid);
        update_grid(grid);
        draw_frame(grid);
        ch = getch();
    } while (ch != CTRL_KEY('q'));

    endwin();

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

    refresh();
}


/**
 * Draw the current frame of the game.
 * The current frame is taken from the `.state` values of all cells inside the
 * given `grid`.
 */
static void draw_frame(cell_t **grid) {
    int x, y, color;

    for (x = 0; x < COLS; ++x) {
        for (y = 0; y < LINES; ++y) {
            if (grid[x][y].state_old == ALIVE) {
                color = COLOR_PAIR(count_living_neighbors(grid, x, y) + 1);
                attron(color);
                mvaddch(y, x, ' ');
                attroff(color);
            } else {
                mvaddch(y, x, ' ');
            }
        }
    }

    refresh();
}
