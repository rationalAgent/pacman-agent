#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <ncurses.h>		/* we still use the getch */
#include "pacman.h"
#include "gameloop.h"
#include "player.h"
#include "ghosts.h"
#include "board.h"
#include "render.h"
#include "pills.h"
#include "analyze.h"


/*
  I got a little bored I'm afraid writing bits of this, which is why
  the animated death and flashing screen happen syncronously. To be done 
  correctly, they should be pseudo-event driven like the rest of the program.
*/
static void DrawDynamic(void *pCtx, GAME_STATE *ptr)
{
	Pac_DrawBoard(pCtx, ptr);
	Pac_DrawPills(pCtx, ptr);
	Pac_DrawPlayer(pCtx, ptr);
	Pac_DrawGhosts(pCtx, ptr);
	Pac_RenderRHS(pCtx, ptr);
}

tGameEnd Pac_GameLoop(void *pCtx, GAME_STATE *ptr)
{
	int c, keypress;
	float telaps;

	do {
		keypress = Pac_AnalyzeGameState(*ptr);
		
		int CenteredStr = CenteredX("Key Hint: A");
		mvprintw(24, CenteredStr, "Key Hint: %c", keypress);

		if (ptr->Player.Agent == ePAC_Human) {
			while (c = getch()) {
				if (c == PACKEY_UP ||
			    	c == PACKEY_DOWN ||
					c == PACKEY_RIGHT || 
					c == PACKEY_LEFT ||
					c == 'q' || 
					c == 'a') break;
			}
			if (c == 'q' || c == 'Q')
				return ePAC_UserQuit;
			keypress = c;
			if (c == 'a') {
				ptr->Player.Agent = ePAC_Computer;
			}
		}

		telaps = 0.2;

		/* scroll marquee */
		if (ptr->pMarquee) {
			c = *ptr->pMarquee;
			memmove(ptr->pMarquee, ptr->pMarquee+1, ptr->iMarqueeSize-1);
			ptr->pMarquee[ptr->iMarqueeSize-1] = c;
		}

		/* Update all game components */
		Pac_UpdatePlayer(ptr, telaps, keypress);
		Pac_CheckPlayerVsGhosts(ptr);
		Pac_UpdateGhosts(ptr, telaps);
		Pac_CheckPlayerVsGhosts(ptr);
		Pac_UpdatePills(ptr, telaps);

		/* Draw everything */
		DrawDynamic(pCtx, ptr);

		/* Make it visible */
		Pac_Blit(pCtx);

		/* Has an end game condition been reached? */
		if (ptr->iDotsLeft == 0)	return ePAC_SheetComplete;
		else if (ptr->Player.bDead)	return ePAC_LifeLost;

		// if (ptr->iDotsLeft < 50) ptr->Player.Agent = ePAC_Human;
		
		usleep(80000);
	} while(1);
}

void Pac_MainGame(void *pCtx, GAME_STATE *ptr)
{
tGameEnd iGS;

	Pac_InitialiseGame(ptr);
	Pac_RenderGameInfo(pCtx);
	Pac_RenderRHS(pCtx, ptr);
	do {
		/* Start of sheet/new life */
		DrawDynamic(pCtx, ptr);
		Pac_Blit(pCtx);
		sleep(2);

		iGS = Pac_GameLoop(pCtx, ptr);
		switch(iGS) {
			case	ePAC_SheetComplete: 
					Pac_FlashBoard(pCtx, ptr); 
					ptr->iLevel++;
					Pac_ReinitialiseGame(ptr);
					break;
			case	ePAC_LifeLost:	 
					--ptr->Player.iLives;
					Pac_RenderRHS(pCtx, ptr);
					Pac_AnimateDeadPlayer(pCtx, ptr); 
					if (ptr->Player.iLives < 0)
						return;
					sleep(1);
					Pac_InitialiseGhosts(ptr);
					Pac_ReinitialisePlayer(ptr);
					break;
			default:
					break;
			}
	
	} while(iGS != ePAC_UserQuit);
}
