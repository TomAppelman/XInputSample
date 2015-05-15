#include "platform.h"
#include "InputInterface.h"
#include <stdio.h>

//
Window				inputWindow;
InputInterface		inputInterface;
bool				running = true;

//
LRESULT CALLBACK InputWndCallback(HWND hwnd, UINT MessageID, WPARAM wParam, LPARAM lParam){
	switch (MessageID){
	default:break;
	case WM_CREATE:
		appInit();
		appOpenConsole();
		if (!inputInterface.init(hwnd)){
			return false;
		}
		break;
	case WM_CLOSE:
		inputWindow.Destroy();
		break;
	case WM_DESTROY:
		inputInterface.shutdown();
		appCloseConsole();
		running = false;
		PostQuitMessage(0);
		break;
	case WM_INPUT:
		inputInterface.handleMessage((HRAWINPUT*)&lParam);
		break;
	case WM_INPUT_DEVICE_CHANGE:
		if (wParam == GIDC_ARRIVAL){
			inputInterface.deviceAdded((HANDLE)lParam);
		}
		if (wParam == GIDC_REMOVAL){
			inputInterface.deviceRemoved((HANDLE)lParam);
		}
		break;
	}
	return DefWindowProc(hwnd, MessageID, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, wchar_t* cmdline, int nShow){
	MSG msg;
	RECT wndRect = { 0, 0, 500, 400 };
	XINPUT_STATE controller_state[4];
	XINPUT_STATE old_controller_state[4];
	double CurrentTime = appGetTimeMs();
	double LastTime = appGetTimeMs();
	const double UpdateTime = 1.0 / 30.0;
	if (!inputWindow.Create(L"Input dummy window", wndRect, InputWndCallback, true)){
		return false;
	}
	while (running){
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// lock update at 30 hz
		CurrentTime = appGetTimeMs();
		const double dt = CurrentTime - LastTime;
		if (dt >= UpdateTime){
			LastTime = CurrentTime;
			for (unsigned int i = 0; i < 4; i++){
				ZeroMemory(&controller_state, sizeof(XINPUT_STATE));
				if (inputInterface.getXInputState(controller_state[i], i)){
					// check if there are changes in the state of the controller
					if (controller_state[i].dwPacketNumber != old_controller_state[i].dwPacketNumber)
					{
						printf("Controller %i : buttons(0x%04x), stick -left(%i,%i) -right(%i,%i)\n",
							i,
							controller_state[i].Gamepad.wButtons,
							controller_state[i].Gamepad.sThumbLX,
							controller_state[i].Gamepad.sThumbLY,
							controller_state[i].Gamepad.sThumbRX,
							controller_state[i].Gamepad.sThumbRY);
					}
					// check if user press back + start, if so destroy window(and close application);
					if (controller_state[i].Gamepad.wButtons == (XINPUT_GAMEPAD_BACK | XINPUT_GAMEPAD_START)){
						inputWindow.Destroy();
					}
					if ((controller_state[i].Gamepad.wButtons == (XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_B))){
						inputInterface.setXInputRumble(65535 / 3, 65535 / 3, i);
					}
					else{
						if (controller_state[i].Gamepad.wButtons & (XINPUT_GAMEPAD_X)){
							inputInterface.setXInputRumble(65535 / 3, 0, i);
						}
						if (controller_state[i].Gamepad.wButtons & (XINPUT_GAMEPAD_B)){
							inputInterface.setXInputRumble(0, 65535 / 3, i);
						}
					}
					if (!(controller_state[i].Gamepad.wButtons & (XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_B))){
						inputInterface.setXInputRumble(0, 0, i);
					}
					old_controller_state[i] = controller_state[i];
				}
			}
		}
		Sleep(0);
	}
	return (int)msg.wParam;
}
