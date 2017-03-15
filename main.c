/*	A chip-8 emulator,written by Mouzakitis Nikolaos,Crete,2017.
			CHIP-8 emulator. */



/*	Includes 	*/

#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <graphics.h>

/*	Defines	*/
#define TRUE 	       1
#define FALSE	       0
#define SCREEN_WIDTH  64
#define	SCREEN_HEIGHT 32
#define SCREEN_DEPTH  32
#define SCALE_FACTOR  14
#define SCREEN_VERTREFRESH 60		/*	 in Hz.		*/

#define NUMBEROFKEYS	15
#define KEY_NOKEY	-999
#define ROM_DEFAULT 0x200



int run_state = 1;		/* 1-run	 0-stop		*/
FILE *db;			/*  used to debbug data.	*/

TTF_Font 	*message_font;
SDL_Surface 	*screen;	/*	main SDL structure	*/
SDL_Surface 	*overlay;	/*	overlay structure	*/
SDL_Surface 	*virtscreen;	/*	chip-8 screen		*/
SDL_Event	event;		/*	stores SDL events	*/
SDL_TimerID	cpu_timer;	/*	CPU tick timer		*/
int 		op_delay;	/*	(ms) delay in CPU	*/
int 		screen_width;
int 		screen_height;
int 		scale_factor;
int 		decrement_timers;

/*	Colors		*/

uint32_t	COLOR_BLACK;
uint32_t	COLOR_WHITE;
uint32_t	COLOR_DGREEN;
uint32_t	COLOR_LGREEN;
SDL_Color	COLOR_TEXT;




struct syst
{
	uint8_t V[16];			//	16 registers.
	uint16_t i;			// 	index register.
	uint16_t opcode;		//	current opcode.
	uint16_t pc;			//	program counter.
	uint16_t stack[16];
	uint8_t stack_pointer;
	uint8_t screen[64*32];
	uint8_t delay_timer;
	uint8_t sound_timer;
	uint8_t Wmem;			//	written mem.(how many bytes after 0x200 have instructions)
	unsigned char memory[4096];
	uint8_t kb[16]; 		//	keabord 16 keys.
} chip;

typedef struct
{
	int keycode;
	SDLKey symbol;

} KEYSPEC ;

KEYSPEC keaboard_def[] = {
	{0x1b,SDLK_END},
	{0x1,SDLK_4},
	{0x2,SDLK_5},
	{0x3,SDLK_6},
	{0x4,SDLK_r},
	{0x5,SDLK_t},
	{0x6,SDLK_y},
	{0x7,SDLK_f},
	{0x8,SDLK_g},
	{0x9,SDLK_h},
	{0xA,SDLK_v},
	{0xB,SDLK_b},
	{0xC,SDLK_7},
	{0xD,SDLK_u},
	{0xE,SDLK_j},
	{0xF,SDLK_h}
};

int keaboard_symboltokeycode(SDLKey symbol)
{
	int i;
	int result = KEY_NOKEY;

	for(i = 0; i < NUMBEROFKEYS; i++)
	{
		if(keaboard_def[i].symbol == symbol)
			result = keaboard_def[i].keycode;
	}
	
	return result;
}

SDLKey keaboard_keycodetosymbol(int keycode)
{
	int i;
	SDLKey result = SDLK_END;

	for(i=0; i < NUMBEROFKEYS; i++)
		if(keaboard_def[i].keycode == keycode)
			result = keaboard_def[i].symbol;

	return result;

}

int keaboard_checkforkeypress(int keycode)
{
	int result = FALSE;
	uint8_t *keystates = SDL_GetKeyState(NULL);

	if(keystates[keaboard_keycodetosymbol(keycode)])
		result = TRUE;
	
	return result;
}


int keaboard_waitgetkeypress(void)
{
	int keycode = KEY_NOKEY;

	while(keycode == KEY_NOKEY)
	{
		if(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_KEYDOWN:
						keycode = keaboard_symboltokeycode(event.key.keysym.sym);
						break;
				default:
						break;
			
			}
		}
		SDL_Delay(20);
	}
	return keycode;
}

void system_initialization(struct syst *s)
{
	s->memory[0] = 0xf0;	//zero sprite
	s->memory[1] = 0x90;
	s->memory[2] = 0x90;
	s->memory[3] = 0x90;
	s->memory[4] = 0xf0;
	s->memory[5] = 0x20;	//one sprite
	s->memory[6] = 0x60;
	s->memory[7] = 0x20;
	s->memory[8] = 0x20;
	s->memory[9] = 0x70;
	s->memory[10] = 0xf0;	//two sprite
	s->memory[11] = 0x10;
	s->memory[12] = 0xf0;
	s->memory[13] = 0x80;
	s->memory[14] = 0xf0;
	s->memory[15] = 0xf0;	//three sprite
	s->memory[16] = 0x10;
	s->memory[17] = 0xf0;
	s->memory[18] = 0x10;
	s->memory[19] = 0xf0;
	s->memory[20] = 0x90;	//four sprite
	s->memory[21] = 0x90;
	s->memory[22] = 0xf0;
	s->memory[23] = 0x10;
	s->memory[24] = 0x10;
	s->memory[25] = 0xf0;	//five sprite
	s->memory[26] = 0x80;
	s->memory[27] = 0xf0;
	s->memory[28] = 0x10;
	s->memory[29] = 0xf0;
	s->memory[30] = 0xf0;	//six sprite
	s->memory[31] = 0x80;
	s->memory[32] = 0xf0;
	s->memory[33] = 0x90;
	s->memory[34] = 0xf0;
	s->memory[35] = 0xf0;	//seven sprite
	s->memory[36] = 0x10;
	s->memory[37] = 0x20;
	s->memory[38] = 0x40;
	s->memory[39] = 0x40;
	s->memory[40] = 0xf0;	//eight sprite
	s->memory[41] = 0x90;
	s->memory[42] = 0xf0;
	s->memory[43] = 0x90;
	s->memory[44] = 0xf0;
	s->memory[45] = 0xf0;	//nine sprite
	s->memory[46] = 0x90;
	s->memory[47] = 0xf0;
	s->memory[48] = 0x10;
	s->memory[49] = 0xf0;
	s->memory[50] = 0xf0;	//a sprite
	s->memory[51] = 0x90;
	s->memory[52] = 0xf0;
	s->memory[53] = 0x90;
	s->memory[54] = 0x90;
	s->memory[55] = 0xe0;	//b sprite
	s->memory[56] = 0x90;
	s->memory[57] = 0xe0;
	s->memory[58] = 0x90;
	s->memory[59] = 0xe0;
	s->memory[60] = 0xf0;	//c sprite
	s->memory[61] = 0x80;
	s->memory[62] = 0x80;
	s->memory[63] = 0x80;
	s->memory[64] = 0xf0;
	s->memory[65] = 0xe0;	//d sprite
	s->memory[66] = 0x90;
	s->memory[67] = 0x90;
	s->memory[68] = 0x90;
	s->memory[69] = 0xe0;
	s->memory[70] = 0xf0;	//e sprite
	s->memory[71] = 0x80;
	s->memory[72] = 0xf0;
	s->memory[73] = 0x80;
	s->memory[74] = 0xf0;
	s->memory[75] = 0xf0;	//f sprite
	s->memory[76] = 0x80;
	s->memory[77] = 0xf0;
	s->memory[78] = 0x80;
	s->memory[79] = 0X80;
//	printf("[sprites]:Initialized.............->OK\n");
	int i;

	for(i = 512; i < 4096; i++)
		s->memory[i] = 0;
	
	s->delay_timer = 0;
	s->sound_timer = 0;
	s->stack_pointer = -1;
	s->i = 0;
	s->opcode = 0;
	s->pc = 0x200;

	for(i = 0; i < 16; i++)
	{
		s->kb[i] = 0;
		s->stack[i] = 0;
	}
//	printf("[Stack]:Initialized................>OK\n");
//	printf("[Keaboard]:Initialized.............>OK\n");
	for(i = 0; i < 16; i++)
		s->V[i] = 0;
//	printf("[registers]:Initialized............>OK\n");
}

void load_on_memory(unsigned char *buffer,unsigned char *memory,int fsize)
{
	int i,j;
	printf("[LOAD ON MEMORY FUNCTION] fsize: %d\n",fsize);	
	for(i = 0,j=512; i < fsize; i++)	// in memory we do not store the spaces or new lince characters.
		if( (buffer[i] != ' ') && (buffer[i] != '\n') )		
			memory[j++] = buffer[i];
	chip.Wmem = j;		//saving value in Wmem.
}


uint32_t cpu_timerinterrupt(uint32_t interval,void *parameter)
{
	decrement_timers = TRUE;
	return interval;
}



int cpu_timerinit(void)
{
	int result = TRUE;

	SDL_InitSubSystem(SDL_INIT_TIMER);

	cpu_timer = SDL_AddTimer(17,cpu_timerinterrupt,NULL);

	if(cpu_timer == NULL)
	{
		printf("Error:couldn't create timers\n");
		result = FALSE;
	}
	
	return result;
}

void init_colors(SDL_Surface *surface)
{
	COLOR_BLACK = SDL_MapRGB(surface->format,0,0,0);
	COLOR_WHITE = SDL_MapRGB(surface->format,250,250,250);
	COLOR_DGREEN = SDL_MapRGB(surface->format,0,70,0);
	COLOR_LGREEN = SDL_MapRGB(surface->format,0,200,0);
	COLOR_TEXT.r = 255;
	COLOR_TEXT.g = 255;
	COLOR_TEXT.b = 255;
}

SDL_Surface * screen_create_surface(int width,int height,int alpha,uint32_t color_key)
{
	uint32_t rmask,gmask,bmask,amask;
	SDL_Surface *tempsurface,*newsurface;

	#if	SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
	#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
	#endif
	
	tempsurface = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA,screen_width,screen_height,SCREEN_DEPTH,rmask,gmask,bmask,amask);
	newsurface = SDL_DisplayFormat(tempsurface);

	if(newsurface == NULL)
	{
		printf("unable to create new surface\n");
		exit(1);
	}
	else
	{
		SDL_SetAlpha(newsurface,SDL_SRCALPHA,alpha);
		
		if(color_key != -1)
			SDL_SetColorKey(newsurface,SDL_SRCCOLORKEY,color_key);

		screen_clear(newsurface,COLOR_BLACK);

	}
	
	return newsurface;

}

int screen_init(void)
{
	int result = 0;	/*    false 	*/

	TTF_Init();

	message_font = TTF_OpenFont("VeraMono.ttf",11);

	screen_width = SCREEN_WIDTH * scale_factor;
	screen_height = SCREEN_HEIGHT * scale_factor;
	screen = SDL_SetVideoMode(screen_width,screen_height,SCREEN_DEPTH,SDL_SWSURFACE);

	if(screen == NULL)
	{
		printf("Error: %s\n",SDL_GetError());
		exit(1);	// ?
	}
	else
	{
		SDL_SetAlpha(screen,SDL_SRCALPHA,255);
		SDL_WM_SetCaption("CHIP-8 Emulator",NULL);
		init_colors(screen);
		virtscreen = screen_create_surface(screen_width,screen_height,255,-1);
		overlay = screen_create_surface(screen_width,screen_height,200,COLOR_BLACK);
		result = 1;
	}
	return ( (virtscreen != NULL) && (overlay != NULL) && result);
}

void screen_clear(SDL_Surface *surface,uint32_t color)
{
	SDL_Rect rect;

	rect.x = 0;
	rect.y = 0;

	rect.w = screen_width;
	rect.h = screen_height;

	SDL_FillRect(surface,&rect,color);

}

void screen_blank(void)
{


	screen_clear(virtscreen,COLOR_BLACK);

}

void screen_blit_surface(SDL_Surface *src,SDL_Surface *dest,int x,int y)
{
	SDL_Rect location;
	location.x = x;
	location.y = y;

	SDL_BlitSurface(src,NULL,dest,&location);

}

void screen_draw(int x,int y,int color)
{
	SDL_Rect pixel;
	uint32_t pixelcolor = COLOR_BLACK;

	pixel.x = x * scale_factor;
	pixel.y = y * scale_factor;
	pixel.w = pixel.h = scale_factor;

	if(color)
		pixelcolor = COLOR_WHITE;
	
	SDL_FillRect(virtscreen,&pixel,pixelcolor);

}

int screen_getpixel(int x,int y)
{
	uint8_t r,g,b;
	uint32_t color = 0;
	int pixelcolor = 0;

	x = x * scale_factor;
	y = y * scale_factor;

	uint32_t * pixels = (uint32_t *) virtscreen->pixels;
	uint32_t pixel = pixels[(virtscreen->w * y) + x];
	SDL_GetRGB(pixel,virtscreen->format,&r,&g,&b);
	color = SDL_MapRGB(virtscreen->format,r,g,b);

	if(color == COLOR_WHITE)
		pixelcolor = 1;

	return pixelcolor;

}

void screen_refresh(int overlay_on)
{
	screen_blit_surface(virtscreen,screen,0,0);
	if(overlay_on)
		screen_blit_surface(overlay,screen,0,0);

	screen_clear(overlay,COLOR_BLACK);
	SDL_UpdateRect(screen,0,0,0,0);
}

void print_MEM(struct syst *s)
{	//used it for debbuging to check what is written actually on memory after 512 index.
	int i;
	printf("written memory inspect.\n");
	for(i = 0; i <chip.Wmem; i++)
		printf("%c",chip.memory[512+i]);
}

void print_registers(void)
{
	int i;

	for(i = 0; i < 16; i++)
		printf("V[%3d] = %3d\n",i,chip.V[i]);
	printf("p_c: %7d\n",chip.pc);
	printf("I  : %7d\n",chip.i);
	printf("de.t: %4d\n",chip.delay_timer);
	printf("so.t: %4d\n",chip.sound_timer);
	//just for debbuging use.
}

void chip_timers()
{
	if(chip.delay_timer > 0)
		chip.delay_timer -= 1;
	if(chip.sound_timer > 0)
		chip.sound_timer -= 1;

	if(chip.sound_timer != 0 )
		printf("%c",7);
}

void cpu_process_sdl_events(void)
{
	if (SDL_PollEvent(&event))
	{
	    switch (event.type)
		{
        	case SDLK_ESCAPE:
                run_state  = 0;
                break;

            case SDL_KEYDOWN:
				
				if (event.key.keysym.sym == SDLK_ESCAPE)
   	 
					run_state = 0;
	
   				break;
    
	        default:
                break;
        }
    }
}


void operate()
{
	unsigned char tbyte;	// a temporary byte.
	char temp;
	int xcor;
	int ycor;
	int color;
	int currentcolor;
	int x,y,n,z;		// used on DXYN instruction.
	char ch,buffer[4],collect_data[2];
	int h1,h2,h3,j,reg_no,value = 0,t = 0;
	int normal_execution = 1;	// if true the program counter increments by 2 bytes to next instruction.
	int switcher;			// used for carry's borrow's cause 8bit int's wont hold them on overflow.
	
	for(j = 0; j < 2; j++)
	{
		collect_data[j] = chip.memory[chip.pc+t];
	//	printf("%x ",collect_data[j]);
		t++;
	}
	putchar('\n');


	buffer[0] = (collect_data[0] & 0xf0) >> 4;
	buffer[1] = collect_data[0] & 0x0f;
	buffer[2] = (collect_data[1] & 0xf0) >> 4;
	buffer[3] = collect_data[1] & 0x0f;
			
	printf("%x %x %x %x\n",buffer[0],buffer[1],buffer[2],buffer[3]);
	printf("%d %d %d %d\n",buffer[0],buffer[1],buffer[2],buffer[3]);
/*	fprintf(db,"%d  %x%x%x%x \n",chip.pc,buffer[0],buffer[1],buffer[2],buffer[3]);


	// For different endianess type.
	temp = buffer[0];
	buffer[0] = buffer[3];
	buffer[3] = temp;
	temp = buffer[2];
	buffer[2] = buffer[1];
	buffer[1] = temp;

	for(j = 0; j < 4; j++)
		printf("%x ",buffer[j]);
*/

	switch(buffer[0])
	{

		
		case 10:	//COSMAC_VIP instruction [ANNN : Sets I = NNN]
				printf("instr. Annn\n");
				h1 = buffer[1];
				h2 = buffer[2];
				h3 = buffer[3];
				
				chip.i = 16*16*h1+16*h2+h3;

				break;

				//COSMAC_VIP instruction [BMMM : jump to MMM + V0]
		case 11:
				printf("instr. bmmm\n");
				h1 = (buffer[1]);
				h2 = (buffer[2]);
				h3 = (buffer[3]);
				
				normal_execution = 0;
				
				chip.pc = 16*16*h1 + 16*h2 + h3 + chip.V[0];
				
				break;
		
		case 12:	//COSMAC_VIP instruction [CXKK : VX = rnd & kk]
				
				printf("instr.Cxkk\n");
				h1 = (buffer[2]);
				h2 = (buffer[3]);
				h3 = (buffer[1]);
				value = 16*h1+h2;
				srand(time(NULL));
				chip.V[h3] = ( rand() % 256 ) & value ;

				break;
		
		
		case 13:	//COSMAC_VIP instruction[DXYN : Draws on screen at (x,y) n bytes.

				x = buffer[1];
				y = buffer[2];
				n = buffer[3];

				chip.V[15] = 0;

				for( j = 0; j < n; j++)
				{	
					tbyte = chip.memory[chip.i + j];
					ycor = chip.V[y] + j;
					ycor = ycor % SCREEN_HEIGHT;

					for(z = 0; z < 8; z++)
					{
						xcor = chip.V[x] + z;
						xcor = xcor % SCREEN_WIDTH;

						color = (tbyte & 0x80) ? 1 : 0;
						currentcolor = screen_getpixel(xcor,ycor);
						chip.V[15] = (color & currentcolor) ? 1 : chip.V[15];
						color = color ^ currentcolor;
						screen_draw(xcor,ycor,color);

						tbyte = tbyte << 1 ;						
					}
					
				}

				screen_refresh(FALSE);

				break;
		case 14:	
				//COSMAC_VIP instruction [EX9E : Skips next instruction if VX == Hex_key ]
				if( (buffer[2] == 9) && ( (buffer[3] == 14)))
				{
					printf("instr. Ex9e\n");

					h1 = buffer[1];

					if(keaboard_checkforkeypress(chip.V[h1]))
						chip.pc +=2;	
				

					break;
				}

				//COSMAC_VIP instruction [EXA1 : Skips next instruction if VX != Hex_key ]
				if( (buffer[3] == 1) && ( (buffer[2] == 10) ))
				{
					printf("instr. ExA1\n");

					h1 = (buffer[1]);

					if( ! keaboard_checkforkeypress(chip.V[h1]))
						chip.pc +=2;	
				
					break;
				}





		case 15:	//COSMAC_VIP instruction [FX07 : Vx = delay_timer]
				if( (buffer[2] == 0) && (buffer[3] == 7))
				{
					printf("instr.fx07\n");
					h1 = buffer[1];
					chip.V[h1] = chip.delay_timer;

					break;
				}
				
				//COSMAC_VIP instruction [FX15 : Sets delay_timer = Vx]
				if( (buffer[2] == 1) && (buffer[3] == 5) )
				{
					printf("instr.fx15\n");
					h1 = buffer[1];
					chip.delay_timer = chip.V[h1];

					break;
				}
				
				//COSMAC_VIP instruction [FX18 : Sets sound_timer = Vx]
				if( (buffer[2] == 1) && (buffer[3] == 8) )
				{
					printf("instr. fx18\n");
					h1 = buffer[1];
					chip.sound_timer = chip.V[h1];

					break;
				}
				
				//COSMAC_VIP instruction [FX1E : Sets I = I + Vx]
				if( (buffer[2] == 1 ) && ( (buffer[3] == 14)))
				{
					printf("instr.fx1e\n");
					h1 = buffer[1];
					chip.i += chip.V[h1];

					break;
				}
				
				//COSMAC_VIP instruction [FX0A : execution stops.key pressed is saved to Vx.]
				if( (buffer[2] == 0) &&  (buffer[3] == 10) )
				{
					printf("instr.fx0a\n");
					h1 = buffer[1];
										
					chip.V[h1] = keaboard_waitgetkeypress();

					break;
				}

				//COSMAC_VIP instruction [FX29 : Loads on I the sprite address indicated to source register.
				if( (buffer[2] == 2) && (buffer[3] == 9) )
				{
					h1 = buffer[1];
					chip.i = chip.V[h1] * 5;		// each sprite 5 bytes long.

					break;
				}

				//COSMAC_VIP instruction [FX33 : BCD repr. of Vx, at m[i],m[i+1],m[i+2] ]

				if( (buffer[2] == 3) && (buffer[3] == 3))
				{
					printf("instr. fx33\n");
					
					h1 = buffer[1];
					h2 = chip.V[h1];

					chip.memory[chip.i] = h2/100;
					chip.memory[chip.i+1] = (h2 % 100)/10;
					chip.memory[chip.i+2] = (h2 % 100)%10;
					
					break;
				}
				
				//COSMAC_VIP instruction [FX55 : Store V0:VX -->M[i].. then i = i+x+1]
				if( (buffer[2] == 5) && (buffer[3] == 5))
				{
					printf("instr. fx55\n");
					
					h1 = buffer[1];
					
					for(j = 0; j <= h1; j++)
						chip.memory[chip.i + j] = chip.V[j];

					chip.i = chip.i + h1 + 1;

					break;
				}
			
				//COSMAC_VIP instruction [FX65 : load V0:VX -->M[i].. then i = i+x+1]
				if( (buffer[2] == 6) && (buffer[3] == 5))
				{
					printf("instr. fx65\n");
					
					h1 =buffer[1];
					
					for(j = 0; j <= h1; j++)
						chip.V[j] = chip.memory[chip.i + j];

					chip.i = chip.i + h1 + 1;

					break;
				}


		case 0:		//COSMAC_VIP instruction 00e0 clearing display.
				
				if( (buffer[2] == 14) && (buffer[3] != 14) )	
				{
					printf("Instr. clear_display\n");
					screen_blank();	
					break;
				}
				
				//COSMAC_VIP instruction [00EE :return from subroutine : RET]
				else if( (buffer[2] == 14) && (buffer[3] == 14) )
				{
					printf("instr. 00ee\n");
					normal_execution = 0;
					chip.pc = chip.stack[chip.stack_pointer];
					chip.pc +=2;
					chip.stack_pointer -=1;					
				
					break;
				}

		case 2:		//COSMAC_VIP instruction [2MMM : call subroutine at MMM : CALL addr]
				
				printf("instr. 2mmm\n");
				chip.stack_pointer += 1;
				chip.stack[chip.stack_pointer] = chip.pc;
				normal_execution = 0;
				h1 = buffer[1];
				h2 = buffer[2];
				h3 = buffer[3];
				chip.pc = 16*16*h1+16*h2+h3;
			//	chip.pc -= 4; // ? really?				
				break;
	
		case 3:		//COSMAC_VIP instruction [3XKK : skip next instr. if VX == KK]
				
				printf("instr. 3xKK\n");
				h1 = buffer[1];
				h2 = buffer[2];
				h3 = buffer[3];
				value = 16*h2 + h3;

				if(chip.V[h1] == value)
					chip.pc += 2;			

				break;
	
		case 4:		//COSMAC_VIP instruction [4XKK : skip next instr. if VX != KK]
				
				printf("instr.4xKK\n");
				h1 = buffer[1];
				h2 = buffer[2];
				h3 = buffer[3];
				value = 16*h2 + h3;

				if(chip.V[h1] != value)
					chip.pc += 2;			

				break;
		
		case 5:		//COSMAC_VIP instruction [5XY0 : Skip next inst. if VX == VY]
				
				printf("instr. 5xy0\n");
				h1 = buffer[1];
				h2 = buffer[2];
				
				if(chip.V[h1] == chip.V[h2])
					chip.pc += 2;	
				
				break;

		case 9:		//COSMAC_VIP instruction.[9XY0 : Skip next instr. if VX != VY]
				
				printf("instr. 9xy0\n");
				h1 = buffer[1];
				h2 = buffer[2];
				
				if(chip.V[h1] != chip.V[h2])
					chip.pc += 2;

				break;

		case 6:	//COSMAC_VIP instruction 6XKK : VX = KK		
		
				printf("instr. 6xkk\n");
				reg_no = buffer[1];
				h1 = 16*buffer[2];
				h2 = 1 *buffer[3];
				value = h1 + h2;
				chip.V[reg_no] = value;
				
				break;
		
		case 8:	
				if(buffer[3] ==0) //COSMAV_VIP instruction [8XY0 :  VX = VY]	
				{
					printf("instr. 8xy0\n");
					h1 = buffer[1];
					h2 = buffer[2];
					chip.V[h1] = chip.V[h2];
				
					break;
				}
				else if(buffer[3]==1) //COSMAC_VIP instruction [8XY1: VX = VX | VY]
				{
					printf("instr. 8xy1\n");
					h1 = buffer[1];
					h2 = buffer[2];
					chip.V[h1] = chip.V[h1] | chip.V[h2];
				
					break;				
				}
				else if(buffer[3] == 2) //COSMAC_VIP instruction [8XY2 : VX = VX & VY]
				{
					printf("instr 8xy2\n");
					h1 = buffer[1];
					h2 = buffer[2];
					chip.V[h1] = chip.V[h1] & chip.V[h2];
				
					break;
				}
				else if(buffer[3] == 3) //COSMAC_VIP instruction [8XY3 : VX = VX ^ VY]
				{
					printf("instr. 8xy3\n");
					h1 = buffer[1];
					h2 = buffer[2];
					chip.V[h1] = chip.V[h1] ^ chip.V[h2];
				
					break;
				}
				else if(buffer[3]==4) //COSMAC_VIP [8XY4: VX = VX + VY(if VX + VY <=ff VF = 00,else 01]
				{
					printf("instr.8xy4\n");
					h1 = buffer[1];
					h2 = buffer[2];
					value =16* 15+15; //255
					switcher = chip.V[h1] + chip.V[h2] ;
					chip.V[h1] += chip.V[h2];
				
					if(switcher <= 255)
						chip.V[15] = 0;
					else if(switcher > 255)
						chip.V[15] = 1;
				
					break;
				}
				else if(buffer[3]==5) //COSMAC_VIP[ 8XY5: VX = VX - VY(if Vx-Vy<0,VF=0,else 01]
				{
					printf("instr 8xy5\n");
					h1 = buffer[1];
					h2 = buffer[2];
					value = 255; //FF
					switcher = chip.V[h1] - chip.V[h2] ;
					chip.V[h1] -= chip.V[h2];
					
					if(switcher < 0 )
						chip.V[15] = 0;
					else
						chip.V[15] = 1;
					break;
				}
				else if(buffer[3] == 6) //COSMAC_VIP SHR Vx{,Vy}
				{
					printf("instr. 8xy6\n");
					h1 = buffer[1];
					h2 = buffer[2];
					
					if(chip.V[h1] % 2) //least-significant is 1
						chip.V[15] = 1;
					else
						chip.V[15] = 0;
					
					chip.V[h1] /= 2;
				
					break;
				}		
				else if(buffer[3] == 7)//COSMAC_VIP instr.[8xy7 : SUBN VX VY]
				{
					printf("instr 8xy7\n");
					h1 = buffer[1];
					h2 = buffer[2];
					switcher = chip.V[h2] - chip.V[h1];
					chip.V[h1] = chip.V[h2] - chip.V[h1];
					
					if(switcher < 0)
						chip.V[15] = 0;
					else 
						chip.V[15] = 1;
					break;
				}	
				else if( (buffer[3] == 14) ) // [8xyE : SHL Vx {,Vy} ]
				{
					printf("instr. 8xyE\n");
					h1 = (buffer[1]);
					switcher = chip.V[h1] & (0x80);	// if m.s.b is 1 ,switcher = 0x80	
					
					if(switcher == (0x80))
						chip.V[15] = 1;
					else
						chip.V[15] = 0;
					
					chip.V[h1] *= 2;
				
					break;
				}		
		case 7:		//COSMAC_VIP instruction 7XKK : [VX = VX + KK.]
				printf("instr: 7xkk\n");
				
				reg_no = buffer[1];
				h1 = 16*(buffer[2]);
				h2 = 1*(buffer[3]);
				value = h1+h2;
				chip.V[reg_no] += value;
				
				break;

		case 1:	
				//COSMAC_VIP instruction 1MMM : Go to MMM
					
				
					printf("instr. 1MMM\n");
					chip.pc =1* (buffer[3]) + 16*(buffer[2]) + 16*16*(buffer[1]) ;			  
					normal_execution = 0;
					break;
				
		default:
				printf("Uknown instruction.-->exiting\n");
				run_state = 0;
				
				break;
	}

	if(normal_execution)
		chip.pc += 2;		//incrementing program counter to show the next instruction.

	op_delay= 5;
	SDL_Delay(op_delay);	

	if (decrement_timers) 
	{	
    	chip.delay_timer -= (chip.delay_timer > 0) ? 1 : 0;
        chip.sound_timer -= (chip.sound_timer > 0) ? 1 : 0;
        decrement_timers = FALSE;
     }

	cpu_process_sdl_events();

	print_registers();	//debug purpose	
}

int loadrom(char *romfilename, int offset) 
{
    FILE *fp;
    int result = TRUE;

    fp = fopen(romfilename, "r");
    if (fp == NULL) {
        printf("Error: could not open ROM image: %s\n", romfilename);
        result = FALSE; 
    } 
    else {
        while (!feof(fp)) {
            fread(&chip.memory[offset], 1, 1, fp);
            offset++;
        } 
        fclose(fp);
    }
    return result;
}



int main(int argc,char *argv[])
{

	char *filename = NULL;

	scale_factor = SCALE_FACTOR-7;
	
	system_initialization(&chip);
	
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Fatal error: unable to initialize:%s\n",SDL_GetError());
		exit(1);
	}

	if( ! screen_init() )
	{
		printf("Fatal:error.. screen initialization.\n");
		SDL_Quit();
		exit(1);	
	}
	
	if( ! cpu_timerinit() )
	{
		printf("Fatal error: unable to initialize cpu_timers\n");
		SDL_Quit();
		exit(1);
	}
	
	printf("program starts at: %x(hex)\n",chip.pc);
	
	if(argc != 2)
	{
		printf("usage: argv[1] -->ROM\n");
		exit(1);
	}

	filename = argv[1];	
	
	loadrom(filename,ROM_DEFAULT);
	
	db = fopen("data_log","w+");

	while(run_state)
	{
		printf("program_counter: %d\n",chip.pc);
		operate();
	}

	
	fclose(db);
	
	SDL_Quit();

	return 0;

}
