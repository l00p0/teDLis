#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"

/*****
 *
 * TODOS
 * 
 *   - level up, increase speed, 
 *   - do some animation when lines complete, when moving, etc
 *   - sound
 *   - press space to drop to bottom
 *   - clean up code, remove globals to allow for multiplayer
 *
 * */

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 640;
enum { HEIGHT = 25, WIDTH = 10};
const int CELL_DIM = 25;



//BOARD BASE POSITION in pixels
const int B_X = (SCREEN_WIDTH - CELL_DIM * WIDTH) / 2;
const int B_Y = (SCREEN_HEIGHT - CELL_DIM * HEIGHT) / 2;

float CURRENT_STEP_TIMEOUT = 1;
float CURRENT_STEP_LEFT = 0;
int   SCORE = 0;
int   LEVEL = 1;
int   TOTAL_LINES = 0;
char  GAME_OVER = 0;

int PIECE_BUFFER[3];
int BOARD [WIDTH][HEIGHT];
//int BOARD [10][25];

int TETROMINOS[7][2][4] = {
    {
     {1,1,1,1},
     {0,0,0,0}
    },
    {
     {0,2,0,0},
     {2,2,2,0} 
    },
    {
     {0,3,3,0},
     {3,3,0,0} 
    },
    {
     {4,4,0,0},
     {0,4,4,0}
    },
    {
     {0,0,5,0},
     {5,5,5,0}
    },
    {
     {6,0,0,0},
     {6,6,6,0}
    },
    {
     {7,7,0,0},
     {7,7,0,0}
    }
};

int CURRENT_PIECE[4][4] = {
    {0,1,0,0},
    {0,1,0,0},
    {1,1,0,0},
    {0,0,0,0}
};

int BLOCK_X = 5;
int BLOCK_Y = 0;
int BLOCK_X_SIZE=0;
int BLOCK_Y_SIZE=0;


int draw_cell_px(int x, int y, int cell_dim, int contents){
    const Color colors[] = {RED,GOLD,ORANGE,LIME,BROWN,DARKBLUE,RAYWHITE};

    int idx = contents -1 % 7;
    DrawRectangle(x, y, cell_dim-1, cell_dim-1, colors[idx]);

}

int draw_cell(int x, int y, int contents){
 
    draw_cell_px(B_X + x * CELL_DIM, B_Y + y * CELL_DIM, CELL_DIM, contents);

}

int draw_score(){

    char scorebuff[1024];
    sprintf(scorebuff,"Score: %d",SCORE);
    DrawText(scorebuff,10,10,20,RAYWHITE);
    sprintf(scorebuff,"Level: %d",LEVEL);
    DrawText(scorebuff,10,45,20,RAYWHITE);
    return 0;
}


int draw_board(){
    int c_x = SCREEN_WIDTH / 2;
    int c_y = SCREEN_HEIGHT / 2;

    //draw surround
    int bx,by;
    bx = c_x - (CELL_DIM * WIDTH) / 2;
    by = c_y - (CELL_DIM * HEIGHT) / 2;
    DrawRectangle(bx,by,CELL_DIM * WIDTH, CELL_DIM * HEIGHT,BLUE);

    //draw contents
    int x,y;
    for(x=0;x<WIDTH;x++){
        for(y=0; y<HEIGHT; y++){
            if(BOARD[x][y] != 0){
                draw_cell(x,y,BOARD[x][y]);
            };
        }
    }
    return 0;
}

int draw_preview(int x, int y, int tet_index, int cell_dim){
    int row,col;
    for(col=0;col<4;col++){
        for(row=0;row<2;row++){
            if(TETROMINOS[tet_index][row][col] != 0){
                draw_cell_px(x+col*cell_dim, y+row*cell_dim, cell_dim, TETROMINOS[tet_index][row][col]);
            }
        }
    }


}

int draw_preview_pieces(){
    int x = (SCREEN_WIDTH + WIDTH * CELL_DIM) / 2 + 50;
    int y = 10;
    int preview_cell_dim = 10;
    draw_preview(x, y, PIECE_BUFFER[0], preview_cell_dim);
    y += preview_cell_dim * 4;
    draw_preview(x, y, PIECE_BUFFER[1], preview_cell_dim);

}

int rotate_block(int result[4][4], int rotation){
    int swap[4][4];

    int x,y,d;
    //clear swap space
    for(x=0;x<4;x++){
        for(y=0;y<4;y++){
            swap[y][x] = 0;
        }
    }
   
    //rotate around diagonal
    for(x=0;x<4;x++){
        for(y=0;y<4;y++){
            swap[y][x] = result[x][y];
        }
    }

    //get middle x 
    int max_x=0;
    for(x=0;x<4;x++){
        for(y=0;y<4;y++){
            if(swap[x][y] >0){
                if(max_x < x){
                    max_x = x;
                }
            }
        }
    }
    //clear result before writing
    for(x=0;x<4;x++){
        for(y=0;y<4;y++){
            result[x][y]=0;
        }
    }

    //flip about half of max_x
    for(y=0;y<4;y++){
        for(d=0;d<=(max_x);d++){
            result[d][y] = swap[max_x-d][y];
        }
    }

    //swap x and y extents;
    int temp = BLOCK_X_SIZE;
    BLOCK_X_SIZE=BLOCK_Y_SIZE;
    BLOCK_Y_SIZE=temp;
    //check if we're out of bounds and move
    if(BLOCK_X +BLOCK_X_SIZE > WIDTH){
        BLOCK_X = WIDTH - BLOCK_X_SIZE;
    }
    if(BLOCK_X < 0){//should never be true
        BLOCK_X = 0;
    }
    return 0;
}


int set_current_piece(int tet_index){

    assert(tet_index < 7);
    int row,col;
    
    //clear 
    for(row = 0; row < 4; row++){
        for(col = 0; col < 4; col++){
            CURRENT_PIECE[row][col] = 0;
        }
    }
    int max_x=0;
    int max_y =0;
    for(row = 0; row < 2; row++){
        for(col = 0; col < 4; col++){
            int curval = TETROMINOS[tet_index][row][col];
            if(curval){
                if(max_x < col){
                    max_x = col;
                }
                if(max_y < row){
                    max_y = row;
                }            
                CURRENT_PIECE[row][col] = curval;
            }            
        }
    }
    
    BLOCK_X_SIZE = max_x + 1;
    BLOCK_Y_SIZE = max_y + 1;
    return 0; 
}

int new_piece(void){

    set_current_piece(PIECE_BUFFER[0]);
    PIECE_BUFFER[0] = PIECE_BUFFER[1];
    PIECE_BUFFER[1] = PIECE_BUFFER[2];
    PIECE_BUFFER[2] = (int) rand() % 7;
    BLOCK_X = 5;
    BLOCK_Y = 0;
}

int initialise(void){
    int x,y;
    for(x=0;x<WIDTH;x++){
        for(y=0; y<HEIGHT; y++){
            BOARD[x][y] = 0;
        }
    }
    CURRENT_STEP_LEFT = CURRENT_STEP_TIMEOUT;
    srand(time(0));
    new_piece();new_piece();new_piece();//fill up the piece buffer
    return 0;
} 

int cell_full(int x, int y){
    
    if(y >= HEIGHT){
//        printf("reached bottom %d,%d \n",x,y);
        return 1;
    }

    if(x < 0 || x >= WIDTH){
        return 1;
    }

    if(BOARD[x][y]){
//        printf("board full @ %d,%d \n",x,y);
        return 1;
    }else{
        return 0;
    }
}

int can_strafe(int x_offset){
    //make sure that there is a space on the board for every row of the current_piece
    int x,y;
    for(x=0; x<BLOCK_X_SIZE; x++){
        for(y=0; y<BLOCK_Y_SIZE; y++){
            if(CURRENT_PIECE[y][x]){
                if(cell_full(BLOCK_X + x + x_offset, BLOCK_Y + y)){
                    return 0;
                }
            }
        }
    }
    //didn't fail == success
    return 1;
}

int can_drop(){
    int row,col;

    //foreach col in the current piece get the highest row-index

    for(col=0; col < 4; col++){
        for(row =0; row < 4; row++){
            if(CURRENT_PIECE[row][col]){
                if(cell_full(BLOCK_X + col, BLOCK_Y + row + 1)){
//                    printf("can't drop \n");
                    return 0;
                }
            }
        }
    }
//    printf("Can drop %d,%d\n",BLOCK_X,BLOCK_Y);
    return 1;
}

int piece_to_board(){
    int row,col;
    for(row=0; row < 4; row++){
        for(col=0;col<4;col++){
            if(CURRENT_PIECE[row][col]){
                BOARD[BLOCK_X+col][BLOCK_Y+row] = CURRENT_PIECE[row][col];
            }
        }
    }
    SCORE += BLOCK_Y*LEVEL;
    return 0;
}

int inc_score(int completed_lines){
    //score should increase with level and the number of lines completed
    int add = LEVEL * completed_lines * completed_lines * completed_lines * 25;
    SCORE += add;
    TOTAL_LINES += completed_lines;
    LEVEL = (TOTAL_LINES/15) + 1;
    CURRENT_STEP_TIMEOUT = 1.0f - (float)(LEVEL/10.0f);
    if(CURRENT_STEP_TIMEOUT < 0.2f){
        CURRENT_STEP_TIMEOUT = 0.2f;
    }

    printf("New Score +%d = %d \n", add, SCORE);

    return 0;
}

int vanish_line(int line){
    int x,y;
    for(y=line; y>0; y--){
        for(x=0; x<WIDTH; x++){
            BOARD[x][y] = BOARD[x][y-1];
        }
    }
    //blank line at top
    for(x=0;x<WIDTH;x++){
        BOARD[x][0] = 0;
    }
    return 0;
}

int score_lines(void){
    int completed_lines = 0;
    int x,y;

    for(y=0;y<HEIGHT;y++){
        int completed=1;
        for(x=0;x<WIDTH;x++){
            if(!BOARD[x][y]){
                completed = 0;
                break;
            }
        }
        if(completed){
            completed_lines++;
            vanish_line(y);
        }
    }

    if(completed_lines > 0){
        inc_score(completed_lines);
    }
    return 0;
}

int next_step(void){
    CURRENT_STEP_LEFT = CURRENT_STEP_TIMEOUT;
    if(can_drop()){
        BLOCK_Y++;
    }else{//we landed
        //are we dead?
        if(BLOCK_Y <=0){
            printf("GAME OVER :( \n");
            GAME_OVER=1;
        }
        piece_to_board();
        score_lines();
        new_piece();
    }
    return 0;
}

int process_step(float dt){
    
    CURRENT_STEP_LEFT = CURRENT_STEP_LEFT - dt;
    if(CURRENT_STEP_LEFT <=0){
        next_step();
    }

    draw_score();
    draw_board();
    draw_preview_pieces();

    //draw our current piece
    int row,col;
    for(col=0;col<4;col++){
        for(row=0;row<4;row++){
            if(CURRENT_PIECE[row][col] != 0){
                draw_cell(BLOCK_X+col,BLOCK_Y+row,CURRENT_PIECE[row][col]);
            }
        }
    }

//    draw_cell(BLOCK_X,BLOCK_Y,0);   

    return 0;

}



int main (void){

    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = SCREEN_WIDTH;
    const int screenHeight = SCREEN_HEIGHT;

    initialise();
    InitWindow(screenWidth, screenHeight, "teDLis");

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    
    char printbuff[1024];

    int key, next_key,last_key;
    double key_pressed_at, gametime,repeat_delay;
    repeat_delay = 0.125;
    key_pressed_at = 0;
    last_key = 0;
    key = 0;
    // Main game loop
    while (!WindowShouldClose() )    // Detect window close button or ESC key
    {
        gametime = GetTime();
        //get input
        if(key != 0){
            last_key = key;
        }
        key=0; 
        while (next_key = GetKeyPressed()){
           key = next_key; 
           key_pressed_at = gametime;
        }
        //we want to repeatedly do the action if the key is held down, need a delay before repeating

        if(last_key != KEY_UP && IsKeyDown(last_key)){
            //printf("%d is held down timenow=%lf ,  key_pressed=%lf \n",last_key,gametime,key_pressed_at);
            if(gametime - key_pressed_at > repeat_delay){
                key = last_key;
                key_pressed_at = gametime;
            } 
        }

        switch(key){
            case KEY_LEFT : {
                                if(can_strafe(-1)){
                                    BLOCK_X -= 1;
                                }
                                break;
                            }
            case KEY_RIGHT : {
                                if(can_strafe(+1)){
                                    BLOCK_X += 1;
                                }
                                break;
                            }
            case KEY_DOWN : {
                                if(can_drop()){
                                    BLOCK_Y += 1;
                                }
                                break;
                            }
            case KEY_UP : {
                                rotate_block(CURRENT_PIECE,1);
                                break;
                            }
        }
        

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(BLACK);
            if(GAME_OVER){
                int starty = SCREEN_HEIGHT/2 - 200;
                int fontSize = 30;
                char scorebuff[1024];
                DrawText("GAME OVER",SCREEN_WIDTH/2-100, starty, fontSize, RED);
                
                sprintf(scorebuff,"SCORE: %d",SCORE);
                starty += fontSize*1.2;
                DrawText(scorebuff,SCREEN_WIDTH/2-80,starty,fontSize,RED);
                
                sprintf(scorebuff,"Level: %d",LEVEL);
                starty += fontSize*1.2;
                DrawText(scorebuff,SCREEN_WIDTH/2-80,starty,fontSize,RED);

                sprintf(scorebuff,"Total Lines: %d",TOTAL_LINES);
                starty += fontSize*1.2;
                DrawText(scorebuff,SCREEN_WIDTH/2-120,starty,fontSize,RED);

            }else{
                process_step(GetFrameTime());
            }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}


