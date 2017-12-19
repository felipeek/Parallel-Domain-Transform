#include "OS.h"
#include "UI.h"
#include "Util.h"
#include "Input.h"
#include <glad.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

Input_Data input_data;

#ifdef _WIN32
#include <windows.h>
#include <Commdlg.h>
#include <Shlobj.h>

// WGL Constants
#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_FLAGS_ARB                   0x2094

HWND main_window;

#elif defined(__linux__)
#error "OS not supported yet"
#endif

#ifdef _WIN32
LRESULT CALLBACK wnd_proc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MOUSEMOVE: {
		r32 mx = (r32)LOWORD(lParam);
		r32 my = (r32)HIWORD(lParam);

		RECT r;
		GetClientRect(window, &r);

		if (!r.right || !r.bottom)
		{
			MessageBox(0, "Window size is zero.", "Fatal Error", 0);
			ExitProcess(-1);
		}

		mx = ((mx / r.right) * 2.0f) - 1.0f;
		my = ((my / r.bottom) * 2.0f) - 1.0f;

		input_data.mx = mx;
		input_data.my = -my;
		return 0;
	} break;
	case WM_MOUSELEAVE: {

	} break;
	case WM_KILLFOCUS: {
		// Reset Input
	} break;
	case WM_RBUTTONDOWN: {

	} break;
	case WM_LBUTTONDOWN: {
		input_data.mclicked = 1;
		return 0;
	} break;
	case WM_RBUTTONUP: {

	} break;
	case WM_LBUTTONUP: {
		input_data.mclicked = 0;
		return 0;
	} break;
	case WM_SIZING:
	case WM_SIZE: {
		RECT r;
		GetClientRect(window, &r);
		s32 width = r.right - r.left;
		s32 height = r.bottom - r.top;
		glViewport(0, 0, width, height);
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

	// Disable VSYNC
	//BOOL (WINAPI* wglSwapIntervalEXT)(s32) = (BOOL(WINAPI*)(s32))wglGetProcAddress("wglSwapIntervalEXT");
	//wglSwapIntervalEXT(0);

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

	main_window = CreateWindow(window_class_name,
		"Domain Transform Colorization",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
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

	ShowCursor(FALSE);		// Hide Windows default cursor

	ui_init();

	//TRACKMOUSEEVENT tme;
	//tme.cbSize = sizeof(TRACKMOUSEEVENT);
	//tme.dwFlags = TME_LEAVE;
	//tme.dwHoverTime = HOVER_DEFAULT;
	//tme.hwndTrack = main_window;

	MSG msg;
	s32 running = 1, fps = 0;

	//while (running)
	//{
		//TrackMouseEvent(&tme);
		
		ui_update();
		ui_render();

		while (GetMessage(&msg, 0, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			
			ui_update();
			ui_render();

			HDC hdc = GetDC(main_window);
			SwapBuffers(hdc);
			ReleaseDC(main_window, hdc);
		}

		/*while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) > 0)
		{
			if (msg.message == WM_QUIT)
			{
				running = 0;
				continue;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		ui_update();
		ui_render();

		HDC hdc = GetDC(main_window);
		SwapBuffers(hdc);
		ReleaseDC(main_window, hdc);

		++fps;

		if (0)//end_clock() >= 1.0f)
		{
			print("FPS: %d\n", fps);
			//start_clock();
			fps = 0;
		}*/
	//}
	
	return msg.wParam;
}

extern bool os_save_file_dialog(s8* buffer, s32 buffer_size) {
	OPENFILENAME fn = { 0 };
	fn.lStructSize = sizeof(OPENFILENAME);
	fn.hwndOwner = 0;
	fn.Flags = 0;
	fn.lpstrFile = buffer;
	fn.nMaxFile = buffer_size;
	buffer[0] = 0;

	ShowCursor(TRUE);
	bool ret = GetSaveFileName(&fn);
	ShowCursor(FALSE);

	return ret;
}

extern bool os_open_file_dialog(s8* buffer, s32 buffer_size) {
	OPENFILENAME fn = { 0 };
	fn.lStructSize = sizeof(OPENFILENAME);
	fn.hwndOwner = 0;
	fn.Flags = 0;
	fn.lpstrFile = buffer;
	fn.nMaxFile = buffer_size;
	fn.lpstrFilter = 0;
	buffer[0] = 0;

	ShowCursor(TRUE);
	bool ret = GetOpenFileName(&fn);
	ShowCursor(FALSE);

	return ret;
}
#elif defined(__linux__)
#error "OS not supported yet"
#endif