#include "OS.h"
#include "UI.h"
#include <glad.h>

#ifdef _WIN32
#include <windows.h>

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_FLAGS_ARB                   0x2094

#elif defined(__linux__)

#endif

#ifdef _WIN32
LRESULT CALLBACK wnd_proc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MOUSELEAVE: {
		//OutputDebugString("it works");
	} break;
	case WM_KILLFOCUS: {
		// resetar input
	} break;
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN: {
		SetCapture(window);
	} break;
	case WM_RBUTTONUP:
	case WM_LBUTTONUP: {
		ReleaseCapture();
	} break;
	case WM_SIZING:
	case WM_SIZE: {
		//RECT r;
		//GetClientRect(window, &r);
		//s32 width = r.right - r.left;
		//s32 height = r.bottom - r.top;
		//glViewport(0, 0, width, height);
		//glClear(GL_COLOR_BUFFER_BIT);
		//SwapBuffers(GetDC(window));
	} break;
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	} break;
	}

	return DefWindowProc(window, msg, wParam, lParam);
}

static s32 create_opengl_context(HWND window)
{
	PIXELFORMATDESCRIPTOR pfd = { 0 };

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.cDepthBits = 32;
	pfd.cColorBits = 24;
	pfd.cAlphaBits = 8;
	pfd.iPixelType = PFD_TYPE_RGBA;

	HDC hdc = GetDC(window);

	s32 pixel_format = ChoosePixelFormat(hdc, &pfd);

	if (!SetPixelFormat(hdc, pixel_format, &pfd))
	{
		MessageBox(0, "Pixel format error", "Error", 0);
		return -1;
	}

	HGLRC tmp_context = wglCreateContext(hdc);

	if (!wglMakeCurrent(hdc, tmp_context))
	{
		MessageBox(0, "Make context error", "Error", 0);
		return -1;
	}

	s32 attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_FLAGS_ARB, 0, 0
	};

	HGLRC(WINAPI* wglCreateContextAttribsARB)(HDC, HGLRC, s32*) = (HGLRC(WINAPI*)(HDC, HGLRC, s32*))wglGetProcAddress("wglCreateContextAttribsARB");

	wglMakeCurrent(0, 0);
	wglDeleteContext(tmp_context);

	HGLRC context = wglCreateContextAttribsARB(hdc, 0, attributes);

	wglMakeCurrent(hdc, context);

	return 0;
}

extern s32 os_init_gui()
{
	static s8 window_class_name[] = "Main Window Class";

	WNDCLASS window_class;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hbrBackground = 0;
	window_class.hCursor = LoadCursor(0, IDC_ARROW);
	window_class.hIcon = 0;
	window_class.hInstance = GetModuleHandle(0);
	window_class.lpfnWndProc = wnd_proc;
	window_class.lpszClassName = window_class_name;
	window_class.lpszMenuName = 0;
	window_class.style = CS_VREDRAW | CS_HREDRAW;

	RegisterClass(&window_class);

	HWND main_window = CreateWindow(window_class_name,
		"Hello World Application",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0, 0, 0, 0);

	if (create_opengl_context(main_window))
	{
		MessageBox(0, "Could not create OpenGL context.", "Fatal Error", 0);
		return -1;
	}

	if (!gladLoadGL()) {
		MessageBox(0, "Error loading glad.", "Fatal Error", 0);
		return -1;
	}

	ShowWindow(main_window, SW_NORMAL);
	UpdateWindow(main_window);

	ui_init();

	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.dwFlags = TME_LEAVE;
	tme.dwHoverTime = HOVER_DEFAULT;
	tme.hwndTrack = main_window;

	MSG msg;
	HDC hdc = GetDC(main_window);

	s32 running = 1;
	while (running)
	{
		TrackMouseEvent(&tme);

		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) > 0)
		{
			if (msg.message == WM_QUIT)
			{
				running = 0;
				continue;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
		ui_render();
		SwapBuffers(hdc);
	}
	
	return msg.wParam;
}
#elif defined(__linux__)
extern s32 os_init_gui()
{
	return -1;
}
#endif