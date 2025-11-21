#include "framework.h"
#include "ShapeDisplay.h"
#include <commdlg.h>
#include <vector>
#include <string>

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

HBITMAP g_hBitmap = NULL;
HBITMAP g_hBuffer = NULL;
BITMAP g_bitmapInfo = {};
POINT g_imagePosition = { 20, 0 };
bool g_isDragging = false;
POINT g_dragOffset = { 0, 0 };
std::vector<POINT> g_shapePixels;
COLORREF g_currentShapeColor = RGB(64, 224, 208);

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

HBITMAP LoadBMPFile(HWND hwnd);
void AnalyzeImage(HBITMAP hBitmap);
void PrepareBuffer(HWND hWnd);
void DrawImage(HDC hdc);
void ChangeShapeColor(COLORREF newColor);
bool IsPointInShape(int x, int y);
void DrawTextOnImage(HDC hdcMem);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SHAPEDISPLAY, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SHAPEDISPLAY));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SHAPEDISPLAY));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SHAPEDISPLAY);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(
        szWindowClass, szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 800, 600,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

HBITMAP LoadBMPFile(HWND hwnd) {
    OPENFILENAME ofn{};
    TCHAR szFile[260] = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = TEXT("BMP Files\0*.bmp\0All Files\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, szFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        if (hBitmap) {
            GetObject(hBitmap, sizeof(BITMAP), &g_bitmapInfo);
            AnalyzeImage(hBitmap);
            return hBitmap;
        }
    }
    return NULL;
}

void AnalyzeImage(HBITMAP hBitmap) {
    g_shapePixels.clear();

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    SelectObject(hdcMem, hBitmap);

    COLORREF bgColor = GetPixel(hdcMem, 0, 0);
    const int tolerance = 10;

    for (int y = 0; y < g_bitmapInfo.bmHeight; y++)
        for (int x = 0; x < g_bitmapInfo.bmWidth; x++)
        {
            COLORREF pixel = GetPixel(hdcMem, x, y);
            if (abs(GetRValue(pixel) - GetRValue(bgColor)) > tolerance ||
                abs(GetGValue(pixel) - GetGValue(bgColor)) > tolerance ||
                abs(GetBValue(pixel) - GetBValue(bgColor)) > tolerance)
                g_shapePixels.push_back({ x, y });
        }

    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void PrepareBuffer(HWND hWnd) {
    if (!g_shapePixels.empty()) {
        RECT rect;
        GetClientRect(hWnd, &rect);

        HDC hdc = GetDC(hWnd);
        if (g_hBuffer) DeleteObject(g_hBuffer);

        HDC hdcMem = CreateCompatibleDC(hdc);
        g_hBuffer = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, g_hBuffer);

        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdcMem, &rect, hBrush);
        DeleteObject(hBrush);

        for (const auto& pt : g_shapePixels) {
            SetPixel(hdcMem, g_imagePosition.x + pt.x, g_imagePosition.y + pt.y, g_currentShapeColor);
        }

        DrawTextOnImage(hdcMem);

        SelectObject(hdcMem, hOld);
        DeleteDC(hdcMem);
        ReleaseDC(hWnd, hdc);
    }
}

void DrawTextOnImage(HDC hdcMem) {
    if (g_shapePixels.empty()) return;

    int minX = g_bitmapInfo.bmWidth, maxX = 0, minY = g_bitmapInfo.bmHeight, maxY = 0;
    for (const auto& pt : g_shapePixels) {
        if (pt.x < minX) minX = pt.x;
        if (pt.x > maxX) maxX = pt.x;
        if (pt.y < minY) minY = pt.y;
        if (pt.y > maxY) maxY = pt.y;
    }

    int centerX = g_imagePosition.x + (minX + maxX) / 2;
    int centerY = g_imagePosition.y + (minY + maxY) / 2;

    HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
    HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);

    SetTextColor(hdcMem, RGB(0, 0, 0));
    SetBkMode(hdcMem, TRANSPARENT);

    std::string text = "Малышева Олеся";
    SIZE sz;
    GetTextExtentPoint32A(hdcMem, text.c_str(), (int)text.length(), &sz);
    TextOutA(hdcMem, centerX - sz.cx / 2, centerY - sz.cy / 2, text.c_str(), (int)text.length());

    SelectObject(hdcMem, hOldFont);
    DeleteObject(hFont);
}

void DrawImage(HDC hdc) {
    if (g_hBuffer) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, g_hBuffer);

        RECT rect;
        GetClientRect(GetActiveWindow(), &rect);
        BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, hOld);
        DeleteDC(hdcMem);
    }
}

void ChangeShapeColor(COLORREF newColor) {
    g_currentShapeColor = newColor;
    PrepareBuffer(GetActiveWindow());
    InvalidateRect(GetActiveWindow(), NULL, FALSE);
}

bool IsPointInShape(int x, int y) {
    int imgX = x - g_imagePosition.x;
    int imgY = y - g_imagePosition.y;
    for (const auto& pt : g_shapePixels)
        if (pt.x == imgX && pt.y == imgY)
            return true;
    return false;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        g_hBitmap = LoadBMPFile(hWnd);
        if (g_hBitmap)
        {
            RECT rect;
            GetClientRect(hWnd, &rect);
            g_imagePosition.y = (rect.bottom - g_bitmapInfo.bmHeight) / 2;
            PrepareBuffer(hWnd);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_ABOUT: DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About); break;
        case IDM_OPEN:
            if (g_hBitmap) DeleteObject(g_hBitmap);
            g_hBitmap = LoadBMPFile(hWnd);
            PrepareBuffer(hWnd);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case IDM_EXIT: DestroyWindow(hWnd); break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        DrawImage(hdc);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_SIZE:
    {
        RECT rect;
        GetClientRect(hWnd, &rect);
        g_imagePosition.y = (rect.bottom - g_bitmapInfo.bmHeight) / 2;
        PrepareBuffer(hWnd);
        InvalidateRect(hWnd, NULL, TRUE);
    }
    break;

    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam), y = HIWORD(lParam);
        if (IsPointInShape(x, y)) {
            g_isDragging = true;
            g_dragOffset.x = x - g_imagePosition.x;
            g_dragOffset.y = y - g_imagePosition.y;
            SetCapture(hWnd);
        }
    }
    break;

    case WM_RBUTTONDOWN:
    {
        int x = LOWORD(lParam), y = HIWORD(lParam);
        if (IsPointInShape(x, y)) {
            static COLORREF colors[] = {
                RGB(64, 224, 208),
                RGB(255, 0, 0),
                RGB(0, 255, 0),
                RGB(0, 0, 255),
                RGB(255, 255, 0),
                RGB(255, 0, 255),
                RGB(0, 255, 255),
                RGB(128, 0, 128)
            };
            static int colorIndex = 0;
            colorIndex = (colorIndex + 1) % (sizeof(colors) / sizeof(colors[0]));
            ChangeShapeColor(colors[colorIndex]);
        }
    }
    break;

    case WM_MOUSEMOVE:
        if (g_isDragging) {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_imagePosition.x = x - g_dragOffset.x;
            g_imagePosition.y = y - g_dragOffset.y;
            PrepareBuffer(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;

    case WM_LBUTTONUP:
        if (g_isDragging) {
            g_isDragging = false;
            ReleaseCapture();
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_F2) {
            if (g_hBitmap) DeleteObject(g_hBitmap);
            g_hBitmap = LoadBMPFile(hWnd);
            PrepareBuffer(hWnd);
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;

    case WM_DESTROY:
        if (g_hBitmap) DeleteObject(g_hBitmap);
        if (g_hBuffer) DeleteObject(g_hBuffer);
        PostQuitMessage(0);
        break;

    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}