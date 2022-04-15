#include <SDL.h>
#include <SDL_syswm.h>
#include <Windows.h>
#include <iostream>
#include <algorithm>

enum class RectResult {
	NONE,
	HOVER,
	HELD,
	CLICKED
};

SDL_HitTestResult dragCallback(SDL_Window* win, const SDL_Point* area, void* data);
RectResult checkRectClick(SDL_Rect rect, Uint32 bS, Uint32 pBS, int mouseX, int mouseY);

const int FPS = 60;
int BPM = 140;
int prevBPM = BPM;

int main(int argc, char* args[]) {
	//Window and other related variables
	int screen_width = 320, screen_height = 320 + 32;
	float msElapsed;
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_Event e;

	//GUI Variables
	SDL_Rect captionRect[3];
	SDL_Rect playButtonRect;
	SDL_Rect sliderRect[2];
	SDL_Point outlinePoints[5];
	SDL_Vertex vert_playButton[3];
	SDL_Rect minmaxRect;
	bool quit = false;
	bool minimize = false;
	bool play = false;
	bool sliderHeld = false;

	//Input Variables
	int mouseX = 0, mouseY = 0, prevMouseX = 0, prevMouseY = 0;
	Uint32 prevButtonState = 0;
	Uint32 buttonState = 0;
	Uint64 start, end;
	
	//Color Variables
	const SDL_Color col_outline =			{ 200, 200, 200 };
	const SDL_Color col_background =		{ 50, 50, 50 };
	const SDL_Color col_captionBar =		{ 69, 69, 69 };
	const SDL_Color col_captionButton =		{ 40, 40, 40 };
	const SDL_Color col_captionMinHover =	{ 75, 75, 75 };
	const SDL_Color col_captionMinHeld =	{ 100, 100, 100 };
	const SDL_Color col_captionCloseHover =	{ 255, 100, 100 };
	const SDL_Color col_captionCloseHeld =	{ 255, 50, 50 };
	const SDL_Color col_Button =			{ 100, 100, 100 };
	const SDL_Color col_ButtonHover =		{ 125, 125, 125 };
	const SDL_Color col_ButtonHeld =		{ 150, 150, 150 };
	const SDL_Color col_sliderBG =			{ 30, 30, 30 };
	SDL_Color col_captionMin =				{ 0, 0, 0 };
	SDL_Color col_captionClose =			{ 0, 0, 0 };
	SDL_Color col_play =					{ 100, 100, 100 };
	SDL_Color col_sliderTab =				{ 100, 100, 100 };
	//The two colors at the bottom are going to be used as buffers and be set to the other colors

	//Audio Variables
	double BPS = (double)BPM / 60.0;
	double msBetweenBeats;
	double msTickTimer = 0;
	int numTest = 0;
	SDL_AudioDeviceID deviceId;
	SDL_AudioSpec wavSpec;
	Uint32 wavLength;
	Uint32 wavLength2;
	Uint8* wavBuffer{};
	Uint8* wavBuffer2{};

	SDL_SetMainReady();	//The SDL wiki says that this function circumvents failure of SDL_Init() when not using SDL_main() as an entry point.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
		std::cout << "Error: SDL could not init.";
	}
	else {
		
		//The window's going to be a square, but I've made it borderless in order to implement
		//my own caption bar (caption bar refers to the titlebar)
		window = SDL_CreateWindow(
			"Small Metronome",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			screen_width, screen_height, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);

		if (window == NULL) { std::cout << "Error, window could not be initialized."; }
		else {
			//Audio initialization
			SDL_LoadWAV("data/s_met_hi.wav", &wavSpec, &wavBuffer, &wavLength);
			SDL_LoadWAV("data/s_met_lo.wav", &wavSpec, &wavBuffer2, &wavLength2);

			deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);
			//std::cout << "Audio device is: " << SDL_GetAudioDeviceName()
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

			//Defining the rectangles that the caption bar/caption buttons take up
			captionRect[0] = { 0, 0, screen_width, 32 };
			captionRect[1] = { screen_width - 92, 0, 48, 32 };
			captionRect[2] = { screen_width - 46, 0, 48, 32 };

			playButtonRect = { (screen_width/2) - 40, screen_height - 120, 80, 80 };

			sliderRect[0] = { 25, 25 + 32, screen_width - 50, 75 };
			sliderRect[1] = { (screen_width / 2) - 12, 20 + 32, 25, 85 };

			//Defining verticies for the play button
			vert_playButton[0].position = { (float)playButtonRect.x + playButtonRect.h, playButtonRect.y + (playButtonRect.h/2.0f) };
			vert_playButton[1].position = { (float)playButtonRect.x, (float)playButtonRect.y};
			vert_playButton[2].position = { (float)playButtonRect.x, (float)playButtonRect.y + 80.0f};
			minmaxRect = { 0, 0, screen_width, screen_height };
			
			//This function is what makes the top 32 pixels of the app
			//drag the window around when left mouse button is held on it
			SDL_SetWindowHitTest(window, dragCallback, &screen_width);

			//Getting window handle
			SDL_SysWMinfo wmInfo;
			SDL_VERSION(&wmInfo.version);
			SDL_GetWindowWMInfo(window, &wmInfo);
			HWND hWnd = wmInfo.info.win.window;

			//Changing window type to layered, so we can implement a custom minimize animation
			SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);

			while (!quit) {	//Main loop
				start = SDL_GetPerformanceCounter();
				prevMouseX = mouseX; prevMouseY = mouseY; prevButtonState = buttonState;

				//I'm keeping this PollEvents call here
				//So that users can still press Alt+F4 to exit
				while (SDL_PollEvent(&e) != 0) {
					if (e.type == SDL_QUIT) {
						quit = true;
					}
				}

				SDL_PumpEvents();
				buttonState = SDL_GetMouseState(&mouseX, &mouseY);

				//So this might be a bit clunky, but it's fine for a smaller application
				//I've written a function that returns a state based on whether a rect was
				//hovered on, held, or clicked, and am handling it with a switch statement like this

				switch (checkRectClick(playButtonRect, buttonState, prevButtonState, mouseX, mouseY)) {
				case RectResult::NONE:
					col_play = col_Button;
					break;
				case RectResult::HOVER:
					col_play = col_ButtonHover;
					break;
				case RectResult::HELD:
					col_play = col_ButtonHeld;
					break;
				case RectResult::CLICKED:
					col_play = col_Button;
					play = !play;
					if (play) {
						system("cls");
						std::cout << std::flush << "Playing at " << BPM << " BPM" << std::endl;
					}
					else { std::cout << "Stopped" << std::endl; }

					break;
				}
				
				vert_playButton[0].color = col_play;
				vert_playButton[1].color = col_play;
				vert_playButton[2].color = col_play;

				switch (checkRectClick(sliderRect[1], buttonState, prevButtonState, mouseX, mouseY)) {
				case RectResult::NONE:
					col_sliderTab = col_Button;
					if (((buttonState & SDL_BUTTON(1)) == SDL_BUTTON(1)) && sliderHeld) {
						sliderHeld = true;
					}
					else {
						sliderHeld = false;
					}
					break;
				case RectResult::HOVER:
					col_sliderTab = col_ButtonHover;
					if (((buttonState & SDL_BUTTON(1)) == SDL_BUTTON(1)) && sliderHeld) {
						sliderHeld = true;
					}
					else {
						sliderHeld = false;
					}
					break;
				case RectResult::HELD:
					col_sliderTab = col_ButtonHeld;
					sliderHeld = true;
					break;
				case RectResult::CLICKED:
					col_sliderTab = col_Button;
					break;
				}
				
				if (sliderHeld) {
					sliderRect[1].x = std::clamp(mouseX, 25, screen_width - 25) - 12;
				}
				else {
					BPM += (sliderRect[1].x - ((screen_width / 2) - 12)) / 10;
					if (prevBPM != BPM) {
						std::cout << "BPM changed from " << prevBPM << " to " << BPM << std::endl;
					} sliderRect[1].x = (screen_width / 2) - 12;
				}

				prevBPM = BPM;

				switch (checkRectClick(captionRect[1], buttonState, prevButtonState, mouseX, mouseY)) {
				case RectResult::NONE:
					col_captionMin = col_captionButton;
					break;
				case RectResult::HOVER:
					col_captionMin = col_captionMinHover;
					break;
				case RectResult::HELD:
					col_captionMin = col_captionMinHeld;
					break;
				case RectResult::CLICKED:
					col_captionMin = col_captionButton;
					minimize = true;
					break;
				}

				switch (checkRectClick(captionRect[2], buttonState, prevButtonState, mouseX, mouseY)) {
				case RectResult::NONE:
					col_captionClose = col_captionButton;
					break;
				case RectResult::HOVER:
					col_captionClose = col_captionCloseHover;
					break;
				case RectResult::HELD:
					col_captionClose = col_captionCloseHeld;
					break;
				case RectResult::CLICKED:
					col_captionClose = col_captionCloseHeld;
					quit = true;
					break;
				}


				SDL_GetWindowSize(window, &screen_width, &screen_height);
				captionRect[0].w = screen_width;

				if (!minimize) { //So in here is where I draw the GUI

					SetLayeredWindowAttributes(hWnd, NULL, NULL, LWA_COLORKEY);
					if (((SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) != SDL_WINDOW_MAXIMIZED)) {
						minmaxRect = { 0, 0, screen_width, screen_height };
					}

					//Caption bar stuff
					SDL_SetRenderDrawColor(renderer, col_background.r, col_background.g, col_background.b, 255);
					SDL_RenderClear(renderer);

					SDL_SetRenderDrawColor(renderer, col_captionBar.r, col_captionBar.g, col_captionBar.b, 255);
					SDL_RenderFillRect(renderer, &captionRect[0]);

					SDL_SetRenderDrawColor(renderer, col_captionMin.r, col_captionMin.g, col_captionMin.b, 255);
					SDL_RenderFillRect(renderer, &captionRect[1]);
					
					SDL_SetRenderDrawColor(renderer, col_captionClose.r, col_captionClose.g, col_captionClose.b, 255);
					SDL_RenderFillRect(renderer, &captionRect[2]);
					
					SDL_SetRenderDrawColor(renderer, col_sliderBG.r, col_sliderBG.g, col_sliderBG.b, 255);
					SDL_RenderFillRect(renderer, &sliderRect[0]);

					SDL_SetRenderDrawColor(renderer, col_sliderTab.r, col_sliderTab.g, col_sliderTab.b, 255);
					SDL_RenderFillRect(renderer, &sliderRect[1]);

					if (play) {
						SDL_SetRenderDrawColor(renderer, col_play.r, col_play.g, col_play.b, 255);
						SDL_RenderFillRect(renderer, &playButtonRect);

						//Audio/Metronome code
						BPS = (double)BPM / 60.0;
						msBetweenBeats = (double)(1000.0 / ((double)BPS));
						msTickTimer += ((float)(1000.0f / ((float)FPS)));
						if (msTickTimer > msBetweenBeats) {
							msTickTimer = 0;
							std::cout << "Tick" << std::endl;
							SDL_ClearQueuedAudio(deviceId);

							if (numTest > 0 && numTest <= 3) {
								SDL_QueueAudio(deviceId, wavBuffer2, wavLength2);
								numTest++;
							} else if (numTest >= 4) { numTest = 0; }
							if (numTest == 0) {
								SDL_QueueAudio(deviceId, wavBuffer, wavLength);
								numTest++;
							}
							SDL_PauseAudioDevice(deviceId, 0);
						}
					}
					else {
						SDL_PauseAudioDevice(deviceId, 1);
						SDL_RenderGeometry(renderer, NULL, vert_playButton, 3, NULL, 0);
						msTickTimer = 0;
						numTest = 0;
					}

					//Outlines
					outlinePoints[0] = { minmaxRect.x, minmaxRect.y };
					outlinePoints[1] = { minmaxRect.x + minmaxRect.w - 1, minmaxRect.y };
					outlinePoints[2] = { minmaxRect.x + minmaxRect.w - 1, minmaxRect.y + minmaxRect.h - 1 };
					outlinePoints[3] = { minmaxRect.x, minmaxRect.y + minmaxRect.h - 1 };
					outlinePoints[4] = { minmaxRect.x, minmaxRect.y };

					SDL_SetRenderDrawColor(renderer, col_outline.r, col_outline.g, col_outline.b, 255);
					SDL_RenderDrawLines(renderer, outlinePoints, 5);

					//Caption Buttons
					SDL_RenderDrawLine(renderer, screen_width - 75, 17, screen_width - 65, 17); //Minimize

					SDL_RenderDrawLine(renderer, screen_width - 29, 12, screen_width - 20, 21); //Backslash
					SDL_RenderDrawLine(renderer, screen_width - 29, 21, screen_width - 20, 12); //Slash

				}
				else {
					//This is the minimizing animation, it will only work on windows
					//I only have this here because the default animation doesn't seem to work for some reason.

					play = false;

					SetLayeredWindowAttributes(hWnd, RGB(col_background.r - 10, col_background.g - 10, col_background.b - 10), NULL, LWA_COLORKEY);
					SDL_SetRenderDrawColor(renderer, col_background.r - 10, col_background.g - 10, col_background.b - 10, 255);
					SDL_RenderClear(renderer);

					if (minmaxRect.w != minmaxRect.h) minmaxRect.h = minmaxRect.w;
					minmaxRect.x += 10; minmaxRect.y += 10;
					minmaxRect.w -= 20; minmaxRect.h -= 20;

					SDL_SetRenderDrawColor(renderer, col_background.r, col_background.g, col_background.b, 255);
					SDL_RenderFillRect(renderer, &minmaxRect);

					//So the reason why I'm not implementing a custom outline on the minimizing animation
					//is because it appears that the chroma key of the layered window is kind of creating
					//an outline effect on it's own, I'm guessing this is because some sort of filtering is
					//currently on, which is blending pixels (or something like that)

					outlinePoints[0] = { minmaxRect.x, minmaxRect.y };
					outlinePoints[1] = { minmaxRect.x + minmaxRect.w - 1, minmaxRect.y };
					outlinePoints[2] = { minmaxRect.x + minmaxRect.w - 1, minmaxRect.y + minmaxRect.h - 1 };
					outlinePoints[3] = { minmaxRect.x, minmaxRect.y + minmaxRect.h - 1 };
					outlinePoints[4] = { minmaxRect.x, minmaxRect.y };
					SDL_SetRenderDrawColor(renderer, col_outline.r, col_outline.g, col_outline.b, 255);
					SDL_RenderDrawLines(renderer, outlinePoints, 5);

					if ((minmaxRect.x > (screen_width / 2)) && (minmaxRect.y > ((screen_height-32) / 2)) && (minmaxRect.w < (screen_width / 2)) && (minmaxRect.h < ((screen_height-32) / 2))) {
						minimize = false;
						SDL_MinimizeWindow(window);
					}
				}
				
				SDL_RenderPresent(renderer);
				end = SDL_GetPerformanceCounter();

				msElapsed = (end - start) / (float)SDL_GetPerformanceFrequency();

				//std::cout << msElapsed << " ms elapsed" << std::endl;
				SDL_Delay(floor(((float)(1000.0f / ((float)FPS))) - msElapsed));
			}
		}
	}

	SDL_FreeWAV(wavBuffer);
	SDL_FreeWAV(wavBuffer2);
	SDL_CloseAudioDevice(deviceId);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

SDL_HitTestResult dragCallback(SDL_Window* win, const SDL_Point* area, void* data) {
	if (area->y <= 32 && area->x < (320 - 92)) {
		return SDL_HITTEST_DRAGGABLE;
	}
	return SDL_HITTEST_NORMAL;
}

RectResult checkRectClick(SDL_Rect rect, Uint32 bS, Uint32 pBS, int mouseX, int mouseY) {
	RectResult r = RectResult::NONE;

	if (mouseX < (rect.w + rect.x) && mouseX > rect.x && mouseY < (rect.h + rect.y) && mouseY > rect.y) {
		r = RectResult::HOVER;
		if ((bS & SDL_BUTTON(1)) == SDL_BUTTON(1)) {
			r = RectResult::HELD;
		}
		else if ((pBS & SDL_BUTTON(1)) == SDL_BUTTON(1) && (bS & SDL_BUTTON(1)) != SDL_BUTTON(1)) {
			r = RectResult::CLICKED;
		}
	}

	return r;
}
