/*
A chams hack for Urban Terror 4.3.4 that both reveals entities through walls and colors these models a bright red. It works by hooking the game's OpenGL function 
"glDrawElements" and disabling depth-testing and textures for OpenGL.
This is done by locating the glDrawElements function inside the OpenGL library and creating a codecave at the start of the function. In the codecave,
we check the amount of vertices associated with the element. If it is over 500, we call glDepthRange to clear the depth clipping plane and glDepthFunc to
disable depth testing. We then disable texture and color arrays and enable color material before setitng the color to red with glColor. 
Otherwise, we call these same functions to re-enable the depth clipping plane and re-enable depth testing and re-enable textures.
This DLL must be injected into the Urban Terror process to work. One way to do this is to use a DLL injector. Another way is to enable AppInit_DLLs in the registry.
The offsets and method to discover them are discussed in the article at: https://gamehacking.academy/lesson/24
*/
#include <Windows.h>
#include <vector>

HMODULE openGLHandle = NULL;

// Function pointers for two OpenGL functions that we will dynamically populate
// after injecting our DLL
void(__stdcall* glDepthFunc)(unsigned int) = NULL;
void(__stdcall* glDepthRange)(double, double) = NULL;

void(__stdcall* glColor4f)(float, float, float, float) = NULL;
void(__stdcall* glEnable)(unsigned int) = NULL;
void(__stdcall* glDisable)(unsigned int) = NULL;
void(__stdcall* glEnableClientState)(unsigned int) = NULL;
void(__stdcall* glDisableClientState)(unsigned int) = NULL;

unsigned char* hook_location;

DWORD ret_address = 0;
DWORD old_protect;
DWORD count = 0;

// Codecave that runs before glDrawElements is called
__declspec(naked) void codecave() {
	// First, we retrieve the count parameter from the original call.
	// Then, we retrieve the value of the count parameter, which specifies the amount
	// of indicies to be rendered
	__asm {
		pushad
		mov eax, dword ptr ds : [esp + 0x10]
		mov count, eax
		popad
		pushad
	}

	// If the count is over 500, we clear the depth clipping plane and then 
	// set the depth function to GL_ALWAYS
	// We then disable color and texture arrays and enable color materials before setting 
	// the color to red
	if (count > 500) {
		(*glDepthRange)(0.0, 0.0);
		(*glDepthFunc)(0x207);

		(*glDisableClientState)(0x8078);
		(*glDisableClientState)(0x8076);
		(*glEnable)(0x0B57);
		(*glColor4f)(1.0f, 0.6f, 0.6f, 1.0f);
	}
	else {
		// Otherwise, restore the depth clipping plane to the game's default value and then
		// set the depth function to GL_LEQUAL and restore textures
		(*glDepthRange)(0.0, 1.0);
		(*glDepthFunc)(0x203);
		
		(*glEnableClientState)(0x8078);
		(*glEnableClientState)(0x8076);
		(*glDisable)(0x0B57);
		(*glColor4f)(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// Finally, restore the original instruction and jump back
	__asm {
		popad
		mov esi, dword ptr ds : [esi + 0xA18]
		jmp ret_address
	}
}

// The injected thread responsible for creating our hooks
void injected_thread() {
	while (true) {
		// Since OpenGL will be loaded dynamically into the process, our thread needs to wait
		// until it sees that the OpenGL module has been loaded.
		if (openGLHandle == NULL) {
			openGLHandle = GetModuleHandle(L"opengl32.dll");
		}

		// Once loaded, we first find the location of the functions we are using in our
		// codecaves above
		if (openGLHandle != NULL && glDepthFunc == NULL) {
			glDepthFunc = (void(__stdcall*)(unsigned int))GetProcAddress(openGLHandle, "glDepthFunc");
			glDepthRange = (void(__stdcall*)(double, double))GetProcAddress(openGLHandle, "glDepthRange");
			glColor4f = (void(__stdcall*)(float, float, float, float))GetProcAddress(openGLHandle, "glColor4f");
			glEnable = (void(__stdcall*)(unsigned int))GetProcAddress(openGLHandle, "glEnable");
			glDisable = (void(__stdcall*)(unsigned int))GetProcAddress(openGLHandle, "glDisable");
			glEnableClientState = (void(__stdcall*)(unsigned int))GetProcAddress(openGLHandle, "glEnableClientState");
			glDisableClientState = (void(__stdcall*)(unsigned int))GetProcAddress(openGLHandle, "glDisableClientState");

			// Then we find the location of glDrawElements and offset to an instruction that is easy to hook
			hook_location = (unsigned char*)GetProcAddress(openGLHandle, "glDrawElements");
			hook_location += 0x16;

			// For the hook, we unprotect the memory at the code we wish to write at
			// Then set the first opcode to E9, or jump
			// Caculate the location using the formula: new_location - original_location+5
			// And finally, since the first original instructions totalled 6 bytes, NOP out the last remaining byte
			VirtualProtect((void*)hook_location, 5, PAGE_EXECUTE_READWRITE, &old_protect);
			*hook_location = 0xE9;
			*(DWORD*)(hook_location + 1) = (DWORD)&codecave - ((DWORD)hook_location + 5);
			*(hook_location + 5) = 0x90;

			// Since OpenGL is loaded dynamically, we need to dynamically calculate the return address
			ret_address = (DWORD)(hook_location + 0x6);
		}

		// So our thread doesn't constantly run, we have it pause execution for a millisecond.
		// This allows the processor to schedule other tasks.
		Sleep(1);
	}
}

// When our DLL is loaded, create a thread in the process to create the hook
// We need to do this as our DLL might be loaded before OpenGL is loaded by the process
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)injected_thread, NULL, 0, NULL);
	}

	return true;
}
