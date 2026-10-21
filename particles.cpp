#include <windows.h>
#include <gdiplus.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <wchar.h>
#include <chrono>
#include <thread>

#pragma comment (lib,"Gdiplus.lib")

using namespace Gdiplus;

#define PI 3.14159265f

int screenWidth = 1200;
int screenHeight = 800;

struct Player {
    float x = 0.0f;
    float y = 50.0f;
    float w = 25.0f;
    float h = 25.0f;
    float angle = 0.0f;
} player;

struct Food {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    int lifeTime = 0;
    bool isCreated = false;
} food;

struct Particle {
    static const int n = 100;
    bool particle[n] = { false };
    float x[n], y[n];
    int size[n];
    float speed[n], acceleration[n];
    float angle[n];
    int colorYellow[n];
    bool canBeCreated = true;
} particles;

int score = 0;

HDC hdcMem;
HBITMAP hbmMem;
ULONG_PTR gdiplusToken;

void Update() {
    player.angle += 1.0f;
    if (player.angle >= 360.0f) player.angle -= 360.0f;

    float sped = 7.0f;

    if (GetAsyncKeyState('W') & 0x8000) player.y += sped;
    if (GetAsyncKeyState('S') & 0x8000) player.y -= sped;
    if (GetAsyncKeyState('A') & 0x8000) player.x -= sped;
    if (GetAsyncKeyState('D') & 0x8000) player.x += sped;

    if (!food.isCreated) {
        food.isCreated = true;
        food.w = (float)(rand() % 21 + 20);
        food.h = food.w;

        food.x = (float)((rand() % (screenWidth - 2 * (int)food.w)) - (screenWidth / 2) + (int)food.w);
        food.y = (float)((rand() % (screenHeight - 2 * (int)food.h)) - (screenHeight / 2) + (int)food.h);

        food.lifeTime = rand() % 41 + 80;
    }
    else {
        food.lifeTime--;
        if (food.lifeTime <= 0) {
            food.isCreated = false;
        }
    }

    if (food.isCreated && player.x >= food.x && player.x <= food.x + food.w &&
        player.y >= food.y && player.y <= food.y + food.h) {
        food.isCreated = false;
        score++;
    }

    for (int i = 0; i < Particle::n; i++) {
        if (!particles.particle[i] && particles.canBeCreated) {
            particles.canBeCreated = false;
            particles.particle[i] = true;

            particles.angle[i] = (float)(rand() % 360);
            particles.speed[i] = (float)(rand() % 5 + 8);
            particles.colorYellow[i] = 255;
            particles.acceleration[i] = (float)-(rand() % 2 + 1);

            particles.x[i] = player.x;
            particles.y[i] = player.y;
            particles.size[i] = rand() % 10 + 6;
        }
        else if (particles.particle[i]) {
            particles.speed[i] += particles.acceleration[i];

            float ar = particles.angle[i] * (PI / 180.0f);
            particles.x[i] += (int)(particles.speed[i] * cos(ar));
            particles.y[i] += (int)(particles.speed[i] * sin(ar));

            if (particles.colorYellow[i] + particles.acceleration[i] - 20 >= 0) {
                particles.colorYellow[i] -= 15;
                if (particles.colorYellow[i] < 0) particles.colorYellow[i] = 0;
            }

            if (particles.speed[i] <= 0) {
                particles.speed[i] = 0;
                particles.acceleration[i] = 0;
                if (particles.colorYellow[i] > 0) {
                    particles.colorYellow[i]--;
                }
                else {
                    particles.particle[i] = false;
                }
            }
        }
    }
}

void Draw(HDC hdc) {
    Graphics graphics(hdc);

    graphics.SetSmoothingMode(SmoothingModeNone);
    graphics.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    graphics.Clear(Color(255, 0, 0, 0));

    graphics.TranslateTransform(screenWidth / 2.0f, screenHeight / 2.0f);
    graphics.ScaleTransform(1.0f, -1.0f);

    for (int i = 0; i < Particle::n; i++) {
        if (particles.particle[i]) {
            SolidBrush brush(Color(255, 255, particles.colorYellow[i], 0));
            graphics.FillRectangle(&brush, particles.x[i], particles.y[i], (float)particles.size[i], (float)particles.size[i]);
        }
    }

    GraphicsState state = graphics.Save();
    SolidBrush pBrush(Color(255, 255, 255, 127));

    graphics.TranslateTransform(player.x, player.y);
    graphics.RotateTransform(player.angle);
    graphics.FillRectangle(&pBrush, -player.w / 2.0f, -player.h / 2.0f, player.w, player.h);

    graphics.Restore(state);

    if (food.isCreated) {
        SolidBrush fBrush(Color(255, 127, 127, 255));
        graphics.FillRectangle(&fBrush, food.x, food.y, food.w, food.h);
    }

    graphics.ResetTransform();

    Font font(L"Arial", 14);
    SolidBrush textBrush(Color(255, 255, 255, 255));

    wchar_t timeText[64];
    swprintf_s(timeText, L"Время жизни: %d", food.lifeTime > 0 ? food.lifeTime : 0);
    graphics.DrawString(timeText, -1, &font, PointF(10.0f, 10.0f), &textBrush);

    wchar_t scoreText[64];
    swprintf_s(scoreText, L"Счет: %d", score);
    graphics.DrawString(scoreText, -1, &font, PointF(10.0f, 40.0f), &textBrush);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        srand((unsigned int)time(NULL));

        HDC hdc = GetDC(hwnd);
        hdcMem = CreateCompatibleDC(hdc);
        hbmMem = CreateCompatibleBitmap(hdc, screenWidth, screenHeight);
        SelectObject(hdcMem, hbmMem);
        ReleaseDC(hwnd, hdc);

        SetTimer(hwnd, 2, 30, NULL);
        break;
    }

    case WM_SIZE: {
        screenWidth = LOWORD(lParam);
        screenHeight = HIWORD(lParam);

        if (hbmMem) DeleteObject(hbmMem);
        HDC hdc = GetDC(hwnd);
        hbmMem = CreateCompatibleBitmap(hdc, screenWidth, screenHeight);
        SelectObject(hdcMem, hbmMem);
        ReleaseDC(hwnd, hdc);
        break;
    }

    case WM_TIMER: {
        if (wParam == 2) {
            particles.canBeCreated = !particles.canBeCreated;
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        Draw(hdcMem);
        BitBlt(hdc, 0, 0, screenWidth, screenHeight, hdcMem, 0, 0, SRCCOPY);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY: {
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    const wchar_t CLASS_NAME[] = L"GameWindowClass";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Particles game",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, screenWidth + 16, screenHeight + 39,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    auto lastTime = std::chrono::high_resolution_clock::now();
    const float targetFPS = 60.0f;
    const auto frameDuration = std::chrono::duration<float>(1.0f / targetFPS);

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            auto currentTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> elapsed = currentTime - lastTime;

            if (elapsed >= frameDuration) {
                Update();
                InvalidateRect(hwnd, NULL, FALSE);
                lastTime = currentTime;
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}