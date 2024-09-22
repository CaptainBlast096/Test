#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
    #include <linux/random.h>
    #include <linux/slab.h>

// Module metadata
MODULE_AUTHOR("Jaleel Rogers");
MODULE_DESCRIPTION("Maze Generator using Prim's Algorithm");
MODULE_LICENSE("GPL");

#define MAX_ROWS 12
#define MAX_COLS 15
#define MAX_FRONTIER 1000  // Maximum number of frontier cells we can store
#define PROC_NAME "raptor_maze"
#define BUF_LEN 2048 // Buffer length for the maze

typedef struct {
    int x, y;
} Cell;

static char *maze_buffer;
static struct proc_dir_entry* proc_entry;

/*Name: Jaleel Rogers
Date: 09/22/2024
Description: Implementation of the read system call
*/
static ssize_t custom_read(struct file* file, char __user* user_buffer, size_t count, loff_t* offset)
{
    printk(KERN_INFO "calling our very own custom read method."); 
    
    char greeting[] = "Hello world!\n";
    int greeting_length = strlen(greeting); 
 
    if (*offset > 0)
    return 0; 

    copy_to_user(user_buffer, greeting, greeting_length);
    *offset = greeting_length; 
    
    return greeting_length;
}

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .read = custom_read
};

/*Name: Jaleel Rogers
Date: 09/20/2024
Description: Prints the maze to the buzzer. 
    Copies each character and sends it to the buffer.
*/
static void print_grid(char grid[][MAX_COLS], int rows, int cols) {
    int pos = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (pos < BUF_LEN - 1) {
                maze_buffer[pos++] = grid[i][j];
            }
        }
        if (pos < BUF_LEN - 1) {
            maze_buffer[pos++] = '\n';
        }
    }
    maze_buffer[pos] = '\0';
}

/*Name: Jaleel Rogers
Date: 09/20/2024
Description: Creates a grid filled with #
*/
static void make_grid(char grid[][MAX_COLS], int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            grid[i][j] = '#';
        }
    }
}

/*Name: Jaleel Rogers
Date: 09/20/2024
Description: Determines if a cell is within bounds
*/
static int is_within_bounds(int x, int y, int rows, int cols) {
    return (x >= 0 && x < rows && y >= 0 && y < cols);
}

/*Name: Jaleel Rogers
Date: 09/20/2024
Description: Creates a space within the maze representing the first cell set to passage
*/
static void random_cell(char grid[][MAX_COLS], int rows, int cols, int *x, int *y) {
    int random_value;
    get_random_bytes(&random_value, sizeof(random_value));
    *x = abs(random_value) % rows;  // Random row
    get_random_bytes(&random_value, sizeof(random_value));
    *y = abs(random_value) % cols;  // Random column
    grid[*x][*y] = ' ';  // Set random cell as passage
}

/*Name: Jaleel Rogers
Date: 09/20/2024
Description: Finds a cell 2 positions either in left, right, up, and down positions
*/
static void add_frontier_cells(char grid[][MAX_COLS], Cell frontier[], int *frontier_count, int x, int y, int rows, int cols) {
    int directions[4][2] = { {-2, 0}, {2, 0}, {0, -2}, {0, 2} };

    for (int i = 0; i < 4; i++) {
        int nx = x + directions[i][0];
        int ny = y + directions[i][1];

        if (is_within_bounds(nx, ny, rows, cols) && grid[nx][ny] == '#') {
            frontier[*frontier_count].x = nx;
            frontier[*frontier_count].y = ny;
            (*frontier_count)++;
        }
    }
}

/*Name: Jaleel Rogers
Date: 09/21/2024
Description: Connects the frontier cell to the neighbor cell with both set to passage
*/
static void connect_to_passage(char grid[][MAX_COLS], int x, int y, int rows, int cols) {
    int directions[4][2] = { {-2, 0}, {2, 0}, {0, -2}, {0, 2} };

    for (int i = 0; i < 4; i++) {
        int nx = x + directions[i][0];
        int ny = y + directions[i][1];

        if (is_within_bounds(nx, ny, rows, cols) && grid[nx][ny] == ' ') {
            int mid_x = (x + nx) / 2;
            int mid_y = (y + ny) / 2;
            grid[mid_x][mid_y] = ' ';
            grid[x][y] = ' ';
            break;
        }
    }
}

/*Name: Jaleel Rogers
Date: 09/21/2024
Description: Generates the maze using the functions for Prim's algorithm
*/
static void generate_maze(void) {
    int rows = MAX_ROWS;
    int cols = MAX_COLS;
    char grid[MAX_ROWS][MAX_COLS];
    Cell frontier[MAX_FRONTIER];
    int frontier_count = 0;

    make_grid(grid, rows, cols);

    int start_x, start_y;
    random_cell(grid, rows, cols, &start_x, &start_y);

    add_frontier_cells(grid, frontier, &frontier_count, start_x, start_y, rows, cols);

    while (frontier_count > 0) {
        int random_value;
        get_random_bytes(&random_value, sizeof(random_value));
        int rand_index = abs(random_value) % frontier_count;
        int fx = frontier[rand_index].x;
        int fy = frontier[rand_index].y;

        connect_to_passage(grid, fx, fy, rows, cols);

        add_frontier_cells(grid, frontier, &frontier_count, fx, fy, rows, cols);

        frontier[rand_index] = frontier[--frontier_count];
    }

    print_grid(grid, rows, cols);
}

/*Name: Jaleel Rogers
Date: 09/21/2024
Description: Proc read function
*/
ssize_t maze_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
    return simple_read_from_buffer(buf, count, pos, maze_buffer, strlen(maze_buffer));
}

static const struct proc_ops proc_file_ops = {
    .proc_read = maze_read,
};

/*Name: Jaleel Rogers
Date: 09/21/2024
Description: Calls to generate maze
*/
static int __init maze_init(void) {
    maze_buffer = kmalloc(BUF_LEN, GFP_KERNEL);
    if (!maze_buffer) return -ENOMEM;

    proc_entry = proc_create(PROC_NAME, 0666, NULL, &fops);
    
    if (!maze_proc) {
        kfree(maze_buffer);
        return -ENOMEM;
    }

    generate_maze();
    printk(KERN_INFO "Maze module loaded.\n");
    return 0;
}

/*Name: Jaleel Rogers
Date: 09/20/2024
Description: Frees to memory and unloads module
*/
static void __exit maze_exit(void) {
    proc_remove(proc_entry);
    kfree(maze_buffer);
    printk(KERN_INFO "Maze module unloaded.\n");
}

module_init(maze_init);
module_exit(maze_exit);
