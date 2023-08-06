#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"

/*****
 *
 * TODOS
 * 
 *   - do some animation when lines complete, when moving, etc
 *   - sound
 *   - press space to drop to bottom
 *   - clean up code, remove globals to allow for multiplayer
 *   - fix GAME OVER display alignment
 * */

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 640;
enum { HEIGHT = 25, WIDTH = 10};
const int CELL_DIM = 25;



//BOARD BASE POSITION in pixels
const int B_X = (SCREEN_WIDTH - CELL_DIM * WIDTH) / 2;
const int B_Y = (SCREEN_HEIGHT - CELL_DIM * HEIGHT) / 2;

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


typedef struct {
	int level;
	int score;
	int total_lines;
    int game_over;
    
    int board[WIDTH][HEIGHT];
	int current_piece[4][4];
	int piece_buffer[3];
	int block_x;
	int block_y;
	int block_x_last;
	int block_y_last;
	int block_x_size;
	int block_y_size;
	double block_update_time;
    float step_timeout;
    float current_step_left;
} Player;

int new_piece(Player*);

int init_player(Player *player){
	player->block_x = 5;
	player->block_y = 0;
	player->block_x_last = 5;
	player->block_y_last = 0;
	player->block_x_size = 0;
	player->block_y_size = 0;
	player->block_update_time = 0;
    player->step_timeout = 1.0f;
    player->piece_buffer[0]=0;
    player->piece_buffer[1]=0;
    player->piece_buffer[2]=0;
	player->current_step_left = player->step_timeout;

    player->game_over = 0;
    player->score = 0;
    player->level = 1;
    player->total_lines = 0;
    
	int x, y;

	for(x=0;x<WIDTH;x++){
        for(y=0; y<HEIGHT; y++){
            player->board[x][y] = 0;
        }
    }
    new_piece(player);new_piece(player);new_piece(player);//fill up the piece buffer

	return 0;
}

int update_block_pos(Player *player, int inc_x, int inc_y){
    player->block_x_last = player->block_x;
    player->block_y_last = player->block_y;
    player->block_x += inc_x;
    player->block_y += inc_y;
    player->block_update_time = GetTime();
}


int draw_cell_px(int x, int y, int cell_dim, int contents){
    const Color colors[] = {RED,GOLD,ORANGE,LIME,BROWN,DARKBLUE,RAYWHITE};

    int idx = contents -1 % 7;
    DrawRectangle(x, y, cell_dim-1, cell_dim-1, colors[idx]);

}

int draw_cell(int base_x, int base_y, int x, int y, int contents){
 
    draw_cell_px(base_x + x * CELL_DIM, base_y + y * CELL_DIM, CELL_DIM, contents);

}

int draw_score(Player *player){

    char scorebuff[1024];
    sprintf(scorebuff,"Score: %d",player->score);
    DrawText(scorebuff,10,10,20,RAYWHITE);
    sprintf(scorebuff,"Level: %d",player->level);
    DrawText(scorebuff,10,45,20,RAYWHITE);
    return 0;
}


int draw_board(int board[WIDTH][HEIGHT], int boardpos){
    int boardoffset = 100 + boardpos * CELL_DIM * WIDTH;

    //int c_x = (boardoffset) / 2 ;
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
            if(board[x][y] != 0){
                draw_cell(bx,by,x,y,board[x][y]);
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

int draw_preview_pieces(Player *player){
    int x = (SCREEN_WIDTH + WIDTH * CELL_DIM) / 2 + 50;
    int y = 10;
    int preview_cell_dim = 10;
    draw_preview(x, y, player->piece_buffer[0], preview_cell_dim);
    y += preview_cell_dim * 4;
    draw_preview(x, y, player->piece_buffer[1], preview_cell_dim);

}

int rotate_block(Player *player, int result[4][4], int rotation){
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
    int temp = player->block_x_size;
    player->block_x_size = player->block_y_size;
    player->block_y_size = temp;
    //check if we're out of bounds and move
    if(player->block_x +player->block_x_size > WIDTH){
        player->block_x = WIDTH - player->block_x_size;
        //in case we wall-kicked we need to update last_pos to avoid animation artifacts
        update_block_pos(player,0,0);
    }
    if(player->block_x < 0){//should never be true
        player->block_x = 0;
    }
    
    return 0;
}


int set_current_piece(Player *player, int tet_index){

    assert(tet_index < 7);
    int row,col;
    
    //clear 
    for(row = 0; row < 4; row++){
        for(col = 0; col < 4; col++){
            player->current_piece[row][col] = 0;
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
                player->current_piece[row][col] = curval;
            }            
        }
    }
    
    player->block_x_size = max_x + 1;
    player->block_y_size = max_y + 1;
    return 0; 
}

int new_piece(Player *player){

    set_current_piece(player,player->piece_buffer[0]);
    player->piece_buffer[0] = player->piece_buffer[1];
    player->piece_buffer[1] = player->piece_buffer[2];
    player->piece_buffer[2] = (int) rand() % 7;
    player->block_x = 5;
    player->block_x_last = 5;
    player->block_y = 0;
    player->block_y_last = 0;
    player->block_update_time = GetTime();
}


int cell_full(int board[WIDTH][HEIGHT], int x, int y){
    
    if(y >= HEIGHT){
//        printf("reached bottom %d,%d \n",x,y);
        return 1;
    }

    if(x < 0 || x >= WIDTH){
        return 1;
    }

    if(board[x][y]){
//        printf("board full @ %d,%d \n",x,y);
        return 1;
    }else{
        return 0;
    }
}

int can_strafe(Player *player, int x_offset){
    //make sure that there is a space on the board for every row of the current_piece
    int x,y;
    for(x=0; x < player->block_x_size; x++){
        for(y=0; y < player->block_y_size; y++){
            if(player->current_piece[y][x]){
                if(cell_full(player->board, player->block_x + x + x_offset, player->block_y + y)){
                    return 0;
                }
            }
        }
    }
    //didn't fail == success
    return 1;
}

int can_drop(Player *player){
    int row,col;

    //foreach col in the current piece get the highest row-index

    for(col=0; col < 4; col++){
        for(row =0; row < 4; row++){
            if(player->current_piece[row][col]){
                if(cell_full(player->board, player->block_x + col, player->block_y + row + 1)){
//                    printf("can't drop \n");
                    return 0;
                }
            }
        }
    }
//    printf("Can drop %d,%d\n",BLOCK_X,BLOCK_Y);
    return 1;
}



int piece_to_board(Player *player){
    int row,col;

	//adjust postition in case we came off the bottom, 
	// FIXME this is porbably buggy
	if(player->block_y + player->block_y_size > HEIGHT){
		player->block_y = HEIGHT - player->block_y_size;		
	}

    for(row=0; row < 4; row++){
        for(col=0;col<4;col++){
            if(player->current_piece[row][col]){
                player->board[player->block_x + col][player->block_y + row] = player->current_piece[row][col];
            }
        }
    }
    player->score += player->block_y * player->level;
    return 0;
}

int calc_score(int completed_lines, int current_level){
    return  current_level * completed_lines * completed_lines * completed_lines * 25;
}

int inc_score(Player *player, int completed_lines){
    //score should increase with level and the number of lines completed
//    int add = LEVEL * completed_lines * completed_lines * completed_lines * 25;
    int add = calc_score(completed_lines, player->level);
    player->score += add;
    player->total_lines += completed_lines;
    player->level = (player->total_lines/15) + 1;
    player->step_timeout = 1.0f - (float)(player->level/10.0f);
    if(player->step_timeout < 0.2f){
        player->step_timeout = 0.2f;
    }

 //   printf("New Score +%d = %d \n", add, player->score);

    return 0;
}



int vanish_line(int board[WIDTH][HEIGHT], int line){
    int x,y;
    for(y=line; y>0; y--){
        for(x=0; x<WIDTH; x++){
            board[x][y] = board[x][y-1];
        }
    }
    //blank line at top
    for(x=0;x<WIDTH;x++){
        board[x][0] = 0;
    }
//    start_animation_completed_line(line);
    return 0;
}

int score_lines(int board[WIDTH][HEIGHT]){
    int completed_lines = 0;
    int x,y;

    for(y=0;y<HEIGHT;y++){
        int completed=1;
        for(x=0;x<WIDTH;x++){
            if(!board[x][y]){
                completed = 0;
                break;
            }
        }
        if(completed){
            completed_lines++;
            vanish_line(board,y);
        }
    }

    return completed_lines;
}

int next_step(Player *player){
    player->current_step_left = player->step_timeout;
    if(can_drop(player)){
        update_block_pos(player,0,1);
    }else{//we landed
        //are we dead?
        if(player->block_y <=0){
            printf("GAME OVER :( \n");
            //GAME_OVER=1;
            player->game_over=1;
        }
        piece_to_board(player);
        int completed_lines = score_lines(player->board);
        if(completed_lines > 0){
            inc_score(player, completed_lines);
        }
        new_piece(player);
    }
    return 0;
}

int process_particles(void *particles, float dt){
    int i;
    for(i=0;i< 1024;i++){
        //particles[i]->lifetime -= dt;
    }
    return 0;
}

int process_step(float dt, double gametime, Player *player){
    
    player->current_step_left -= dt;
    if(player->current_step_left <= 0){
        //printf("CURRENT STEP ELAPSED %lf \n",CURRENT_STEP_LEFT),
        next_step(player);
    }

    draw_score(player);
    draw_board(player->board,1);//TODO fix for multiplayer mode
    draw_preview_pieces(player);

    void * particles;
    process_particles(particles, dt);

    //draw our current piece 
    // we want to lerp between last and current pos 
    // ? in 1/8 of a sec we want to go cross a cell-width with ease in and out, at 30fps ~ 4 frames


    float lerp = (gametime - player->block_update_time)/.4;
    int offset_x = 0;
    int offset_y = 0;
    if(lerp <= 1){
        offset_x = (player->block_x - player->block_x_last) * CELL_DIM * lerp + (player->block_x_last * CELL_DIM);
        offset_y = (player->block_y - player->block_y_last) * CELL_DIM * lerp + (player->block_y_last * CELL_DIM);
//        printf("lerp is %.4f, offset_x=%d, offset_y=%d ,final_y = %d\n",lerp, offset_x, offset_y,B_Y + offset_y + (BLOCK_Y +1 )*CELL_DIM);
    }else{
		offset_x = player->block_x * CELL_DIM;
		offset_y = player->block_y * CELL_DIM;
	}
    int row,col;
    int base_x = B_X + offset_x;// + (BLOCK_X_LAST * CELL_DIM);
    int base_y = B_Y + offset_y;// + (BLOCK_Y_LAST * CELL_DIM);
//    printf("Drawing at basepos %d, %d \n",base_x,base_y);
    for(col=0;col<4;col++){
        for(row=0;row<4;row++){
            if(player->current_piece[row][col] != 0){
                draw_cell_px(base_x + (col*CELL_DIM), base_y + (row*CELL_DIM), CELL_DIM, player->current_piece[row][col]);
 //               draw_cell_px(B_X - 4 + ((BLOCK_X + col)*CELL_DIM), B_Y + offset_y + ((BLOCK_Y + row-4)*CELL_DIM), CELL_DIM, CURRENT_PIECE[row][col]);
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


    srand(time(0));
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
	Player player;
	init_player(&player);
    printf("initialised player\n");
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
                                if(can_strafe(&player,-1)){
                                    update_block_pos(&player,-1,0);//BLOCK_X -= 1;
                                }
                                break;
                            }
            case KEY_RIGHT : {
                                if(can_strafe(&player,+1)){
                                    update_block_pos(&player,1,0);
                                }
                                break;
                            }
            case KEY_DOWN : {
                                if(can_drop(&player)){
                                    update_block_pos(&player, 0,1);                                
                                }
                                break;
                            }
            case KEY_UP : {     
                                rotate_block(&player, player.current_piece, 1);
                                break;
                            }
        }
        

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(BLACK);
            if(player.game_over){
                int starty = SCREEN_HEIGHT/2 - 200;
                int fontSize = 30;
                char scorebuff[1024];
                DrawText("GAME OVER",SCREEN_WIDTH/2-100, starty, fontSize, RED);
                
                sprintf(scorebuff,"SCORE: %d", player.score);
                starty += fontSize*1.2;
                DrawText(scorebuff,SCREEN_WIDTH/2-80,starty,fontSize,RED);
                
                sprintf(scorebuff,"Level: %d", player.level);
                starty += fontSize*1.2;
                DrawText(scorebuff,SCREEN_WIDTH/2-80, starty,fontSize,RED);

                sprintf(scorebuff,"Total Lines: %d", player.total_lines);
                starty += fontSize*1.2;
                DrawText(scorebuff, SCREEN_WIDTH/2-120, starty, fontSize,RED);

            }else{
                process_step(GetFrameTime(), gametime, &player);                
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


