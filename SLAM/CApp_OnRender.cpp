#include "CApp.h"

void CApp::OnRender() {	
	CSurface::Transparent(Surf_Map,0,0,0);
	CSurface::Transparent(Surf_Overlay,0,0,0);
	CSurface::Transparent(Surf_Odometry,0,0,0);

	CSurface::OnDraw(Surf_Display,Surf_Odometry,0,0);
	CSurface::OnDraw(Surf_Display,Surf_Overlay,Surf_Display->w-Surf_Overlay->w,0);
	CSurface::OnDraw(Surf_Display,Surf_Map,0,0);
	SDL_Flip(Surf_Display);

	SDL_SetColorKey(Surf_Odometry,0,0);
	SDL_SetColorKey(Surf_Overlay,0,0);
	SDL_SetColorKey(Surf_Map,0,0);

	SDL_FillRect(Surf_Odometry,NULL,0x000000);
	SDL_FillRect(Surf_Overlay,NULL,0x000000);
	SDL_FillRect(Surf_Map,NULL,0x000000);
	SDL_FillRect(Surf_Display,NULL,0x000000);
}