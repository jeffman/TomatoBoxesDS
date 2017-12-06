#include <nds.h>
#include <nds\arm9\math.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "data.h"

#define timers2ms(tlow,thigh)(tlow | (thigh<<16)) >> 5

int maxx = 256;
int maxy = 192;

uint8 matox = 0;
uint8 matoy = 0;
uint8 matodir = 5;

uint8 mapcopy[15][20];
uint8 currmap = 0;

uint32 moves = 0;

char undo[4096];
bool mflag[4096];
uint16 upos = 0;

bool doScores = false;
bool thinger = false;

uint8* srm = SRAM;

void StartTicks(void)
{
   TIMER0_DATA=0;
   TIMER1_DATA=0;
   TIMER0_CR=TIMER_ENABLE|TIMER_DIV_1024;
   TIMER1_CR=TIMER_ENABLE|TIMER_CASCADE;
}

uint32 GetTicks(void)
{
   return timers2ms(TIMER0_DATA, TIMER1_DATA);
}

void Delay(uint32 ms)
{
   uint32 now;
   now=timers2ms(TIMER0_DATA, TIMER1_DATA);
   while((uint32)timers2ms(TIMER0_DATA, TIMER1_DATA)<now+ms);

}

// Math functions, because devKitPro doesn't have a standard math file for some reason
int round(float n) {
	return (int)n;
}

int abs(int n) {
	if (n < 0)
		return (n * -1);
	return n;
}

float absf(float n) {
	if (n < 0)
		return (n * -1);
	return n;
}
int RandomInt(int inMin, int inMax)
{
	int difference = inMax - inMin;
	return ( rand() % (difference + 1) ) + inMin;
}

// Screen drawing functions
void DrawRect(int x, int y, int w, int h, uint16 color)
{
  uint16* buffer = VRAM_A;
  buffer += y * maxx + x;
  for(int i = 0; i < h; ++i) {
    uint16* line = buffer + (maxx * i);
    for(int j = 0; j < w; ++j) {
      *line++ = color;
    }
  }
}

void DrawPixel(int x, int y, int c){
	uint16* buffer = y * maxx + x + VRAM_A;
	*buffer = c | BIT(15);
}

void DrawLine(int x1, int y1, int x2, int y2, int c)
{
	float dx = (float)(x2 - x1);
	float dy = (float)(y2 - y1);
	float x = (float)x1; float y = (float)y1;
	if (absf(dy) < absf(dx)){
		int stepx = (dx > 0) ? 1 : -1;
		float slope = dy / dx * stepx;
		while (x != x2){
			x += stepx; y += slope;
			DrawPixel((int)x, (int)y, c);
		}
	} else {
		int stepy = (dy > 0) ? 1 : -1;
		float slope = dx / dy * stepy;
		while (y != y2){
			y += stepy; x += slope;
			DrawPixel((int)x, (int)y, c);
		}
	}
}

void DrawTile(int t, int x, int y)
{
	//if (t == 4)
	//{
	//}
	//else
	{
		for (int j = 0; j < 10; j++)
		{
			for (int i = 0; i < 10; i++)
			{
				DrawPixel(x + i, y + j, pal[tile[tilemap[t]][j][i]]);
			}
		}
	}
}

void DrawMap(int x, int y)
{
	for (int j = 0; j < yt; j++)
	{
		for (int i = 0; i < xt; i++)
		{
			DrawTile(mapcopy[j][i], x + (i * 10), y + (j * 10));
		}
	}
}

void DrawMato()
{
		for (int j = 0; j < 10; j++)
		{
			for (int i = 0; i < 10; i++)
			{
				DrawPixel((matox * 10) + i + xoff, (matoy * 10) + j + yoff, pal[tile[matodir][j][i]]);
			}
		}
}

void CopyMap(int m)
{
	for (int j = 0; j < yt; j++)
	{
		for (int i = 0; i < xt; i++)
		{
			mapcopy[j][i] = map[m][j][i];
		}
	}
}

bool WaitKey(int k)
{
	scanKeys();
	if (keysDown() & k)
		return true;
	return false;
}

void ClearScreen(void)
{    
    printf("\033[2J");
	DrawRect(0, 0, maxx, maxy, 0);
}

bool CheckComplete()
{
	for (int j = 0; j < yt; j++)
	{
		for (int i = 0; i < xt; i++)
		{
			if (mapcopy[j][i] == 3)
			{
				return false;
			}
		}
	}
	return true;
}

void GetMatoPos(void)
{
	for (int j = 0; j < yt; j++)
	{
		for (int i = 0; i < xt; i++)
		{
			if (mapcopy[j][i] == 4)
			{
				matox = i;
				matoy = j;
				return;
			}
		}
	}
}

void AddUndo(char c, bool f)
{
	doScores = true;
	if (upos == 4095)
		return;
	undo[upos] = c;
	mflag[upos] = f;
	upos++;
}

void ResetUndo(void)
{
	for (int i = 0; i < 4096; i++)
		undo[i] = (char)0;
	upos = 0;
}

bool CheckInput(void)
{
	doScores = false;

	scanKeys();
	
	// Exit
	if (keysDown() & KEY_START)
	{
		thinger = true;
		return true;
	}

	// UP //
	if (keysDown() & KEY_UP)
	{
		matodir = 6;

		int upTile = mapcopy[matoy - 1][matox];

			// Check if we're upping a box
			if (upTile == 2 || upTile == 5)
			{
				// Going up on a box
				int upBox = mapcopy[matoy - 2][matox];
				
				// Check if there's anything behind the box
				if (upBox == 0 || upBox == 3 || upBox == 4)
				{
					// Succesful move; there's nothing behind the box

					// Move the box over the new tile
					mapcopy[matoy - 2][matox] = (upBox == 3) ? 5 : 2;

					// Move the box off of the old tile
					mapcopy[matoy - 1][matox] = (upTile == 2) ? 0 : 3;

					// Move Mr. Mato
					matoy--;
					moves++;
					AddUndo('U', true);

					// Update the map
					DrawTile(mapcopy[matoy + 1][matox], (matox * 10) + xoff, ((matoy + 1) * 10) + yoff);
					DrawTile(mapcopy[matoy - 1][matox], (matox * 10) + xoff, ((matoy - 1) * 10) + yoff);

				}
			}

			// Check if we're upping nothing
			else if (upTile == 0 || upTile == 3 || upTile == 4)
			{
				// Move to the new place
				matoy--;
				moves++;
				AddUndo('U', false);
				DrawTile(mapcopy[matoy + 1][matox], (matox * 10) + xoff, ((matoy + 1) * 10) + yoff);

			}

	}

	// DOWN //
	if (keysDown() & KEY_DOWN)
	{
		matodir = 5;

		int upTile = mapcopy[matoy + 1][matox];
		
		// Check if we're downing a box
		if (upTile == 2 || upTile == 5)
		{
			// Going down on a box
			int upBox = mapcopy[matoy + 2][matox];
			
			// Check if there's anything behind the box
			if (upBox == 0 || upBox == 3 || upBox == 4)
			{
				// Succesful move; there's nothing behind the box

				// Move the box over the new tile
				mapcopy[matoy + 2][matox] = (upBox == 3) ? 5 : 2;

				// Move the box off of the old tile
				mapcopy[matoy + 1][matox] = (upTile == 2) ? 0 : 3;

				// Move Mr. Mato
				matoy++;
				moves++;
				AddUndo('D', true);

				// Update the map
				DrawTile(mapcopy[matoy + 1][matox], (matox * 10) + xoff, ((matoy + 1) * 10) + yoff);
				DrawTile(mapcopy[matoy - 1][matox], (matox * 10) + xoff, ((matoy - 1) * 10) + yoff);

			}
		}

		// Check if we're downing nothing
		else if (upTile == 0 || upTile == 3 || upTile == 4)
		{
			// Move to the new place
			matoy++;
			moves++;
			AddUndo('D', false);
			DrawTile(mapcopy[matoy - 1][matox], (matox * 10) + xoff, ((matoy - 1) * 10) + yoff);
		}
	}

	// LEFT //
	if (keysDown() & KEY_LEFT)
	{
		matodir = 7;

		int upTile = mapcopy[matoy][matox - 1];
		
		// Check if we're lefting a box
		if (upTile == 2 || upTile == 5)
		{
			// Going left on a box
			int upBox = mapcopy[matoy][matox - 2];
			
			// Check if there's anything behind the box
			if (upBox == 0 || upBox == 3 || upBox == 4)
			{
				// Succesful move; there's nothing behind the box

				// Move the box over the new tile
				mapcopy[matoy][matox - 2] = (upBox == 3) ? 5 : 2;

				// Move the box off of the old tile
				mapcopy[matoy][matox - 1] = (upTile == 2) ? 0 : 3;

				// Move Mr. Mato
				matox--;
				moves++;
				AddUndo('L', true);

				// Update the map
				DrawTile(mapcopy[matoy][matox + 1], ((matox + 1) * 10) + xoff, (matoy * 10) + yoff);
				DrawTile(mapcopy[matoy][matox - 1], ((matox - 1) * 10) + xoff, (matoy * 10) + yoff);

			}
		}

		// Check if we're lefting nothing
		else if (upTile == 0 || upTile == 3 || upTile == 4)
		{
			// Move to the new place
			matox--;
			moves++;
			AddUndo('L', false);
			DrawTile(mapcopy[matoy][matox + 1], ((matox + 1) * 10) + xoff, (matoy * 10) + yoff);
		}
	}

	// RIGHT //
	if (keysDown() & KEY_RIGHT)
	{
		matodir = 8;

		int upTile = mapcopy[matoy][matox + 1];
		
		// Check if we're righting a box
		if (upTile == 2 || upTile == 5)
		{
			// Going right on a box
			int upBox = mapcopy[matoy][matox + 2];
			
			// Check if there's anything behind the box
			if (upBox == 0 || upBox == 3 || upBox == 4)
			{
				// Succesful move; there's nothing behind the box

				// Move the box over the new tile
				mapcopy[matoy][matox + 2] = (upBox == 3) ? 5 : 2;

				// Move the box off of the old tile
				mapcopy[matoy][matox + 1] = (upTile == 2) ? 0 : 3;

				// Move Mr. Mato
				matox++;
				moves++;
				AddUndo('R', true);

				// Update the map
				DrawTile(mapcopy[matoy][matox + 1], ((matox + 1) * 10) + xoff, (matoy * 10) + yoff);
				DrawTile(mapcopy[matoy][matox - 1], ((matox - 1) * 10) + xoff, (matoy * 10) + yoff);

			}
		}

		// Check if we're rightingting nothing
		else if (upTile == 0 || upTile == 3 || upTile == 4)
		{
			// Move to the new place
			matox++;
			moves++;
			AddUndo('R', false);
			DrawTile(mapcopy[matoy][matox - 1], ((matox - 1) * 10) + xoff, (matoy * 10) + yoff);
		}
	}

	// L // (UNDO)
	if ((keysDown() & KEY_L) && upos > 0)
	{
		char c = undo[upos - 1];
		upos--;
		moves--;

		doScores = true;

		if (c == 'U')
		{
			matodir = 6;

			int upTile = mapcopy[matoy - 1][matox];
			int onTile = mapcopy[matoy][matox];

			if (mflag[upos])
			{
				// Box
				mapcopy[matoy - 1][matox] = (upTile == 2) ? 0 : 3;
				mapcopy[matoy][matox] = (onTile == 3) ? 5 : 2;
			}

			DrawTile(mapcopy[matoy - 1][matox], (matox * 10) + xoff, ((matoy - 1) * 10) + yoff);
			DrawTile(mapcopy[matoy][matox], (matox * 10) + xoff, (matoy * 10) + yoff);
			DrawTile(mapcopy[matoy + 1][matox], (matox * 10) + xoff, ((matoy + 1) * 10) + yoff);
			matoy++;
		}
		else if (c == 'D')
		{
			matodir = 5;

			int upTile = mapcopy[matoy + 1][matox];
			int onTile = mapcopy[matoy][matox];

			if (mflag[upos])
			{
				// Box
				mapcopy[matoy + 1][matox] = (upTile == 2) ? 0 : 3;
				mapcopy[matoy][matox] = (onTile == 3) ? 5 : 2;
			}

			DrawTile(mapcopy[matoy - 1][matox], (matox * 10) + xoff, ((matoy - 1) * 10) + yoff);
			DrawTile(mapcopy[matoy][matox], (matox * 10) + xoff, (matoy * 10) + yoff);
			DrawTile(mapcopy[matoy + 1][matox], (matox * 10) + xoff, ((matoy + 1) * 10) + yoff);
			matoy--;
		}
		else if (c == 'L')
		{
			matodir = 7;

			int upTile = mapcopy[matoy][matox - 1];
			int onTile = mapcopy[matoy][matox];

			if (mflag[upos])
			{
				// Box
				mapcopy[matoy][matox - 1] = (upTile == 2) ? 0 : 3;
				mapcopy[matoy][matox] = (onTile == 3) ? 5 : 2;
			}

			DrawTile(mapcopy[matoy][matox - 1], ((matox - 1) * 10) + xoff, (matoy * 10) + yoff);
			DrawTile(mapcopy[matoy][matox], (matox * 10) + xoff, (matoy * 10) + yoff);
			DrawTile(mapcopy[matoy][matox + 1], ((matox + 1) * 10) + xoff, (matoy * 10) + yoff);
			matox++;
		}
		else if (c == 'R')
		{
			matodir = 8;

			int upTile = mapcopy[matoy][matox + 1];
			int onTile = mapcopy[matoy][matox];

			if (mflag[upos])
			{
				// Box
				mapcopy[matoy][matox + 1] = (upTile == 2) ? 0 : 3;
				mapcopy[matoy][matox] = (onTile == 3) ? 5 : 2;
			}

			DrawTile(mapcopy[matoy][matox - 1], ((matox - 1) * 10) + xoff, (matoy * 10) + yoff);
			DrawTile(mapcopy[matoy][matox], (matox * 10) + xoff, (matoy * 10) + yoff);
			DrawTile(mapcopy[matoy][matox + 1], ((matox + 1) * 10) + xoff, (matoy * 10) + yoff);
			matox--;
		}
	}

	//scanKeys();
	if (!doScores) return false;

	// Display move count
	printf("\033[6;0fMoves: %i        \n", moves);
	if (upos > 0) {
	for (int i = 0; i <= upos; i++)
		printf(" ");}
	printf("\033[7;0f");
	if (upos > 0) {
	for (int i = 0; i < upos; i++)
		printf("%c", undo[i]);} else { printf(" "); }

	return true;
}

void WriteStringSRAM(char s[], int n)
{
	for (u8 i = 0; i < n; i++)
	{
		*srm++ = (uint8)*s++;
	}
}

int main(void) {
	StartTicks();
	/********** MAIN GFX ****************/
	powerON(POWER_ALL);
	videoSetMode(MODE_FB0);
	vramSetBankA(VRAM_A_LCD);
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	vramSetBankC(VRAM_C_SUB_BG);

	SUB_BG0_CR = BG_MAP_BASE(31);

	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	BG_PALETTE_SUB[255] = RGB15(31,31,31);

	//lcdMainOnBottom();

	/*for (int i = 0; i < 55; i++)
	{
		printf("%s\n", mapnames[i]);
	}*/

	//CopyMap(35);
	//DrawMap(28, 21);
	//return 0;

	char s[] = "Tomato Boxes Pro!\n";
	WriteStringSRAM(s, sizeof(s));

	int levelstart = 0;

	while (true)
	{
		levelstart = 0;
		thinger = false;
		currmap = 0;

		ClearScreen();
		printf("Start at level: (use L and R)\n");
		while(!WaitKey(KEY_A))
		{
			scanKeys();
			if ((keysHeld() & KEY_LEFT) || (keysHeld() & KEY_L))
			{
				levelstart--;
				if (levelstart < 0) levelstart = ct - 1;
			}
			else if ((keysHeld() & KEY_RIGHT) || (keysHeld() & KEY_R))
			{
				levelstart++;
				if (levelstart == ct) levelstart = 0;
			}
			printf("\033[1;0f%i ", levelstart + 1);
			Delay(100);
		}

		for (currmap = levelstart; currmap < ct; currmap++)
		{
			ClearScreen();
			printf("Tomato Boxes! By Clyde Mandelin\nPorted to the DS by JeffMan\n\nLevel %i: %s\nHigh score: %i\nPar: %i", (currmap + 1), mapnames[currmap], maphighscore[currmap], mapparscore[currmap]);

			thinger = false;
			while(!WaitKey(KEY_A))
			{ if (keysDown() & KEY_START) { thinger = true; break; } }

			if (!thinger)
			{
				CopyMap(currmap);
				DrawMap(xoff, yoff);
				GetMatoPos();

				moves = 0;
				matodir = 5;

				ResetUndo();

				while (true)
				{

					// Check to see if the level is done
					if (CheckComplete())
						break;

					// Check for input
					CheckInput();

					if (thinger) break;

					// Re-draw the mato
					DrawMato();

				}

				if (!thinger) {

				// Show results screen
				printf("\033[6;0fMoves taken to complete: %i\n", moves);
				if (upos > 0) {
				for (int i = 0; i < upos; i++)
					printf("%c", undo[i]);} else { printf(" "); }

				if (moves < maphighscore[currmap])
				{
					printf("\nA NEW RECORD!");
					char n[] = "Level ";
					WriteStringSRAM(n, sizeof(n));
					int tmpmap = currmap + 1;
					int mapct = 0;
					while (tmpmap > 10)
					{
						tmpmap -= 10;
						mapct++;
					}
					*srm++ = (mapct + 48);
					*srm++ = (tmpmap + 48);
					*srm++ = (uint8)':';
					*srm++ = (uint8)'\n';
					WriteStringSRAM(undo, upos);
					*srm++ = (uint8)'\n';
				}

				while(!WaitKey(KEY_A))
				{ }
				}

			}
			
			if (thinger)
			{
				currmap = ct;
			}
		}
	}
}

/* Touch stuff
	while (1)
	{
		touchPosition touchXY = touchReadXY();
		int x=touchXY.px;
		int y=maxy - touchXY.py;
		//printf("%d, %d", x, y);
		if (x > 0 && y > 0)
		{
			break;
		}
		//swiWaitForVBlank();
	}
	*/