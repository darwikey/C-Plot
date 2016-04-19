#include "Application.h"

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char*, int nShowCmd)
{
	Application app;
	app.init();
	app.main();
}
