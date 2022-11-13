#include "gui/includes.h"
#include "cheats/Offsets.h"
#include "cheats/userSettings.h"
#include "cheats/background stuff/memoryManager.h"

HANDLE process_handle;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND hwnd;
DWORD procId;
uintptr_t moduleBase;
HANDLE hProcess;

EndScene oEndScene = NULL;
WNDPROC oWndProc;
static HWND window = NULL;

void InitImGui(LPDIRECT3DDEVICE9 pDevice)
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(pDevice);
}

struct view_matrix_t {
	float matrix[16];
} vm;

Vector3 WorldToScreen(const Vector3 pos, view_matrix_t matrix)
{
	struct Vector3 out;
	float _x = matrix.matrix[0] * pos.x + matrix.matrix[1] * pos.y + matrix.matrix[2] * pos.z + matrix.matrix[3];
	float _y = matrix.matrix[4] * pos.x + matrix.matrix[5] * pos.y + matrix.matrix[6] * pos.z + matrix.matrix[7];
	out.z = matrix.matrix[12] * pos.x + matrix.matrix[13] * pos.y + matrix.matrix[14] * pos.z + matrix.matrix[15];

	_x *= 1.f / out.z;
	_y *= 1.f / out.z;

	out.x = 2560 * .5f;
	out.y = 1440 * .5f;

	out.x += 0.5f * _x * 2560 + 0.5f;
	out.y -= 0.5f * _y * 1440 + 0.5f;

	return out;
}

bool init = false;
bool show = false;

long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	if (!init)
	{
		InitImGui(pDevice);
		init = true;
	}

	if (GetAsyncKeyState(VK_END)) {
		kiero::shutdown();
		return 0;
	}

	if (GetAsyncKeyState(VK_INSERT) & 1)
	{
		show = !show;
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (show)
	{
		ImGui::Begin("cheats");
		ImGui::BeginTabBar("main Tab bar");

		//walls Tab
		ImGui::BeginTabItem("ESP", &userSettings::walls::tabActive);
		if (userSettings::walls::tabActive)
		{
			ImGui::Checkbox("Esp", &userSettings::walls::esp);
			ImGui::Checkbox("lines", &userSettings::walls::espOptions::espLines);
			ImGui::Checkbox("Rectangles", &userSettings::walls::espOptions::espRects);
			ImGui::Checkbox("health Bar", &userSettings::walls::espOptions::espHealthBar);
			ImGui::Checkbox("health Bar Changing Colour", &userSettings::walls::espOptions::espHealthbarChangingColour);

			ImGui::Checkbox("Custom Colours", &userSettings::walls::espOptions::customColours);
			if (userSettings::walls::espOptions::customColours)
			{
				ImGui::SliderInt("R", &userSettings::walls::espOptions::enemyColourR, 0, 255);
				ImGui::SliderInt("G", &userSettings::walls::espOptions::enemyColourG, 0, 255);
				ImGui::SliderInt("B", &userSettings::walls::espOptions::enemyColourB, 0, 255);
			}
		}

		ImGui::EndTabItem();



		ImGui::EndTabBar();
		ImGui::End();

	}
	

	if (userSettings::walls::esp)
	{
		//TODO: run esp from another function
		ImDrawList* drawlist = ImGui::GetBackgroundDrawList();
		view_matrix_t vm = memoryManager::RPM<view_matrix_t>(moduleBase + dwViewMatrix, hProcess);
		for (int i = 1; i < 64; i++)
		{
			using namespace memoryManager;
			uintptr_t player = RPM<uintptr_t>(moduleBase + dwEntityList + i * 0x10, hProcess);
			uintptr_t localPlayer = memoryManager::RPM<uintptr_t>(moduleBase + dwLocalPlayer, hProcess);
			int localTeam = memoryManager::RPM<int>(localPlayer + m_iTeamNum, hProcess);

			if (localTeam != RPM<int>(player + m_iTeamNum, hProcess))
			{
				int health = RPM<int>(player + m_iHealth, hProcess);
				if (health > 100 || health < 1) continue;
				if (RPM<int>(player + m_bDormant, hProcess) == true) continue;
				Vector3 PlayerLocation = RPM<Vector3>(player + m_vecOrigin, hProcess);
				struct Vector3 PlayerHead;
				PlayerHead.x = PlayerLocation.x;
				PlayerHead.y = PlayerLocation.y;
				PlayerHead.z = PlayerLocation.z + 75.f;

				struct Vector3  PlayerScreen = WorldToScreen(PlayerLocation, vm);
				struct Vector3 PlayerHeadScreen = WorldToScreen(PlayerHead, vm);
				float height = PlayerHeadScreen.y - PlayerScreen.y;
				float width = height / 2.4f;

				if (PlayerScreen.z >= 0.1f) {
					using namespace userSettings::walls::espOptions;
					if (userSettings::walls::espOptions::espLines == true)
						drawlist->AddLine(ImVec2(2560 / 2, 1440), ImVec2(PlayerScreen.x, PlayerScreen.y), ImColor(enemyColourR, enemyColourG, enemyColourB), 2.0f);
					if (userSettings::walls::espOptions::espRects == true)
						drawlist->AddRect(ImVec2(PlayerHeadScreen.x - (width / 2), PlayerHeadScreen.y), ImVec2(PlayerScreen.x + (width / 2), PlayerScreen.y), ImColor(enemyColourR, enemyColourG, enemyColourB), 0.0f, 15, 2.0f);
					if (userSettings::walls::espOptions::espHealthBar)
					{

						ImColor healthColour;
						if (userSettings::walls::espOptions::espHealthbarChangingColour)
						{
							if (health >= 90)
							{
								healthColour = ImColor(0, 255, 0);
							}
							else if (health >= 70)
							{
								healthColour = ImColor(160, 255, 0);
							}
							else if (health >= 50)
							{
								healthColour = ImColor(255, 255, 0);
							}
							else if (health >= 30)
							{
								healthColour = ImColor(255, 180, 0);
							}
							else if (health >= 15)
							{
								healthColour = ImColor(255, 120, 0);
							}
							else if (health >= 1)
							{
								healthColour = ImColor(255, 0, 0);
							}
						}

						//healthBar
						ImVec2 p_max, p_min;
						p_max.x = PlayerHeadScreen.x + (width / 1.5f);
						p_max.y = PlayerHeadScreen.y - (height / 5);

						p_min.x = PlayerScreen.x + width;
						p_min.y = PlayerScreen.y + (height / 5);
						drawlist->AddRect(p_min, p_max, ImColor(enemyColourR, enemyColourG, enemyColourB), 0.0f, 15, 2.0f);

						//health
						ImVec2 p_max_health;
						p_max_health.x = p_max.x;

						p_max_health.y = (p_min.y - p_max.y) / 100;
						p_max_health.y = p_min.y - (p_max_health.y * health);

						drawlist->AddRectFilled(p_min, p_max_health, healthColour, 0.0f);
					}
					
				}
			}
		}
	}
		
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return oEndScene(pDevice);
}



LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
	DWORD wndProcId;
	GetWindowThreadProcessId(handle, &wndProcId);

	if (GetCurrentProcessId() != wndProcId)
		return TRUE; // skip to next window

	window = handle;
	return FALSE; // window found abort search
}

HWND GetProcessWindow()
{
	window = NULL;
	EnumWindows(EnumWindowsCallback, NULL);
	return window;
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	hwnd = FindWindowA(NULL, "Counter-Strike: Global Offensive - Direct3D 9");
	GetWindowThreadProcessId(hwnd, &procId);
	moduleBase = memoryManager::GetModuleAddress("client.dll", procId);
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);

	bool attached = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success)
		{
			kiero::bind(42, (void**)& oEndScene, hkEndScene);
			do
				window = GetProcessWindow();
			while (window == NULL);
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);
			attached = true;
		}
	} while (!attached);
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}
