#include "Win32Window.h"
#include <windows.h>

namespace Mai {
  namespace {
	Key VirtualKeyCodeToKey(WPARAM key, LPARAM flags)
	{
	  switch (key) {
		// Check the scancode to distinguish between left and right shift
	  case VK_SHIFT:
	  {
		static unsigned int lShift = MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC);
		unsigned int scancode = static_cast<unsigned int>((flags & (0xFF << 16)) >> 16);
		return scancode == lShift ? KEY_LSHIFT : KEY_RSHIFT;
	  }

	  // Check the "extended" flag to distinguish between left and right alt
	  case VK_MENU:
		return (HIWORD(flags) & KF_EXTENDED) ? KEY_RALT : KEY_LALT;

		// Check the "extended" flag to distinguish between left and right control
	  case VK_CONTROL:
		return (HIWORD(flags) & KF_EXTENDED) ? KEY_RCONTROL : KEY_LCONTROL;

		// Other keys are reported properly
	  case VK_LWIN:
		return KEY_LSYSTEM;
	  case VK_RWIN:
		return KEY_RSYSTEM;
	  case VK_APPS:
		return KEY_MENU;
	  case VK_OEM_1:
		return KEY_SEMICOLON;
	  case VK_OEM_2:
		return KEY_SLASH;
	  case VK_OEM_PLUS:
		return KEY_EQUAL;
	  case VK_OEM_MINUS:
		return KEY_DASH;
	  case VK_OEM_4:
		return KEY_LBRACKET;
	  case VK_OEM_6:
		return KEY_RBRACKET;
	  case VK_OEM_COMMA:
		return KEY_COMMA;
	  case VK_OEM_PERIOD:
		return KEY_PERIOD;
	  case VK_OEM_7:
		return KEY_QUOTE;
	  case VK_OEM_5:
		return KEY_BACKSLASH;
	  case VK_OEM_3:
		return KEY_TILDE;
	  case VK_ESCAPE:
		return KEY_ESCAPE;
	  case VK_SPACE:
		return KEY_SPACE;
	  case VK_RETURN:
		return KEY_RETURN;
	  case VK_BACK:
		return KEY_BACK;
	  case VK_TAB:
		return KEY_TAB;
	  case VK_PRIOR:
		return KEY_PAGEUP;
	  case VK_NEXT:
		return KEY_PAGEDOWN;
	  case VK_END:
		return KEY_END;
	  case VK_HOME:
		return KEY_HOME;
	  case VK_INSERT:
		return KEY_INSERT;
	  case VK_DELETE:
		return KEY_DELETE;
	  case VK_ADD:
		return KEY_ADD;
	  case VK_SUBTRACT:
		return KEY_SUBTRACT;
	  case VK_MULTIPLY:
		return KEY_MULTIPLY;
	  case VK_DIVIDE:
		return KEY_DIVIDE;
	  case VK_PAUSE:
		return KEY_PAUSE;
	  case VK_F1:
		return KEY_F1;
	  case VK_F2:
		return KEY_F2;
	  case VK_F3:
		return KEY_F3;
	  case VK_F4:
		return KEY_F4;
	  case VK_F5:
		return KEY_F5;
	  case VK_F6:
		return KEY_F6;
	  case VK_F7:
		return KEY_F7;
	  case VK_F8:
		return KEY_F8;
	  case VK_F9:
		return KEY_F9;
	  case VK_F10:
		return KEY_F10;
	  case VK_F11:
		return KEY_F11;
	  case VK_F12:
		return KEY_F12;
	  case VK_F13:
		return KEY_F13;
	  case VK_F14:
		return KEY_F14;
	  case VK_F15:
		return KEY_F15;
	  case VK_LEFT:
		return KEY_LEFT;
	  case VK_RIGHT:
		return KEY_RIGHT;
	  case VK_UP:
		return KEY_UP;
	  case VK_DOWN:
		return KEY_DOWN;
	  case VK_NUMPAD0:
		return KEY_NUMPAD0;
	  case VK_NUMPAD1:
		return KEY_NUMPAD1;
	  case VK_NUMPAD2:
		return KEY_NUMPAD2;
	  case VK_NUMPAD3:
		return KEY_NUMPAD3;
	  case VK_NUMPAD4:
		return KEY_NUMPAD4;
	  case VK_NUMPAD5:
		return KEY_NUMPAD5;
	  case VK_NUMPAD6:
		return KEY_NUMPAD6;
	  case VK_NUMPAD7:
		return KEY_NUMPAD7;
	  case VK_NUMPAD8:
		return KEY_NUMPAD8;
	  case VK_NUMPAD9:
		return KEY_NUMPAD9;
	  case 'A':
		return KEY_A;
	  case 'Z':
		return KEY_Z;
	  case 'E':
		return KEY_E;
	  case 'R':
		return KEY_R;
	  case 'T':
		return KEY_T;
	  case 'Y':
		return KEY_Y;
	  case 'U':
		return KEY_U;
	  case 'I':
		return KEY_I;
	  case 'O':
		return KEY_O;
	  case 'P':
		return KEY_P;
	  case 'Q':
		return KEY_Q;
	  case 'S':
		return KEY_S;
	  case 'D':
		return KEY_D;
	  case 'F':
		return KEY_F;
	  case 'G':
		return KEY_G;
	  case 'H':
		return KEY_H;
	  case 'J':
		return KEY_J;
	  case 'K':
		return KEY_K;
	  case 'L':
		return KEY_L;
	  case 'M':
		return KEY_M;
	  case 'W':
		return KEY_W;
	  case 'X':
		return KEY_X;
	  case 'C':
		return KEY_C;
	  case 'V':
		return KEY_V;
	  case 'B':
		return KEY_B;
	  case 'N':
		return KEY_N;
	  case '0':
		return KEY_NUM0;
	  case '1':
		return KEY_NUM1;
	  case '2':
		return KEY_NUM2;
	  case '3':
		return KEY_NUM3;
	  case '4':
		return KEY_NUM4;
	  case '5':
		return KEY_NUM5;
	  case '6':
		return KEY_NUM6;
	  case '7':
		return KEY_NUM7;
	  case '8':
		return KEY_NUM8;
	  case '9':
		return KEY_NUM9;
	  }
	  return Key(0);
	}

	bool HasClick(int x1, int y1, int64_t time, const Win32Window::MouseInfo& prevInfo) {
	  if (time - prevInfo.time > 500/* ms */) {
		return false;
	  }
	  const int diffX = x1 - prevInfo.x;
	  const int diffY = y1 - prevInfo.y;
	  const int length = static_cast<int>(std::sqrt(diffX * diffX + diffY * diffY));
	  return length < 8/* pixel */;
	}

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	  if (message == WM_NCCREATE) {
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		return DefWindowProcA(hWnd, message, wParam, lParam);
	  }

	  Win32Window *window = reinterpret_cast<Win32Window *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	  if (window) {
		const int64_t time = GetMessageTime();
		switch (message) {
		case WM_DESTROY:
		case WM_CLOSE:
		{
		  Event event;
		  event.Type = Event::EVENT_CLOSED;
		  event.Time = time;
		  window->PushEvent(event);
		  break;
		}

		case WM_MOVE:
		{
		  RECT winRect;
		  GetClientRect(hWnd, &winRect);

		  POINT topLeft;
		  topLeft.x = winRect.left;
		  topLeft.y = winRect.top;
		  ClientToScreen(hWnd, &topLeft);

		  Event event;
		  event.Type = Event::EVENT_MOVED;
		  event.Time = time;
		  event.Move.X = topLeft.x;
		  event.Move.Y = topLeft.y;
		  window->PushEvent(event);

		  break;
		}

		case WM_SIZE:
		{
		  RECT winRect;
		  GetClientRect(hWnd, &winRect);

		  POINT topLeft;
		  topLeft.x = winRect.left;
		  topLeft.y = winRect.top;
		  ClientToScreen(hWnd, &topLeft);

		  POINT botRight;
		  botRight.x = winRect.right;
		  botRight.y = winRect.bottom;
		  ClientToScreen(hWnd, &botRight);

		  Event event;
		  event.Type = Event::EVENT_RESIZED;
		  event.Time = time;
		  event.Size.Width = botRight.x - topLeft.x;
		  event.Size.Height = botRight.y - topLeft.y;
		  window->PushEvent(event);

		  break;
		}

		case WM_SETFOCUS:
		{
		  Event event;
		  event.Type = Event::EVENT_GAINED_FOCUS;
		  event.Time = time;
		  window->PushEvent(event);
		  break;
		}

		case WM_KILLFOCUS:
		{
		  Event event;
		  event.Type = Event::EVENT_LOST_FOCUS;
		  event.Time = time;
		  window->PushEvent(event);
		  break;
		}

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
		  const bool down = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);

		  Event event;
		  event.Type = down ? Event::EVENT_KEY_PRESSED : Event::EVENT_KEY_RELEASED;
		  event.Time = time;
		  event.Key.Alt = HIWORD(GetAsyncKeyState(VK_MENU)) != 0;
		  event.Key.Control = HIWORD(GetAsyncKeyState(VK_CONTROL)) != 0;
		  event.Key.Shift = HIWORD(GetAsyncKeyState(VK_SHIFT)) != 0;
		  event.Key.System = HIWORD(GetAsyncKeyState(VK_LWIN)) || HIWORD(GetAsyncKeyState(VK_RWIN));
		  event.Key.Code = VirtualKeyCodeToKey(wParam, lParam);
		  window->PushEvent(event);
#ifndef NDEBUG
		  if (event.Key.Code == KEY_P) {
			Event e0;
			e0.Type = Event::EVENT_TERM_WINDOW;
			window->PushEvent(e0);
			Event e1;
			e1.Type = Event::EVENT_INIT_WINDOW;
			window->PushEvent(e1);
		  }
#endif // NDEBUG
		  break;
		}

		case WM_MOUSEWHEEL:
		{
		  Event event;
		  event.Type = Event::EVENT_MOUSE_WHEEL_MOVED;
		  event.Time = time;
		  event.MouseWheel.Delta = static_cast<short>(HIWORD(wParam)) / 120;
		  window->PushEvent(event);
		  break;
		}

		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		{
		  Event event;
		  event.Type = Event::EVENT_MOUSE_BUTTON_PRESSED;
		  event.Time = time;
		  event.MouseButton.Button = MOUSEBUTTON_LEFT;
		  event.MouseButton.X = static_cast<int16_t>(LOWORD(lParam));
		  event.MouseButton.Y = static_cast<int16_t>(HIWORD(lParam));
		  window->SetMouseInfo(event.MouseButton.X, event.MouseButton.Y, time);
		  window->PushEvent(event);
		  break;
		}

		case WM_LBUTTONUP:
		{
		  Event event;
		  event.Type = Event::EVENT_MOUSE_BUTTON_RELEASED;
		  event.Time = time;
		  event.MouseButton.Button = MOUSEBUTTON_LEFT;
		  event.MouseButton.X = static_cast<short>(LOWORD(lParam));
		  event.MouseButton.Y = static_cast<short>(HIWORD(lParam));
		  if (HasClick(event.MouseButton.X, event.MouseButton.Y, time, window->GetPrevMouseInfo())) {
			Event eventClick = event;
			eventClick.Type = Event::EVENT_MOUSE_BUTTON_CLICKED;
			window->PushEvent(eventClick);
		  }
		  window->PushEvent(event);
		  break;
		}

		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		{
		  Event event;
		  event.Type = Event::EVENT_MOUSE_BUTTON_PRESSED;
		  event.Time = time;
		  event.MouseButton.Button = MOUSEBUTTON_RIGHT;
		  event.MouseButton.X = static_cast<int16_t>(LOWORD(lParam));
		  event.MouseButton.Y = static_cast<int16_t>(HIWORD(lParam));
		  window->SetMouseInfo(event.MouseButton.X, event.MouseButton.Y, time);
		  window->PushEvent(event);
		  break;
		}

		// Mouse right button up event
		case WM_RBUTTONUP:
		{
		  Event event;
		  event.Type = Event::EVENT_MOUSE_BUTTON_RELEASED;
		  event.Time = time;
		  event.MouseButton.Button = MOUSEBUTTON_RIGHT;
		  event.MouseButton.X = static_cast<short>(LOWORD(lParam));
		  event.MouseButton.Y = static_cast<short>(HIWORD(lParam));
		  if (HasClick(event.MouseButton.X, event.MouseButton.Y, time, window->GetPrevMouseInfo())) {
			Event eventClick = event;
			eventClick.Type = Event::EVENT_MOUSE_BUTTON_CLICKED;
			window->PushEvent(eventClick);
		  }
		  window->PushEvent(event);
		  break;
		}

		// Mouse wheel button down event
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		{
		  Event event;
		  event.Type = Event::EVENT_MOUSE_BUTTON_PRESSED;
		  event.Time = time;
		  event.MouseButton.Button = MOUSEBUTTON_MIDDLE;
		  event.MouseButton.X = static_cast<int16_t>(LOWORD(lParam));
		  event.MouseButton.Y = static_cast<int16_t>(HIWORD(lParam));
		  window->SetMouseInfo(event.MouseButton.X, event.MouseButton.Y, time);
		  window->PushEvent(event);
		  break;
		}

		// Mouse wheel button up event
		case WM_MBUTTONUP:
		{
		  Event event;
		  event.Type = Event::EVENT_MOUSE_BUTTON_RELEASED;
		  event.Time = time;
		  event.MouseButton.Button = MOUSEBUTTON_MIDDLE;
		  event.MouseButton.X = static_cast<short>(LOWORD(lParam));
		  event.MouseButton.Y = static_cast<short>(HIWORD(lParam));
		  if (HasClick(event.MouseButton.X, event.MouseButton.Y, time, window->GetPrevMouseInfo())) {
			Event eventClick = event;
			eventClick.Type = Event::EVENT_MOUSE_BUTTON_CLICKED;
			window->PushEvent(eventClick);
		  }
		  window->PushEvent(event);
		  break;
		}

		// Mouse X button down event
		case WM_XBUTTONDOWN:
		case WM_XBUTTONDBLCLK:
		{
		  Event event;
		  event.Type = Event::EVENT_MOUSE_BUTTON_PRESSED;
		  event.Time = time;
		  event.MouseButton.Button = (HIWORD(wParam) == XBUTTON1) ? MOUSEBUTTON_BUTTON4 : MOUSEBUTTON_BUTTON5;
		  event.MouseButton.X = static_cast<int16_t>(LOWORD(lParam));
		  event.MouseButton.Y = static_cast<int16_t>(HIWORD(lParam));
		  window->SetMouseInfo(event.MouseButton.X, event.MouseButton.Y, time);
		  window->PushEvent(event);
		  break;
		}

		// Mouse X button up event
		case WM_XBUTTONUP:
		{
		  Event event;
		  event.Type = Event::EVENT_MOUSE_BUTTON_RELEASED;
		  event.Time = time;
		  event.MouseButton.Button = (HIWORD(wParam) == XBUTTON1) ? MOUSEBUTTON_BUTTON4 : MOUSEBUTTON_BUTTON5;
		  event.MouseButton.X = static_cast<short>(LOWORD(lParam));
		  event.MouseButton.Y = static_cast<short>(HIWORD(lParam));
		  if (HasClick(event.MouseButton.X, event.MouseButton.Y, time, window->GetPrevMouseInfo())) {
			Event eventClick = event;
			eventClick.Type = Event::EVENT_MOUSE_BUTTON_CLICKED;
			window->PushEvent(eventClick);
		  }
		  window->PushEvent(event);
		  break;
		}

		case WM_MOUSEMOVE:
		{
		  if (!window->IsMouseInWindow()) {
			window->IsMouseInWindow(true);

			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hWnd;
			TrackMouseEvent(&tme);

			Event event;
			event.Time = time;
			event.Type = Event::EVENT_MOUSE_ENTERED;
			window->PushEvent(event);
		  }

		  const int mouseX = static_cast<short>(LOWORD(lParam));
		  const int mouseY = static_cast<short>(HIWORD(lParam));

		  Event event;
		  event.Type = Event::EVENT_MOUSE_MOVED;
		  event.Time = time;
		  event.MouseMove.X = mouseX;
		  event.MouseMove.Y = mouseY;
		  window->PushEvent(event);
		  break;
		}

		case WM_MOUSELEAVE:
		{
		  Event event;
		  event.Type = Event::EVENT_MOUSE_LEFT;
		  event.Time = time;
		  window->PushEvent(event);
		  window->IsMouseInWindow(false);
		  break;
		}

		case WM_USER:
		{
		  Event testEvent;
		  testEvent.Type = Event::EVENT_TEST;
		  testEvent.Time = time;
		  window->PushEvent(testEvent);
		  break;
		}
		}
	  }
	  return DefWindowProcA(hWnd, message, wParam, lParam);
	}
  } // unnamed namespace

  Win32Window::~Win32Window() {
	Destroy();
  }

  EGLNativeWindowType Win32Window::GetWindowType() const {
	return nativeWindow;
  }

  EGLNativeDisplayType Win32Window::GetDisplayType() const {
	return nativeDisplay;
  }

  void Win32Window::MessageLoop() {
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
	  TranslateMessage(&msg);
	  DispatchMessage(&msg);
	}
  }

  bool Win32Window::Initialize(const char* name, size_t width, size_t height)
  {
	Destroy();
	IsMouseInWindow(false);

	parentClassName = name;
	childClassName = parentClassName + "_Child";

	// Work around compile error from not defining "UNICODE" while Chromium does
	const LPSTR idcArrow = MAKEINTRESOURCEA(32512);

	WNDCLASSEXA parentWindowClass = { 0 };
	parentWindowClass.cbSize = sizeof(WNDCLASSEXA);
	parentWindowClass.style = 0;
	parentWindowClass.lpfnWndProc = WndProc;
	parentWindowClass.cbClsExtra = 0;
	parentWindowClass.cbWndExtra = 0;
	parentWindowClass.hInstance = GetModuleHandle(NULL);
	parentWindowClass.hIcon = NULL;
	parentWindowClass.hCursor = LoadCursorA(NULL, idcArrow);
	parentWindowClass.hbrBackground = 0;
	parentWindowClass.lpszMenuName = NULL;
	parentWindowClass.lpszClassName = parentClassName.c_str();
	if (!RegisterClassExA(&parentWindowClass)) {
	  return false;
	}

	WNDCLASSEXA childWindowClass = { 0 };
	childWindowClass.cbSize = sizeof(WNDCLASSEXA);
	childWindowClass.style = CS_OWNDC;
	childWindowClass.lpfnWndProc = WndProc;
	childWindowClass.cbClsExtra = 0;
	childWindowClass.cbWndExtra = 0;
	childWindowClass.hInstance = GetModuleHandle(NULL);
	childWindowClass.hIcon = NULL;
	childWindowClass.hCursor = LoadCursorA(NULL, idcArrow);
	childWindowClass.hbrBackground = 0;
	childWindowClass.lpszMenuName = NULL;
	childWindowClass.lpszClassName = childClassName.c_str();
	if (!RegisterClassExA(&childWindowClass)) {
	  return false;
	}

	DWORD parentStyle = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
	DWORD parentExtendedStyle = WS_EX_APPWINDOW;

	RECT sizeRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	AdjustWindowRectEx(&sizeRect, parentStyle, FALSE, parentExtendedStyle);

	parentWindow = CreateWindowExA(
	  parentExtendedStyle,
	  parentClassName.c_str(),
	  name,
	  parentStyle,
	  CW_USEDEFAULT,
	  CW_USEDEFAULT,
	  sizeRect.right - sizeRect.left,
	  sizeRect.bottom - sizeRect.top,
	  NULL,
	  NULL,
	  GetModuleHandle(NULL),
	  this
	);

	nativeWindow = CreateWindowExA(
	  0,
	  childClassName.c_str(),
	  name,
	  WS_CHILD,
	  0,
	  0,
	  static_cast<int>(width),
	  static_cast<int>(height),
	  parentWindow,
	  NULL,
	  GetModuleHandle(NULL),
	  this
	);

	nativeDisplay = GetDC(nativeWindow);
	if (!nativeDisplay) {
	  Destroy();
	  return false;
	}

	{
	  Event event;
	  event.Type = Event::EVENT_INIT_WINDOW;
	  event.Time = GetTickCount64();
	  PushEvent(event);
	}

	return true;
  }

  void Win32Window::Destroy()
  {
	if (nativeDisplay) {
	  ReleaseDC(nativeWindow, nativeDisplay);
	  nativeDisplay = 0;
	}

	if (nativeWindow) {
	  DestroyWindow(nativeWindow);
	  nativeWindow = 0;
	}

	if (parentWindow) {
	  DestroyWindow(parentWindow);
	  parentWindow = 0;
	}

	UnregisterClassA(parentClassName.c_str(), NULL);
	UnregisterClassA(childClassName.c_str(), NULL);
  }

  void Win32Window::PushEvent(const Event& e) {
	Window::PushEvent(e);
	switch (e.Type) {
	case Event::EVENT_RESIZED:
	  MoveWindow(nativeWindow, 0, 0, GetWidth(), GetHeight(), FALSE);
	  break;
	default:
	  break;
	}
  }

  void Win32Window::SetVisible(bool isVisible)
  {
	const int flag = isVisible ? SW_SHOW : SW_HIDE;
	ShowWindow(parentWindow, flag);
	ShowWindow(nativeWindow, flag);
  }


  bool Win32Window::SaveUserFile(const char* filename, const void* data, size_t size) const {
	return true;
  }
  size_t Win32Window::GetUserFileSize(const char* filename) const {
	return 0;
  }
  bool Win32Window::LoadUserFile(const char* filename, void* data, size_t size) const {
	return true;
  }
  bool Win32Window::DeleteUserFile(const char* filename) const {
	return true;
  }

} // namespace Mai
