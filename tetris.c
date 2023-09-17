#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"

/*****
 *
 * TODOS
 *   - do some animation when lines complete, when moving, etc
 *   - sound
 *   - ? multiplayer  (with punish)
 *   - ? network multiplayer?
 *   - INI file to store keymap / hihgscores / other defaults (e.g. num players)
 * */

// STORE some of these in INI file and use as global vars
#define PLAYERS 2
#define TOTAL_SCREEN_WIDTH 1000
#define TOTAL_SCREEN_HEIGHT 640


#define SCREEN_WIDTH (TOTAL_SCREEN_WIDTH/PLAYERS)
#define SCREEN_HEIGHT TOTAL_SCREEN_HEIGHT
#define HEIGHT 25
#define WIDTH 10
#define CELL_DIM 25



// constants for key-indexes  
#define KEYIDX_UP    0
#define KEYIDX_LEFT  1
#define KEYIDX_DOWN  2
#define KEYIDX_RIGHT 3
#define KEYIDX_DROP  4


//BOARD BASE POSITION in pixels
const int PLAYER_BOARD_OFFSET_X = (SCREEN_WIDTH - CELL_DIM * WIDTH) / 2;
const int PLAYER_BOARD_OFFSET_Y = (SCREEN_HEIGHT - CELL_DIM * HEIGHT) / 2;

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
	int playernum;
    int keymap[5];
	int level;
	int score;
	int total_lines;
    int game_over;

	int boardpos_x; //base board display pixel position
	int boardpos_y;
    
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

int* init_keymap(Player *player){
	//dirs and space to drop
    if(player->playernum == 1){	
        player->keymap[KEYIDX_UP]    = KEY_UP;
        player->keymap[KEYIDX_DOWN]  = KEY_DOWN;
        player->keymap[KEYIDX_LEFT]  = KEY_LEFT;
        player->keymap[KEYIDX_RIGHT] = KEY_RIGHT;
        player->keymap[KEYIDX_DROP]  = KEY_RIGHT_CONTROL;
    }
	if(player->playernum == 0){	
        player->keymap[KEYIDX_UP]    = KEY_W;
        player->keymap[KEYIDX_DOWN]  = KEY_S;
        player->keymap[KEYIDX_LEFT]  = KEY_A;
        player->keymap[KEYIDX_RIGHT] = KEY_D;
        player->keymap[KEYIDX_DROP]  = KEY_LEFT_CONTROL;
    }

	return player->keymap;
}

enum sound_enums { sound_vanish, sound_land, sound_rotate, sound_invalid, sound_game_over, sound_max};
Sound sounds[sound_max];


int init_sounds(){
    
    InitAudioDevice();      // Initialize audio device

    sounds[sound_vanish]  = LoadSound("sfx_explode.ogg");
    sounds[sound_land]    = LoadSound("sfx_land.ogg"); 
    sounds[sound_rotate]  = LoadSound("sfx_ping.wav");
    sounds[sound_invalid] = LoadSound("sfx_meh.wav");
}

int init_player(Player *player, int playernum){
	
    player->playernum = playernum;
	player->boardpos_x = PLAYER_BOARD_OFFSET_X + (SCREEN_WIDTH * playernum);
	//currently we only support one row of players
	player->boardpos_y = PLAYER_BOARD_OFFSET_Y;
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

	init_keymap(player);
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
	int x_start = SCREEN_WIDTH * player->playernum + 5;//player->boardpos_x - ( WIDTH * CELL_DIM ) -20;
    sprintf(scorebuff,"Score: %d",player->score);
    DrawText(scorebuff, x_start, 10, 20, RAYWHITE);
    sprintf(scorebuff,"Level: %d",player->level);
    DrawText(scorebuff, x_start, 45, 20, RAYWHITE);
    return 0;
}


int draw_board(Player *player){
//    int board[WIDTH][HEIGHT] = player->board,
    //int boardoffset = 100 + player->boardpos_x * CELL_DIM * WIDTH;
	

    //int c_x = (boardoffset) / 2 ;
    int c_x = SCREEN_WIDTH / 2;
    int c_y = SCREEN_HEIGHT / 2;

    //draw surround
    //int bx,by;
    //bx = c_x - (CELL_DIM * WIDTH) / 2;
    //by = c_y - (CELL_DIM * HEIGHT) / 2;
    DrawRectangle(player->boardpos_x, player->boardpos_y, CELL_DIM * WIDTH, CELL_DIM * HEIGHT, BLUE);

    //draw contents
    int x,y;
    for(x=0;x<WIDTH;x++){
        for(y=0; y<HEIGHT; y++){
            if(player->board[x][y] != 0){
                draw_cell(player->boardpos_x, player->boardpos_y, x, y, player->board[x][y]);
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
    int x = player->boardpos_x + (WIDTH * CELL_DIM) + 40; //(SCREEN_WIDTH + WIDTH * CELL_DIM) / 2 + 50;
    int y = 10;
    int preview_cell_dim = 10;
    draw_preview(x, y, player->piece_buffer[0], preview_cell_dim);
    y += preview_cell_dim * 4;
    draw_preview(x, y, player->piece_buffer[1], preview_cell_dim);

}


int play_sound(enum sound_enums sound_t){
    //printf("playing sound %d \n",sound_t);
    PlaySound(sounds[sound_t]);
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

/**
 * returns true if piece fits
 *
 **/
bool piece_fits(int board[WIDTH][HEIGHT], int piece[4][4], int pos_x, int pos_y,int x_size, int y_size){
    int col, row;
    //printf("checking pos %d,%d width %d\n",pos_x,pos_y,x_size);
    if(pos_x + x_size > WIDTH) return false;
    if(pos_y + y_size > HEIGHT) return false;
    for(col =0; col < y_size; col++){
        for(row = 0; row < x_size; row++){
            if(piece[col][row]){
                if(cell_full(board, pos_x + row, pos_y + col)){
                    return false;
                }
            }
        }
    }
    return true;
}


/**
 * convenience function to call piece_fits() from a Player
 */
bool can_go_to(Player *player, int x_offset, int y_offset){
    return piece_fits(
            player->board, 
            player->current_piece, 
            player->block_x + x_offset, 
            player->block_y + y_offset, 
            player->block_x_size, 
            player->block_y_size);
}

int rotate_block(Player *player, int piece[4][4], int rotation){
    int swap[4][4];
    int copy[4][4];
	
    int x,y,d, copy_x_size, copy_y_size;

    //make a copy to mess with
    for(y=0;y<4;y++){
        for(x=0;x<4;x++){
            copy[x][y] = piece[x][y];
        }
    }

    //clear swap space
    for(x=0;x<4;x++){
        for(y=0;y<4;y++){
            swap[y][x] = 0;
        }
    }
   
    //rotate around diagonal
    for(x=0;x<4;x++){
        for(y=0;y<4;y++){
            swap[y][x] = piece[x][y];
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
            copy[x][y]=0;
        }
    }

    //flip about half of max_x
    for(y=0;y<4;y++){
        for(d=0;d<=(max_x);d++){
            copy[d][y] = swap[max_x-d][y];
        }
    }

    //swap x and y extents;
    copy_x_size = player->block_y_size;
    copy_y_size = player->block_x_size;

    bool valid = 0;

    //check if new rotation collides with blocks on the board or if it's possible to offset on the same line and get a fit
    int offset = 0;
    for(offset = 0; offset >= -copy_x_size; offset--){
        valid = piece_fits(player->board, copy, player->block_x+offset,player->block_y,copy_x_size, copy_y_size);
        if(valid){
            player->block_x += offset; //adjust block x pos if we found a valid position
            play_sound(sound_rotate);
            break;
        }
    }
    
    if(!valid){
        play_sound(sound_invalid);
        return -1;
    }else{
        //copy result to incoming piece
        for(y=0;y<4;y++){
            for(x=0;x<4;x++){
                piece[y][x]=copy[y][x];
            }
        }
        player->block_x_size = copy_x_size;
        player->block_y_size = copy_y_size;
        return 0;
    }
}



/**
 * places the currently moving piece on the board (makes it settle/stick) before the next piece comes into play
 *
 */
int piece_to_board(Player *player){
    int row,col;

	//adjust postition in case we came off the bottom, 
	// FIXME this is probably buggy
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
    play_sound(sound_land);
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

    play_sound(sound_vanish);
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
    if(can_go_to(player,0,1)){
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
    draw_board(player);//TODO fix for multiplayer mode
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
    int base_x = player->boardpos_x + offset_x;// + (BLOCK_X_LAST * CELL_DIM);
    int base_y = player->boardpos_y + offset_y;// + (BLOCK_Y_LAST * CELL_DIM);
//    printf("Drawing at basepos %d, %d \n",base_x,base_y);
    for(col=0;col<4;col++){
        for(row=0;row<4;row++){
            if(player->current_piece[row][col] != 0){
                draw_cell_px(base_x + (col*CELL_DIM), base_y + (row*CELL_DIM), CELL_DIM, player->current_piece[row][col]);
            }
        }
    }
    DrawRectangle(base_x, player->boardpos_y + (HEIGHT*CELL_DIM), player->block_x_size * CELL_DIM, 10, GRAY);

//    draw_cell(BLOCK_X,BLOCK_Y,0);   

    return 0;

}



int draw_text_centered(char *txtbuff, int center_x, int start_y, int fontsize, Color color){
    int offset = MeasureText(txtbuff, fontsize)/2;

    DrawText(txtbuff, center_x-offset, start_y, fontsize, color);
    return 0;

}

int draw_game_over_screen(Player *player){

    int starty = SCREEN_HEIGHT/2 - 150;
    int fontSize = 30;
    char scorebuff[1024];
    int center_x = SCREEN_WIDTH * player->playernum + SCREEN_WIDTH /2;
    
    sprintf(scorebuff,"PLAYER %d", player->playernum + 1);
    draw_text_centered(scorebuff, center_x,starty,fontSize,RED);
    
    starty += fontSize*1.2;   
    draw_text_centered("Game Over", center_x, starty, fontSize, RED);
    
    sprintf(scorebuff,"SCORE: %d", player->score);
    starty += fontSize*1.2;
    draw_text_centered(scorebuff, center_x,starty,fontSize,RED);
    
    sprintf(scorebuff,"Level: %d", player->level);
    starty += fontSize*1.2;
    draw_text_centered(scorebuff, center_x, starty,fontSize,RED);

    sprintf(scorebuff,"Total Lines: %d", player->total_lines);
    starty += fontSize*1.2;
    draw_text_centered(scorebuff, center_x, starty, fontSize,RED);


}



int main (void){

    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = TOTAL_SCREEN_WIDTH;
    const int screenHeight = TOTAL_SCREEN_HEIGHT;


    srand(time(0));
    InitWindow(screenWidth, screenHeight, "teDLis");


    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    
    init_sounds();
    //--------------------------------------------------------------------------------------

    
    char printbuff[1024];

    int key, next_key,last_key;
    double key_pressed_at, gametime,repeat_delay;
    repeat_delay = 0.125;
    key_pressed_at = 0;
    last_key = 0;
    key = 0;
	Player players[PLAYERS];
    
	int i;
	for(i=0; i<PLAYERS; i++){
		init_player(&players[i], i);
	}	

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

        /* THIS WON't WORK FOR MULTIPLAYER
        //we want to repeatedly do the action if the key is held down, need a delay before repeating

        if(last_key != KEY_UP && IsKeyDown(last_key)){
            //printf("%d is held down timenow=%lf ,  key_pressed=%lf \n",last_key,gametime,key_pressed_at);
            if(gametime - key_pressed_at > repeat_delay){
                key = last_key;
                key_pressed_at = gametime;
            } 
        }
        */
		//printf("key pressed = %d\n",key);
		int p;
		for(p=0; p < PLAYERS; p++){
			
				if(key == players[p].keymap[KEYIDX_LEFT]) {
									if(can_go_to(&players[p],-1,0)){
										update_block_pos(&players[p],-1,0);//BLOCK_X -= 1;
									}									
								}
				if(key == players[p].keymap[KEYIDX_RIGHT]) {
                                    if(can_go_to(&players[p],1,0)){
										update_block_pos(&players[p],1,0);
									}
								}
				if(key == players[p].keymap[KEYIDX_DOWN]) {
									if(can_go_to(&players[p],0,1)){
										update_block_pos(&players[p], 0,1);                                
									}
								}
				if(key == players[p].keymap[KEYIDX_UP]) {     
									rotate_block(&players[p], players[p].current_piece, 1);
								}
				if(key == players[p].keymap[KEYIDX_DROP]) {
									while(can_go_to(&players[p],0,1)){
										update_block_pos(&players[p],0,1);
									}
								}
			
		}

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
			for(p=0;p<PLAYERS;p++){
				ClearBackground(BLACK);
				if(players[p].game_over){
				   draw_game_over_screen(&players[p]);
				}else{
					process_step(GetFrameTime(), gametime, &players[p]);                
				}
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


