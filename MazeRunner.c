/*
* MazeRunner
*
* Program by: Michael Glanzmann & Marshall Denson
*
* Class: CPSC 305 with Dr. Finlayson
*
* Starting code from Dr. Finlayson (Used for accessing memory)
*
*/

//C imports
#include <stdbool.h>
#include <stdio.h>

// Screen dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160


//include the background image we are using 
#include "bgImage.h"

//include the sprite images we are using
#include "allSprites.h"

//include the tile maps we are using
#include "Maze.h" //Maze for actual gameplay
#include "homeScreen.h" //Credit & Titlescreen
#include "instructions.h" //Rules
#include "black.h" //Completely black tilemap


/* the tile mode flags needed for display control register */
#define MODE0 0x00
//BG0 for Menu Screen, Instructions Screen, and End Screen
#define BG0_ENABLE 0x100
//BG1 for completely black screen
#define BG1_ENABLE 0x200
//BG2 for Timer
#define BG2_ENABLE 0x400
//BG3 for Maze
#define BG3_ENABLE 0x800

/* flags to set sprite handling in display control register */
#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000

/* the control registers for the four tile layers */
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*) 0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*) 0x400000e;

/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* there are 128 sprites on the GBA */
#define NUM_SPRITES 128

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the memory location which controls sprite attributes */
volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

/* the memory location which stores sprite image data */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

/* the address of the color palettes used for backgrounds and sprites */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

/* the button register holds the bits which indicate whether each button has
* been pressed - this has got to be volatile as well
*/
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* scrolling registers for backgrounds */
volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (volatile short*) 0x4000014;
volatile short* bg1_y_scroll = (volatile short*) 0x4000016;
volatile short* bg2_x_scroll = (volatile short*) 0x4000018;
volatile short* bg2_y_scroll = (volatile short*) 0x400001a;
volatile short* bg3_x_scroll = (volatile short*) 0x400001c;
volatile short* bg3_y_scroll = (volatile short*) 0x400001e;

/* the bit positions indicate each button - the first bit is for A, second for
* B, and so on, each constant below can be ANDED into the register to get the
* status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
* much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank() {
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160) { }
}

/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* return a pointer to one of the 4 character blocks (0-3) */
volatile unsigned short* char_block(unsigned long block) {
    /* they are each 16K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}

/* return a pointer to one of the 32 screen blocks (0-31) */
volatile unsigned short* screen_block(unsigned long block) {
    /* they are each 2K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}

/* flag for turning on DMA */
#define DMA_ENABLE 0x80000000

/* flags for the sizes to transfer, 16 or 32 bits */
#define DMA_16 0x00000000
#define DMA_32 0x04000000

/* pointer to the DMA source location */
volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;

/* pointer to the DMA destination location */
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;

/* pointer to the DMA count/control */
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

/* copy data using DMA */
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
}









/* function to setup default backgrounds for this program */
void setup_background() {

    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) bgImage_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) bgImage_data,
            (bgImage_width * bgImage_height) / 2);

    /* set all control the bits in this register */
    *bg3_control = 3 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (8 << 8) |       /* the screen block the tile data is stored in */
        (0 << 13) |       /* wrapping flag */
        (3 << 14);        /* bg size, 0 is 256x256 */

    /* load the Maze tile data into screen block 8 */
    memcpy16_dma((unsigned short*) screen_block(8), (unsigned short*) Maze, Maze_width * Maze_height);

    /* set all control the bits in this register */
    *bg1_control = 1 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (12 << 8) |       /* the screen block the tile data is stored in */
        (0 << 13) |       /* wrapping flag */
        (0 << 14);        /* bg size, 0 is 256x256 */

    /* load the Maze tile data into screen block 8 */
    memcpy16_dma((unsigned short*) screen_block(12), (unsigned short*) black, black_width * black_height);

    /* set all control the bits in this register */
    *bg0_control = 0 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (13 << 8) |       /* the screen block the tile data is stored in */
        (0 << 13) |       /* wrapping flag */
        (0 << 14);        /* bg size, 0 is 256x256 */

    /* load the Maze tile data into screen block 8 */
    memcpy16_dma((unsigned short*) screen_block(13), (unsigned short*) homeScreen, homeScreen_width * homeScreen_height);
}

/* setup backgrounds for instructions screen after menu */
void setup_instructions(){
    /* set all control the bits in this register */
    *bg0_control = 0 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (14 << 8) |       /* the screen block the tile data is stored in */
        (0 << 13) |       /* wrapping flag */
        (0 << 14);        /* bg size, 0 is 256x256 */

    /* load the Maze tile data into screen block 8 */
    memcpy16_dma((unsigned short*) screen_block(14), (unsigned short*) instructions, instructions_width * instructions_height);
}







/* just kill time */
void delay(unsigned int amount) {
    for (int i = 0; i < amount * 10; i++);
}

/* a sprite is a moveable image on the screen */
struct Sprite {
    unsigned short attribute0;
    unsigned short attribute1;
    unsigned short attribute2;
    unsigned short attribute3;
};

/* array of all the sprites available on the GBA */
struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

/* the different sizes of sprites which are possible */
enum SpriteSize {
    SIZE_8_8,
    SIZE_16_16,
    SIZE_32_32,
    SIZE_64_64,
    SIZE_16_8,
    SIZE_32_8,
    SIZE_32_16,
    SIZE_64_32,
    SIZE_8_16,
    SIZE_8_32,
    SIZE_16_32,
    SIZE_32_64
};

/* function to initialize a sprite with its properties, and return a pointer */
struct Sprite* sprite_init(int x, int y, enum SpriteSize size,
        int horizontal_flip, int vertical_flip, int tile_index, int priority) {

    /* grab the next index */
    int index = next_sprite_index++;

    /* setup the bits used for each shape/size possible */
    int size_bits, shape_bits;
    switch (size) {
        case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
        case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
        case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
        case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
        case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
        case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
        case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
        case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
        case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
        case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
        case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
        case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
    }

    int h = horizontal_flip ? 1 : 0;
    int v = vertical_flip ? 1 : 0;

    /* set up the first attribute */
    sprites[index].attribute0 = y |             /* y coordinate */
                            (0 << 8) |          /* rendering mode */
                            (0 << 10) |         /* gfx mode */
                            (0 << 12) |         /* mosaic */
                            (1 << 13) |         /* color mode, 0:16, 1:256 */
                            (shape_bits << 14); /* shape */

    /* set up the second attribute */
    sprites[index].attribute1 = x |             /* x coordinate */
                            (0 << 9) |          /* affine flag */
                            (h << 12) |         /* horizontal flip flag */
                            (v << 13) |         /* vertical flip flag */
                            (size_bits << 14);  /* size */

    /* setup the second attribute */
    sprites[index].attribute2 = tile_index |   // tile index */
                            (priority << 10) | // priority */
                            (0 << 12);         // palette bank (only 16 color)*/

    /* return pointer to this sprite */
    return &sprites[index];
}

/* update all of the spries on the screen */
void sprite_update_all() {
    /* copy them all over */
    memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

/* setup all sprites */
void sprite_clear() {
    /* clear the index counter */
    next_sprite_index = 0;

    /* move all sprites offscreen to hide them */
    for(int i = 0; i < NUM_SPRITES; i++) {
        sprites[i].attribute0 = SCREEN_HEIGHT;
        sprites[i].attribute1 = SCREEN_WIDTH;
    }
}

/* set a sprite postion */
void sprite_position(struct Sprite* sprite, int x, int y) {
    /* clear out the y coordinate */
    sprite->attribute0 &= 0xff00;

    /* set the new y coordinate */
    sprite->attribute0 |= (y & 0xff);

    /* clear out the x coordinate */
    sprite->attribute1 &= 0xfe00;

    /* set the new x coordinate */
    sprite->attribute1 |= (x & 0x1ff);
}

/* move a sprite in a direction */
void sprite_move(struct Sprite* sprite, int dx, int dy) {
    /* get the current y coordinate */
    int y = sprite->attribute0 & 0xff;

    /* get the current x coordinate */
    int x = sprite->attribute1 & 0x1ff;

    /* move to the new location */
    sprite_position(sprite, x + dx, y + dy);
}

/* change the vertical flip flag */
void sprite_set_vertical_flip(struct Sprite* sprite, int vertical_flip) {
    if (vertical_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x2000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xdfff;
    }
}

/* change the horizontal flip flag */
void sprite_set_horizontal_flip(struct Sprite* sprite, int horizontal_flip) {
    if (horizontal_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x1000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xefff;
    }
}

/* change the tile offset of a sprite */
void sprite_set_offset(struct Sprite* sprite, int offset) {
    /* clear the old offset */
    sprite->attribute2 &= 0xfc00;

    /* apply the new one */
    sprite->attribute2 |= (offset & 0x03ff);
}

/* setup the sprite image and palette */
void setup_sprite_image() {
    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) allSprites_palette, PALETTE_SIZE);

    /* load the image into sprite image memory */
    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) allSprites_data, (allSprites_width * allSprites_height) / 2);
}


/* finds which tile a screen coordinate maps to, taking scroll into account */
unsigned short tile_lookup(int x, int y, int xscroll, int yscroll,
        const unsigned short* tilemap, int tilemap_w, int tilemap_h) {

    /* adjust for the scroll */
    x += xscroll;
    y += yscroll;

    /* convert from screen coordinates to tile coordinates */
    x >>= 3;
    y >>= 3;

    /* account for wraparound */
    while (x >= tilemap_w) {
        x -= tilemap_w;
    }
    while (y >= tilemap_h) {
        y -= tilemap_h;
    }
    while (x < 0) {
        x += tilemap_w;
    }
    while (y < 0) {
        y += tilemap_h;
    }

    /* the larger screen maps (bigger than 32x32) are made of multiple stitched
       together - the offset is used for finding which screen block we are in
       for these cases */
    int offset = 0;

    /* if the width is 64, add 0x400 offset to get to tile maps on right   */
    if (tilemap_w == 64 && x >= 32) {
        x -= 32;
        offset += 0x400;
    }

    /* if height is 64 and were down there */
    if (tilemap_h == 64 && y >= 32) {
        y -= 32;

        /* if width is also 64 add 0x800, else just 0x400 */
        if (tilemap_w == 64) {
            offset += 0x800;
        } else {
            offset += 0x400;
        }
    }

    /* find the index in this tile map */
    int index = y * 32 + x;

    /* return the tile */
    return tilemap[index + offset];
}

/* returns 1 if passed index is wall, 0 if path */
int isWall(unsigned short tile){
    int walls[] = {2,3,6,7,10,11,16,17,34,35,38,39,42,43,48,49};
    int foundWall = 0;

    for(int i = 0; i < 16; i++){
        if(walls[i] == tile){
            foundWall = 1;
            break;
        }
    }
        
    if(foundWall){
        return 1;
    }
    else {
        return 0;
    }
}

/* struct for our Runner sprite's logic and behavior */
struct Runner {
    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion */
    int x, y;

    /* which frame of the animation he is on */
    int frame;

    /* the number of frames to wait before flipping */
    int animation_delay;

    /* the animation counter counts how many frames until we flip */
    int counter;

    /* whether the Runner is moving right now or not */
    int move;

    /* the number of pixels away from the edge of the screen the Runner stays */
    int border;
};

/* initialize the Runner */
void runner_init(struct Runner* run) {
    run->x = 16;
    run->y = 16;
    run->border = 8;
    run->frame = 0;
    run->move = 0;
    run->counter = 0;
    run->animation_delay = 16;
    run->sprite = sprite_init(run->x, run->y, SIZE_16_16, 0, 0, run->frame, 0);
}

/* move the runner left or right returns if it is at edge of the screen */
/* Also checks for wall collisions */
int runner_left(struct Runner* run, int xscroll, int yscroll) {
    /* face left */
    sprite_set_horizontal_flip(run->sprite, 1);
    run->move = 1;

    /* if we are at the left end, just scroll the screen */
    if (run->x < run->border) {
        return 1;
    } else {

        /* Check for tile to left */
        unsigned short tile = tile_lookup(run->x - 1, run->y + 8, xscroll, yscroll, Maze, Maze_width, Maze_height);

        if(!isWall(tile)){
            run->x--;
        }
        return 0;
    }
}
int runner_right(struct Runner* run, int xscroll, int yscroll) {
    /* face right */
    sprite_set_horizontal_flip(run->sprite, 0);
    run->move = 1;

    /* if we are at the right end, just scroll the screen */
    if (run->x > (SCREEN_WIDTH - 16 - run->border)) {
        return 1;
    } else {
        
        /* check for tile to right */
        unsigned short tile = tile_lookup(run->x + 16, run->y + 8, xscroll, yscroll, Maze, Maze_width, Maze_height);

        if(!isWall(tile)){
            run->x++;
        }
        return 0;
    }
}

/* move the runner up or down returns if it is at edge of the screen */
int runner_up(struct Runner* run, int xscroll, int yscroll) {
    /* face left */
    sprite_set_horizontal_flip(run->sprite, 1);
    run->move = 1;

    /* if we are at the top, just scroll the screen */
    if (run->y < run->border) {
        return 1;
    } else {
        
        /* check for tile above */
        unsigned short tile = tile_lookup(run->x + 8, run->y - 1, xscroll, yscroll, Maze, Maze_width, Maze_height);

        if(!isWall(tile)){
            run->y--;
        }
        return 0;
    }
}
int runner_down(struct Runner* run, int xscroll, int yscroll) {
    /* face right */
    sprite_set_horizontal_flip(run->sprite, 0);
    run->move = 1;

    /* if we are at the bottom, just scroll the screen */
    if (run->y > (SCREEN_HEIGHT - 16 - run->border)) {
        return 1;
    } else {

        /* check for tile below */
        unsigned short tile = tile_lookup(run->x + 8, run->y + 16, xscroll, yscroll, Maze, Maze_width, Maze_height);

        if(!isWall(tile)){
            run->y++;
        }
        return 0;
    }
}

void runner_stop(struct Runner* run) {
    run->move = 0;
    run->frame = 0;
    run->counter = 7;
    sprite_set_offset(run->sprite, run->frame);
}

/* update the runner */
void runner_update(struct Runner* run) {
    if (run->move) {
        run->counter++;
        if (run->counter >= run->animation_delay) {
            if(run->frame == 8){
                run->frame = 0;
            }
            else{
                run->frame = 8;
            }
            sprite_set_offset(run->sprite, run->frame);
            run->counter = 0;
        }
    }

    sprite_position(run->sprite, run->x, run->y);
}





/* struct for our Key sprite's logic and behavior */
struct Key {
    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion */
    int x, y;

    /* which frame of the animation he is on */
    int frame;

    /* the number of frames to wait before flipping */
    int animation_delay;

    /* the animation counter counts how many frames until we flip */
    int counter;
};

/* initialize the Runner */
void key_init(struct Key* key, int x, int y) {
    key->x = x;
    key->y = y;
    key->frame = 16;
    key->counter = 0;
    key->animation_delay = 32;
    key->sprite = sprite_init(key->x, key->y, SIZE_16_16, 0, 0, key->frame, 1);
}

/* update the runner */
void key_update(struct Key* key) {
    key->counter++;
    if (key->counter >= key->animation_delay) {
        if(key->frame == 24){
            key->frame = 16;
        }
        else{
            key->frame = 24;
        }
        sprite_set_offset(key->sprite, key->frame);
        key->counter = 0;
    }
}





/* Scrolls the screen if possible, but will not scroll off screen to repeat maze */
void safe_xscroll(int *xscroll, int scrollBy) {
    //If it is safe to scroll right
    if(scrollBy > 0 && *xscroll < 272){
        *xscroll += 1;
    }
    //If safe to scroll left
    if(scrollBy < 0 && *xscroll > 0){
        *xscroll -= 1;
    }
}

/* Scrolls the screen if possible, but will not scroll off screen to repeat maze */
void safe_yscroll(int *yscroll, int scrollBy) {
    //If it is safe to scroll down
    if(scrollBy > 0 && *yscroll < 352){
        *yscroll += 1;
    }
    //If safe to scroll up
    if(scrollBy < 0 && *yscroll > 0){
        *yscroll -= 1;
    }
}








/* the main function */
int main() {
    /* we set the mode to mode 0 with bg2 on */
    //*display_control = MODE0 | BG3_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;

    *display_control = MODE0 | BG0_ENABLE | BG1_ENABLE;

    /* setup the background Maze:3 Black: 1 Menu:0 */
    setup_background();

    /* setup the sprite image data */
    setup_sprite_image();

    /* clear all the sprites on screen now */
    sprite_clear();



    /* create the runner */
    struct Runner runner;
    runner_init(&runner);
    
    /* create keys */
    struct Key key1;
    key_init(&key1, 80, 144);
    struct Key key2;
    key_init(&key2, 368, 112);
    struct Key key3;
    key_init(&key3, 384, 384);

    /* set initial scroll to 0 */
    int yscroll = 0;
    int xscroll = 0;

    /* True when player is on menu screen */
    bool onMenu = true;

    /* True when player is on instructions screen */
    bool onInstructions = true;
    
    /* True when player wins or game overs */
    bool gameOver = false; 

    /* loop forever */
    while (1) {

        //Setup for menu loop
        if(onMenu){
            setup_background();
            *display_control = MODE0 | BG0_ENABLE | BG1_ENABLE;
        }

        //Menu loop
        while(onMenu){
        
            //Break out of loop
            if(button_pressed(BUTTON_LEFT)){
                setup_instructions();
                *display_control = MODE0 | BG0_ENABLE | BG1_ENABLE;
                onMenu = false;
            }

        }
        // Instructions (menu page 2) loop
        while(onInstructions){

            //Break out of loop
            if(button_pressed(BUTTON_RIGHT)){
                *display_control = MODE0 | BG3_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;
                onInstructions = false;
            }

        }


        /* update the runner */
        runner_update(&runner);

        /* update keys */
        key_update(&key1);
        key_update(&key2);
        key_update(&key3);

        /* now the arrow keys move the runner */
        if (button_pressed(BUTTON_RIGHT)) {
            
            if (runner_right(&runner,xscroll,yscroll)) {
                safe_xscroll(&xscroll, 1);
                sprite_move(key1.sprite, -1, 0);
                sprite_move(key2.sprite, -1, 0);
                sprite_move(key3.sprite, -1, 0);
            }

        } else if (button_pressed(BUTTON_LEFT)) {

            if (runner_left(&runner,xscroll,yscroll)) {
                safe_xscroll(&xscroll, -1);
                sprite_move(key1.sprite, 1, 0);
                sprite_move(key2.sprite, 1, 0);
                sprite_move(key3.sprite, 1, 0);
            }

        } else if (button_pressed(BUTTON_DOWN)) {

            if (runner_down(&runner,xscroll,yscroll)){
                safe_yscroll(&yscroll, 1);
                sprite_move(key1.sprite, 0, -1);
                sprite_move(key2.sprite, 0, -1);
                sprite_move(key3.sprite, 0, -1);
            }
        
        } else if (button_pressed(BUTTON_UP)) {

            if (runner_up(&runner,xscroll,yscroll)){
                safe_yscroll(&yscroll, -1);
                sprite_move(key1.sprite, 0, 1);
                sprite_move(key2.sprite, 0, 1);
                sprite_move(key3.sprite, 0, 1);
            }

        } else {
            runner_stop(&runner);
        }


        /* wait for vblank before scrolling and moving sprites */
        wait_vblank();
        *bg3_x_scroll = xscroll;
        *bg3_y_scroll = yscroll;
        sprite_update_all();

        /* delay some */
        delay(500);
    }
}
