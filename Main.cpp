//////////////////////////////////////////////////////////////////////////////////
// Project: Block Breaker (Breakout)
// File:    Main.cpp
//////////////////////////////////////////////////////////////////////////////////

// These three lines link in the required SDL components for our project. //
// Alternatively, we could have linked them in our project settings.      //
#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "SDLmain.lib")
#pragma comment(lib, "SDL_TTF.lib")

#include <string>    // We'll use the STL string for text output and for our file names
#include <fstream>   // We need to read in our levels from a file
#include "SDL/SDL.h"     // Main SDL header 
#include "SDL/SDL_TTF.h" // True Type Font header
#include "Defines.h" // Our defines header

using namespace std;   

// The STL stack can't take a function pointer as a type //
// so we encapsulate a function pointer within a struct. //
struct StateStruct
{
	void (*StatePointer)();
};

// The block just stores it's location and the amount of times it can be hit (health) //
struct Block
{
	SDL_Rect screen_location;  // location on screen
	SDL_Rect bitmap_location;  // location of image in bitmap 

	int num_hits;   // health
};

// The paddle only moves horizontally so there's no need for a y_speed variable //
struct Paddle
{
	SDL_Rect screen_location;  // location on screen
	SDL_Rect bitmap_location;  // location of image in bitmap 

	int x_speed;
};

// The ball moves in any direction so we need to have two speed variables //
struct Ball
{
	SDL_Rect screen_location;  // location on screen
	SDL_Rect bitmap_location;  // location of image in bitmap 

	int x_speed;
	int y_speed; 
};

#define MAX_STACK_SIZE     16

class StateStack {
  public:
	StateStack() : stack_size(0) {}
	bool empty() const {
		return (stack_size == 0);
	}
	const StateStruct& top() const {
		if (!empty())
			return pointers[stack_size - 1];
		return StateStruct();
	}
	void pop() {
		if (!empty())
			--stack_size;
	}
	void push(const StateStruct& ptr) {
		if (stack_size < MAX_STACK_SIZE)
			pointers[stack_size++] = ptr;
	}
  private:
	StateStruct pointers[MAX_STACK_SIZE];
	int stack_size;
};

// Global data //
StateStack		   g_StateStack;		 // Our state stack
SDL_Surface*       g_Bitmap = NULL;		 // Our background image
SDL_Surface*       g_Window = NULL;		 // Our backbuffer
SDL_Event		   g_Event;				 // An SDL event structure for input
int				   g_Timer;				 // Our timer is just an integer
Paddle             g_Player;			 // The player's paddle
Ball               g_Ball;				 // The game ball
int				   g_Lives;				 // Player's lives
int				   g_Level = 1;			 // Current level
int                g_NumBlocks = 0;      // Keep track of number of blocks
Block              g_Blocks[MAX_BLOCKS]; // The blocks we're breaking

// Functions to handle the states of the game //
void Menu();
void Game();
void Exit();
void GameWon();
void GameLost();

// Helper functions for the main game state functions //
void ClearScreen();
void DisplayText(string text, int x, int y, int size, int fR, int fG, int fB, int bR, int bG, int bB);
void HandleMenuInput();
void HandleGameInput();
void HandleExitInput();
void HandleWinLoseInput();

// Since there's only one paddle, we no longer need to pass the player's paddle //
// to CheckBallCollisions. The function will just assume that's what we mean.   //
bool CheckBallCollisions();
void CheckBlockCollisions();
void HandleBlockCollision(int index);
bool CheckPointInRect(int x, int y, SDL_Rect rect);
void HandleBall();
void MoveBall();
void HandleLoss();
void HandleWin();
void ChangeLevel();

// Init and Shutdown functions //
void Init();
void InitBlocks();
void Shutdown();

int main(int argc, char **argv)
{
	Init();
	
	// Our game loop is just a while loop that breaks when our state stack is empty. //
	while (!g_StateStack.empty())
	{
		g_StateStack.top().StatePointer();		
	}

	Shutdown();

	return 0;
}


// This function initializes our game. //
void Init()
{
	// Initiliaze SDL video and our timer. //
	SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER);
	// Setup our window's dimensions, bits-per-pixel (0 tells SDL to choose for us), //
	// and video format (SDL_ANYFORMAT leaves the decision to SDL). This function    //
	// returns a pointer to our window which we assign to g_Window.                  //
	g_Window = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 0, SDL_ANYFORMAT);    
	// Set the title of our window. //
	SDL_WM_SetCaption(WINDOW_CAPTION, 0);
	// Get the number of ticks since SDL was initialized. //
	g_Timer = SDL_GetTicks();

	// Initialize the player's data //
	// screen locations
	g_Player.screen_location.x = (WINDOW_WIDTH / 2) - (PADDLE_WIDTH / 2);   // center screen
	g_Player.screen_location.y = PLAYER_Y;
	g_Player.screen_location.w = PADDLE_WIDTH;
	g_Player.screen_location.h = PADDLE_HEIGHT;
	// image location 
	g_Player.bitmap_location.x = PADDLE_BITMAP_X;
	g_Player.bitmap_location.y = PADDLE_BITMAP_Y;
	g_Player.bitmap_location.w = PADDLE_WIDTH;
	g_Player.bitmap_location.h = PADDLE_HEIGHT;
	// player speed
	g_Player.x_speed   = PLAYER_SPEED;
	// lives 
	g_Lives = NUM_LIVES;

	// Initialize the ball's data //
	// screen location
	g_Ball.screen_location.x = (WINDOW_WIDTH / 2) - (BALL_DIAMETER / 2);   // center screen
	g_Ball.screen_location.y = (WINDOW_HEIGHT / 2) - (BALL_DIAMETER / 2);  // center screen
	g_Ball.screen_location.w = BALL_DIAMETER;
	g_Ball.screen_location.h = BALL_DIAMETER;
	// image location
	g_Ball.bitmap_location.x = BALL_BITMAP_X;
	g_Ball.bitmap_location.y = BALL_BITMAP_Y;
	g_Ball.bitmap_location.w = BALL_DIAMETER;
	g_Ball.bitmap_location.h = BALL_DIAMETER;	
	// speeds 	
	g_Ball.x_speed = 0;
	g_Ball.y_speed = 0;	

	// We'll need to initialize our blocks for each level, so we have a separate function handle it //
	InitBlocks();	

	// Fill our bitmap structure with information. //
	g_Bitmap = SDL_LoadBMP("data/BlockBreaker.bmp");	

	// Set our transparent color (magenta) //
	SDL_SetColorKey( g_Bitmap, SDL_SRCCOLORKEY, SDL_MapRGB(g_Bitmap->format, 255, 0, 255) );

	// We start by adding a pointer to our exit state, this way //
	// it will be the last thing the player sees of the game.   //
	StateStruct state;
	state.StatePointer = Exit;
	g_StateStack.push(state);

	// Then we add a pointer to our menu state, this will //
	// be the first thing the player sees of our game.    //
	state.StatePointer = Menu;
	g_StateStack.push(state);

	// Initialize the true type font library. //
	TTF_Init();
}

// This function determines which level to load and then iterates through the block structure //
// reading in values from the level file and setting up the blocks accordingly. //
void InitBlocks()
{
	fstream inFile;

	// The following code creates a string storing the proper file name. If   //
	// g_Level = 1, we get: "data\\level" + "1" + ".txt" = "data\\level1.txt" //
	char level_num[256];              // for itoa
	string file_name = "data\\level"; // the file will always start with "level"
	itoa(g_Level, level_num, 10);     // convert the level to a string
	file_name.append(level_num);      // append the level number
	file_name.append(".txt");         // we'll just use txt's for our levels

	// Open the file for input. Note that this function takes a    //
	// char* so we need to use the std::string's c_str() function. //
	inFile.open(file_name.c_str(), ios::in);  

	int index = 0; // used to index blocks in g_Blocks array

	// We'll first read in the number of hits a block has and then determine whether we //
	// should create the block (num_hits = 1-4) or if we should skip that block (0)     //
	int temp_hits;

	// Iterate through each row and column of blocks //
	for (int row=1; row<=NUM_ROWS; row++)
	{
		for (int col=1; col<=NUMCOLS; col++)
		{
			// Read the next value into temp_hits // 
			inFile >> temp_hits;

			// If temp_hits is zero, we go on to the next block //
			if (temp_hits != 0)
			{
				g_Blocks[index].num_hits = temp_hits;
			
				// We set the location of the block according to what row and column   //
				// we're on in our loop. Notice that we use BLOCK_SCREEN_BUFFER to set //
				// the blocks away from the sides of the screen. //
				g_Blocks[index].screen_location.x = col*BLOCK_WIDTH - BLOCK_SCREEN_BUFFER;
				g_Blocks[index].screen_location.y = row*BLOCK_HEIGHT + BLOCK_SCREEN_BUFFER;
				g_Blocks[index].screen_location.w = BLOCK_WIDTH;
				g_Blocks[index].screen_location.h = BLOCK_HEIGHT;
				g_Blocks[index].bitmap_location.w = BLOCK_WIDTH;
				g_Blocks[index].bitmap_location.h = BLOCK_HEIGHT;

				// Now we set the bitmap location rect according to num_hits //
				switch (g_Blocks[index].num_hits)
				{
					case 1:
					{
						g_Blocks[index].bitmap_location.x = YELLOW_X;
						g_Blocks[index].bitmap_location.y = YELLOW_Y;
					} break;
					case 2:
					{
						g_Blocks[index].bitmap_location.x = RED_X;
						g_Blocks[index].bitmap_location.y = RED_Y;
					} break;
					case 3:
					{
						g_Blocks[index].bitmap_location.x = GREEN_X;
						g_Blocks[index].bitmap_location.y = GREEN_Y;
					} break;
					case 4:
					{
						g_Blocks[index].bitmap_location.x = BLUE_X;
						g_Blocks[index].bitmap_location.y = BLUE_Y;
					} break;
				}

				// For future use, keep track of how many blocks we have. //
				g_NumBlocks++;
				index++;	// move to next block
			}
		}
	}

	inFile.close();	
}

// This function shuts down our game. //
void Shutdown()
{
	// Shutdown the true type font library. //
	TTF_Quit();

	// Free our surfaces. //
	SDL_FreeSurface(g_Bitmap);
	SDL_FreeSurface(g_Window);

	// Tell SDL to shutdown and free any resources it was using. //
	SDL_Quit();
}

// This function handles the game's main menu. From here //
// the player can select to enter the game, or quit.     //
void Menu()
{
	// Here we compare the difference between the current time and the last time we //
	// handled a frame. If FRAME_RATE amount of time has, it's time for a new frame. //
	if ( (SDL_GetTicks() - g_Timer) >= FRAME_RATE )
	{
		HandleMenuInput();

		// Make sure nothing from the last frame is still drawn. //
		ClearScreen();

		DisplayText("Start (G)ame", 350, 250, 12, 255, 255, 255, 0, 0, 0);
		DisplayText("(Q)uit Game",  350, 270, 12, 255, 255, 255, 0, 0, 0);
			
		// Tell SDL to display our backbuffer. The four 0's will make //
		// SDL display the whole screen. //
		SDL_UpdateRect(g_Window, 0, 0, 0, 0);

		// We've processed a frame so we now need to record the time at which we did it. //
		// This way we can compare this time the next time our function gets called and  //
		// see if enough time has passed between iterations. //
		g_Timer = SDL_GetTicks();
	}	
}

// This function handles the main game. We'll control the   //
// drawing of the game as well as any necessary game logic. //
void Game()
{	
	// Here we compare the difference between the current time and the last time we //
	// handled a frame. If FRAME_RATE amount of time has, it's time for a new frame. //
	if ( (SDL_GetTicks() - g_Timer) >= FRAME_RATE )
	{
		HandleGameInput();

		HandleBall();

		// Make sure nothing from the last frame is still drawn. //
		ClearScreen();

		// Draw the paddle and the ball //
		SDL_BlitSurface(g_Bitmap, &g_Player.bitmap_location, g_Window, &g_Player.screen_location);
		SDL_BlitSurface(g_Bitmap, &g_Ball.bitmap_location,   g_Window, &g_Ball.screen_location);

		// Iterate through the blocks array, drawing each block //
		for (int i=0; i<g_NumBlocks; i++)
		{
			SDL_BlitSurface(g_Bitmap, &g_Blocks[i].bitmap_location, g_Window, &g_Blocks[i].screen_location);
		}

		// Output the number of lives the player has left and the current level //
		char buffer[256];

		string lives = "Lives: ";
		itoa(g_Lives, buffer, 10);
		lives.append(buffer);

		string level = "Level: ";
		itoa(g_Level, buffer, 10);
		level.append(buffer);
		
		DisplayText(lives, LIVES_X, LIVES_Y, 12, 66, 239, 16, 0, 0, 0);		
		DisplayText(level, LEVEL_X, LEVEL_Y, 12, 66, 239, 16, 0, 0, 0);		

		// Tell SDL to display our backbuffer. The four 0's will make //
		// SDL display the whole screen. //
		SDL_UpdateRect(g_Window, 0, 0, 0, 0);

		// We've processed a frame so we now need to record the time at which we did it. //
		// This way we can compare this time the next time our function gets called and  //
		// see if enough time has passed between iterations. //
		g_Timer = SDL_GetTicks();
	}	
}

// This function handles the game's exit screen. It will display //
// a message asking if the player really wants to quit.          //
void Exit()
{	
	// Here we compare the difference between the current time and the last time we //
	// handled a frame. If FRAME_RATE amount of time has, it's time for a new frame. //
	if ( (SDL_GetTicks() - g_Timer) >= FRAME_RATE )
	{
		HandleExitInput();

		// Make sure nothing from the last frame is still drawn. //
		ClearScreen();

		DisplayText("Quit Game (Y or N)?", 350, 260, 12, 255, 255, 255, 0, 0, 0);

		// Tell SDL to display our backbuffer. The four 0's will make //
		// SDL display the whole screen. //
		SDL_UpdateRect(g_Window, 0, 0, 0, 0);

		// We've processed a frame so we now need to record the time at which we did it. //
		// This way we can compare this time the next time our function gets called and  //
		// see if enough time has passed between iterations. //
		g_Timer = SDL_GetTicks();
	}	
}

// Display a victory message. //
void GameWon()
{
	if ( (SDL_GetTicks() - g_Timer) >= FRAME_RATE )
	{
		HandleWinLoseInput();

		ClearScreen();

		DisplayText("You Win!!!", 350, 250, 12, 255, 255, 255, 0, 0, 0);
		DisplayText("Quit Game (Y or N)?", 350, 270, 12, 255, 255, 255, 0, 0, 0);

		SDL_UpdateRect(g_Window, 0, 0, 0, 0);

		g_Timer = SDL_GetTicks();
	}	
}

// Display a game over message. //
void GameLost()
{	
	if ( (SDL_GetTicks() - g_Timer) >= FRAME_RATE )
	{
		HandleWinLoseInput();

		ClearScreen();

		DisplayText("You Lose.", 350, 250, 12, 255, 255, 255, 0, 0, 0);
		DisplayText("Quit Game (Y or N)?", 350, 270, 12, 255, 255, 255, 0, 0, 0);

		SDL_UpdateRect(g_Window, 0, 0, 0, 0);

		g_Timer = SDL_GetTicks();
	}	
}

// This function simply clears the back buffer to black. //
void ClearScreen()
{
	// This function just fills a surface with a given color. The //
	// first 0 tells SDL to fill the whole surface. The second 0  //
	// is for black. //
	SDL_FillRect(g_Window, 0, 0);
}

// This function displays text to the screen. It takes the text //
// to be displayed, the location to display it, the size of the //
// text, and the color of the text and background.              //
void DisplayText(string text, int x, int y, int size, int fR, int fG, int fB, int bR, int bG, int bB) 
{
	// Open our font and set its size to the given parameter. //
    TTF_Font* font = TTF_OpenFont("arial.ttf", size);

	SDL_Color foreground  = { fR, fG, fB};   // Text color. //
	SDL_Color background  = { bR, bG, bB };  // Color of what's behind the text. //

	// This renders our text to a temporary surface. There //
	// are other text functions, but this one looks nice.  //
	SDL_Surface* temp = TTF_RenderText_Shaded(font, text.c_str(), foreground, background);

	// A structure storing the destination of our text. //
	SDL_Rect destination = { x, y, 0, 0 };
	
	// Blit the text surface to our window surface. //
	SDL_BlitSurface(temp, NULL, g_Window, &destination);

	// Always free memory! //
	SDL_FreeSurface(temp);

	// Close the font. //
	TTF_CloseFont(font);
}

// This function receives player input and //
// handles it for the game's menu screen.  //
void HandleMenuInput() 
{
	// Fill our event structure with event information. //
	if ( SDL_PollEvent(&g_Event) )
	{
		// Handle user manually closing game window //
		if (g_Event.type == SDL_QUIT)
		{			
			// While state stack isn't empty, pop //
			while (!g_StateStack.empty())
			{
				g_StateStack.pop();
			}

			return;  // game is over, exit the function
		}

		// Handle keyboard input here //
		if (g_Event.type == SDL_KEYDOWN)
		{
			if (g_Event.key.keysym.sym == SDLK_ESCAPE)
			{
				g_StateStack.pop();
				return;  // this state is done, exit the function 
			}
			// Quit //
			if (g_Event.key.keysym.sym == SDLK_q)
			{
				g_StateStack.pop();
				return;  // game is over, exit the function 
			}
			// Start Game //
			if (g_Event.key.keysym.sym == SDLK_g)
			{
				StateStruct temp;
				temp.StatePointer = Game;
				g_StateStack.push(temp);
				return;  // this state is done, exit the function 
			}
		}
	}
}

// This function receives player input and //
// handles it for the main game state.     //
void HandleGameInput() 
{
	static bool left_pressed  = false;
	static bool right_pressed = false;

	// Fill our event structure with event information. //
	if ( SDL_PollEvent(&g_Event) )
	{
		// Handle user manually closing game window //
		if (g_Event.type == SDL_QUIT)
		{			
			// While state stack isn't empty, pop //
			while (!g_StateStack.empty())
			{
				g_StateStack.pop();
			}

			return;  // game is over, exit the function
		}

		// Handle keyboard input here //
		if (g_Event.type == SDL_KEYDOWN)
		{
			if (g_Event.key.keysym.sym == SDLK_ESCAPE)
			{
				g_StateStack.pop();
				
				return;  // this state is done, exit the function 
			}	
			if (g_Event.key.keysym.sym == SDLK_SPACE)
			{
				// Player can hit 'space' to make the ball move at start //
				if (g_Ball.y_speed == 0)
					g_Ball.y_speed = BALL_SPEED_Y;
			}
			if (g_Event.key.keysym.sym == SDLK_LEFT)
			{
				left_pressed = true;
			}
			if (g_Event.key.keysym.sym == SDLK_RIGHT)
			{
				right_pressed = true;
			}
		}
		if (g_Event.type == SDL_KEYUP)
		{
			if (g_Event.key.keysym.sym == SDLK_LEFT)
			{
				left_pressed = false;
			}
			if (g_Event.key.keysym.sym == SDLK_RIGHT)
			{
				right_pressed = false;
			}
		}		
	}

	// This is where we actually move the paddle //
	if (left_pressed)
	{
		// Notice that we do this here now instead of in a separate function //
		if ( (g_Player.screen_location.x - PLAYER_SPEED) >= 0 )
		{
			g_Player.screen_location.x -= PLAYER_SPEED;
		}
	}
	if (right_pressed)
	{
		if ( (g_Player.screen_location.x + PLAYER_SPEED) <= WINDOW_WIDTH )
		{
			g_Player.screen_location.x += PLAYER_SPEED;
		}
	}
}

// This function receives player input and //
// handles it for the game's exit screen.  //
void HandleExitInput() 
{
	// Fill our event structure with event information. //
	if ( SDL_PollEvent(&g_Event) )
	{
		// Handle user manually closing game window //
		if (g_Event.type == SDL_QUIT)
		{			
			// While state stack isn't empty, pop //
			while (!g_StateStack.empty())
			{
				g_StateStack.pop();
			}

			return;  // game is over, exit the function
		}

		// Handle keyboard input here //
		if (g_Event.type == SDL_KEYDOWN)
		{
			if (g_Event.key.keysym.sym == SDLK_ESCAPE)
			{
				g_StateStack.pop();
				
				return;  // this state is done, exit the function 
			}
			// Yes //
			if (g_Event.key.keysym.sym == SDLK_y)
			{
				g_StateStack.pop();
				return;  // game is over, exit the function 
			}
			// No //
			if (g_Event.key.keysym.sym == SDLK_n)
			{
				StateStruct temp;
				temp.StatePointer = Menu;
				g_StateStack.push(temp);
				return;  // this state is done, exit the function 
			}
		}
	}
}

// Input handling for win/lose screens. //
void HandleWinLoseInput()
{
	if ( SDL_PollEvent(&g_Event) )
	{
		// Handle user manually closing game window //
		if (g_Event.type == SDL_QUIT)
		{			
			// While state stack isn't empty, pop //
			while (!g_StateStack.empty())
			{
				g_StateStack.pop();
			}

			return;  
		}

		// Handle keyboard input here //
		if (g_Event.type == SDL_KEYDOWN)
		{
			if (g_Event.key.keysym.sym == SDLK_ESCAPE)
			{
				g_StateStack.pop();
				
				return;  
			}
			if (g_Event.key.keysym.sym == SDLK_y)
			{
				g_StateStack.pop();
				return;  
			}
			// If player chooses to continue playing, we pop off    //
			// current state and push exit and menu states back on. //
			if (g_Event.key.keysym.sym == SDLK_n)
			{
				g_StateStack.pop();

				StateStruct temp;
				temp.StatePointer = Exit;
				g_StateStack.push(temp);

				temp.StatePointer = Menu;
				g_StateStack.push(temp);
				return;  
			}
		}
	}
}

// Check to see if the ball is going to hit the paddle //
bool CheckBallCollisions()
{
	// Temporary values to keep things tidy //
	int ball_x      = g_Ball.screen_location.x;
	int ball_y      = g_Ball.screen_location.y;
	int ball_width  = g_Ball.screen_location.w;
	int ball_height = g_Ball.screen_location.h;
	int ball_speed  = g_Ball.y_speed;

	int paddle_x      = g_Player.screen_location.x;
	int paddle_y      = g_Player.screen_location.y;	
	int paddle_width  = g_Player.screen_location.w;
	int paddle_height = g_Player.screen_location.h;

	// Check to see if ball is in Y range of the player's paddle. //
	// We check its speed to see if it's even moving towards the player's paddle. //
	if ( (ball_speed > 0) && (ball_y + ball_height >= paddle_y) && 
		 (ball_y + ball_height <= paddle_y + paddle_height) )        // side hit
	{
		// If ball is in the X range of the paddle, return true. //
		if ( (ball_x <= paddle_x + paddle_width) && (ball_x + ball_width >= paddle_x) )
		{
			return true;
		}
	}

	return false;
}

// This function checks to see if the ball has hit one of the blocks. It also checks  //
// what part of the ball hit the block so we can adjust the ball's speed acoordingly. //
void CheckBlockCollisions()
{
	// collision points 
	int left_x   = g_Ball.screen_location.x;
	int left_y   = g_Ball.screen_location.y + g_Ball.screen_location.h/2;
	int right_x  = g_Ball.screen_location.x + g_Ball.screen_location.w;
	int right_y  = g_Ball.screen_location.y + g_Ball.screen_location.h/2;
	int top_x    = g_Ball.screen_location.x + g_Ball.screen_location.w/2;
	int top_y    = g_Ball.screen_location.y;
	int bottom_x = g_Ball.screen_location.x + g_Ball.screen_location.w/2;
	int bottom_y = g_Ball.screen_location.y + g_Ball.screen_location.h;

	bool top = false;
	bool bottom = false;
	bool left = false;
	bool right = false;

	for (int block=0; block<g_NumBlocks; block++)
	{
		// top //
		if ( CheckPointInRect(top_x, top_y, g_Blocks[block].screen_location) )
		{
			top = true;
			HandleBlockCollision(block);
		}
		// bottom //
		if ( CheckPointInRect(bottom_x, bottom_y, g_Blocks[block].screen_location) )
		{
			bottom = true;
			HandleBlockCollision(block);
		}
		// left //
		if ( CheckPointInRect(left_x, left_y, g_Blocks[block].screen_location) )
		{
			left = true;
			HandleBlockCollision(block);
		}
		// right //
		if ( CheckPointInRect(right_x, right_y, g_Blocks[block].screen_location) )
		{
			right = true;
			HandleBlockCollision(block);
		}
	}

	if (top)
	{
		g_Ball.y_speed = -g_Ball.y_speed;
		g_Ball.screen_location.y += BALL_DIAMETER;
	}
	if (bottom)
	{
		g_Ball.y_speed = -g_Ball.y_speed;
		g_Ball.screen_location.y -= BALL_DIAMETER;
	}
	if (left)
	{
		g_Ball.x_speed = -g_Ball.x_speed;
		g_Ball.screen_location.x += BALL_DIAMETER;
	}
	if (right)
	{
		g_Ball.x_speed = -g_Ball.x_speed;
		g_Ball.screen_location.x -= BALL_DIAMETER;
	}
}

// This function changes the block's hit count. We also need it to change  //
// the color of the block and check to see if the hit count reached zero.  //
void HandleBlockCollision(int index)
{
	g_Blocks[index].num_hits--;

	// If num_hits is 0, the block needs to be erased //
	if (g_Blocks[index].num_hits == 0)
	{
		// Since order isn't important in our block array, we can quickly erase a block  //
		// simply by copying the last block into the deleted block's space. Note that we //
		// have to decrement the block count so we don't keep trying to access the       //
		// last block (the one we just moved). //
		g_Blocks[index] = g_Blocks[g_NumBlocks-1];
		g_NumBlocks--;

		// Check to see if it's time to change the level //
		if (g_NumBlocks == 0)
		{
			ChangeLevel();			
		}
	}
	// If the hit count hasn't reached zero, we need to change the block's color //
	else
	{
		switch (g_Blocks[index].num_hits)
		{
			case 1:
			{
				g_Blocks[index].bitmap_location.x = YELLOW_X;
				g_Blocks[index].bitmap_location.y = YELLOW_Y;
			} break;
			case 2:
			{
				g_Blocks[index].bitmap_location.x = RED_X;
				g_Blocks[index].bitmap_location.y = RED_Y;
			} break;
			case 3:
			{
				g_Blocks[index].bitmap_location.x = GREEN_X;
				g_Blocks[index].bitmap_location.y = GREEN_Y;
			} break;
		}
	}
}

// Check to see if a point is within a rectangle //
bool CheckPointInRect(int x, int y, SDL_Rect rect)
{
	if ( (x >= rect.x) && (x <= rect.x + rect.w) && 
		 (y >= rect.y) && (y <= rect.y + rect.h) )
	{
		return true;
	}

	return false;
}

void ChangeLevel()
{
	g_Level++;

	// Check to see if the player has won //
	if (g_Level > NUM_LEVELS)
	{
		HandleWin();
		return;
	}

	// Reset the ball //
	g_Ball.x_speed = 0;
	g_Ball.y_speed = 0;

	g_Ball.screen_location.x = WINDOW_WIDTH/2 - g_Ball.screen_location.w/2;
	g_Ball.screen_location.y = WINDOW_HEIGHT/2 - g_Ball.screen_location.h/2;

	g_NumBlocks = 0; // Set this to zero before calling InitBlocks()
	InitBlocks();    // InitBlocks() will load the proper level 
}

void HandleBall()
{	
	// Start by moving the ball //
	MoveBall();	

	if ( CheckBallCollisions() )	
	{
		// Get center location of paddle //
	    int paddle_center = g_Player.screen_location.x + g_Player.screen_location.w / 2;
		int ball_center = g_Ball.screen_location.x + g_Ball.screen_location.w / 2;

		// Find the location on the paddle that the ball hit //
		int paddle_location = ball_center - paddle_center;

		// Increase X speed according to distance from center of paddle. //
		g_Ball.x_speed = paddle_location / BALL_SPEED_MODIFIER;
		g_Ball.y_speed = -g_Ball.y_speed;
	}

	// Check for collisions with blocks //
	CheckBlockCollisions();
}

void MoveBall()
{
	g_Ball.screen_location.x += g_Ball.x_speed;
	g_Ball.screen_location.y += g_Ball.y_speed;	

	// If the ball is moving left, we see if it hits the wall. If does, //
	// we change its direction. We do the same thing if it's moving right. //
	if ( ( (g_Ball.x_speed < 0) && (g_Ball.screen_location.x <= 0)  ) || 
		 ( (g_Ball.x_speed > 0) &&
		   (g_Ball.screen_location.x + g_Ball.screen_location.w >= WINDOW_WIDTH) ) )
	{
		g_Ball.x_speed = -g_Ball.x_speed;
	}

	// If the ball is moving up, we should check to see if it hits the 'roof' //
	if ( (g_Ball.y_speed < 0) && (g_Ball.screen_location.y <= 0) )
	{
		g_Ball.y_speed = -g_Ball.y_speed;
	}

	// Check to see if ball has passed the player //
	if ( g_Ball.screen_location.y  >= WINDOW_HEIGHT )
	{
		g_Lives--;

		g_Ball.x_speed = 0;
		g_Ball.y_speed = 0;

		g_Ball.screen_location.x = WINDOW_WIDTH/2 - g_Ball.screen_location.w/2;
		g_Ball.screen_location.y = WINDOW_HEIGHT/2 - g_Ball.screen_location.h/2;

		if (g_Lives == 0)
		{
			HandleLoss();
		}
	}
}

void HandleLoss()
{
	while ( !g_StateStack.empty() )
	{
		g_StateStack.pop();
	}	

	g_Ball.x_speed = 0;
	g_Ball.y_speed = 0;

	g_Ball.screen_location.x = WINDOW_WIDTH/2 - g_Ball.screen_location.w/2;
	g_Ball.screen_location.y = WINDOW_HEIGHT/2 - g_Ball.screen_location.h/2;

	g_Lives = NUM_LIVES;
	g_NumBlocks = 0;
	g_Level = 1;
	InitBlocks();

	StateStruct temp;
	temp.StatePointer = GameLost;
	g_StateStack.push(temp);

}

void HandleWin()
{
	while ( !g_StateStack.empty() )
	{
		g_StateStack.pop();
	}	

	g_Ball.x_speed = 0;
	g_Ball.y_speed = 0;

	g_Ball.screen_location.x = WINDOW_WIDTH/2 - g_Ball.screen_location.w/2;
	g_Ball.screen_location.y = WINDOW_HEIGHT/2 - g_Ball.screen_location.h/2;

	g_Lives = NUM_LIVES;
	g_NumBlocks = 0;
	g_Level = 1;
	InitBlocks();

	StateStruct temp;
	temp.StatePointer = GameWon;
	g_StateStack.push(temp);
}

//  Aaron Cox, 2004 //
