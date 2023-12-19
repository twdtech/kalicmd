﻿#include <iostream>
#include <windows.h>
#include <string>
#include <algorithm>
#include <direct.h>
#include <cctype>
#include <shlobj_core.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <atomic>
#include <csignal>

using namespace std;
char currentDirectory[MAX_PATH];
std::string username;  // global variable for username
std::string computername;  // global variable for computername
std::atomic<bool> stopCommand(false);
wstring userProfile;
int ctrl_press = 0;

static std::string convertToUTF8(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static std::wstring convertToWideString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

static std::string convertWideStringToString(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static void setConsoleColor(int text, int background) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), text | (background << 4));
}

static void setWindowTitle() {
    std::string title = username + "@" + computername;
    std::wstring wTitle = std::wstring(title.begin(), title.end());
    SetConsoleTitle(wTitle.c_str());
}

static void writeUI() {
    bool isAdmin = IsUserAnAdmin() != 0;

    if (isAdmin) {
        username = "Administrator";
    }

    GetCurrentDirectoryA(MAX_PATH, currentDirectory);

    setConsoleColor(15, 0);

    setConsoleColor(9, 0);
    const wchar_t firstString[] = L"┌──(";
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), firstString, wcslen(firstString), nullptr, nullptr);

    setConsoleColor(10, 0);
    wprintf(L"%hs@%hs", username.c_str(), computername.c_str());

    setConsoleColor(9, 0);
    wprintf(L")-[");

    setConsoleColor(15, 0);
    printf("%s", currentDirectory);

    setConsoleColor(9, 0);
    printf("]\n");

    setConsoleColor(9, 0);
    const wchar_t secondString[] = L"└─";
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), secondString, wcslen(secondString), nullptr, nullptr);

    setConsoleColor(10, 0);
    printf("$ ");

    setConsoleColor(15, 0);

    setWindowTitle();
}

static void saveToHistory(const std::wstring& command) {
    static bool isDirectoryCreated = false;
    wchar_t userProfile[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, userProfile))) {
        std::wstring historyFolder = userProfile;
        historyFolder += L"\\kaliCMD";

        if (!isDirectoryCreated) {
            std::filesystem::create_directories(historyFolder); // create "kaliCMD" folder if it doesn't exist in userprofile folder
            isDirectoryCreated = true;
        }

        std::wstring filePath = historyFolder + L"\\history.txt";
        std::wofstream historyFile(filePath, std::ios_base::app);
        if (historyFile.is_open()) {
            historyFile << command << std::endl;
            historyFile.close();
        }
    }
}

static void signalHandler(int signum) {
    stopCommand = true;
}

static void handle_signal(int signal) {
    if (signal == SIGINT) {
        // Wenn Ctrl+C gedrückt wird, erhöhen Sie den Wert der Variable um 1
        ctrl_press++;
        std::cout << "Der Wert der Variable ist jetzt: " << ctrl_press << std::endl;
    }
}

int main() {
    // register the signal handler
    std::signal(SIGINT, signalHandler);

    const int UNLEN = 256;
    char usernameBuffer[UNLEN + 1];
    DWORD size = UNLEN + 1;
    GetUserNameA(usernameBuffer, &size);
    username = usernameBuffer;  // set global username

    char computernameBuffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD sizeComputerName = sizeof(computernameBuffer) / sizeof(computernameBuffer[0]);
    GetComputerNameA(computernameBuffer, &sizeComputerName);
    computername = computernameBuffer;  // set global computername

    SetConsoleOutputCP(CP_UTF8);
    cout << "Kali Linux Style Windows Command Prompt" << endl;
    cout << "Version: 1.0.1" << endl;
    cout << "Made by @thewindev" << endl;

    while (true) {

        std::wcout << "\n"; // new line before displaying UI
        writeUI(); // write UI

        std::wstring inputCmd;
        std::wcout << "";
        std::getline(std::wcin, inputCmd);

        

        saveToHistory(inputCmd);

        // convert the entered command to lowercase for case-insensitive comparison
        std::wstring command = inputCmd.substr(0, inputCmd.find(L' '));
        std::transform(command.begin(), command.end(), command.begin(), ::towlower);
        
        if (command == L"cls") {
            system("cls"); // clear the console
        }
        else if (command == L"exit") {
            break; // exit the loop and end the program
        }
        else if (command == L"cd") {
            std::wstring directory = inputCmd.substr(inputCmd.find(L' ') + 1);

            // Überprüfen Sie, ob der Pfad absolut oder relativ ist
            if (directory.front() != L'\\' && directory.front() != L'/') {
                // Wenn der Pfad relativ ist, fügen Sie das aktuelle Verzeichnis hinzu
                WCHAR currentDir[MAX_PATH];
                GetCurrentDirectory(MAX_PATH, currentDir);
                directory = std::wstring(currentDir) + L"\\" + directory;
            }

            // Prüfen, ob cd \ eingegeben wurde, und dann ins Stammverzeichnis wechseln
            if (directory == L"\\" || directory == L"/") {
                SetCurrentDirectory(L"\\");
            }
            else {
                // Öffnen Sie das Verzeichnis und suchen Sie nach Übereinstimmungen
                WIN32_FIND_DATA findData;
                HANDLE hFind = FindFirstFile(directory.c_str(), &findData);
                if (hFind == INVALID_HANDLE_VALUE) {
                    // Fehler beim Öffnen des Verzeichnisses
                    std::wcerr << L"Directory " << directory << L" does not exist." << std::endl;
                }
                else {
                    // Überprüfen Sie, ob das gefundene Element ein Verzeichnis ist
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        // Wechseln Sie das Verzeichnis, wenn es ein Verzeichnis ist
                        SetCurrentDirectory(directory.c_str());
                    }
                    else {
                        // Fehler, wenn das gefundene Element kein Verzeichnis ist
                        std::wcerr << L"Error: " << directory << L" is not a directory." << std::endl;
                    }

                    FindClose(hFind);
                }
            }
        }
        else if (inputCmd.length() >= 2 && iswalpha(inputCmd[0]) && inputCmd[1] == L':') {
            // if command is drive letter...
            wchar_t driveLetter = towupper(inputCmd[0]); // convert to uppercase
            std::string letter(1, static_cast<char>(driveLetter));
            letter += ":\\";
            std::filesystem::current_path(letter);
        }
        else if (command == L"about") {
            printf("Console Version: 1.0.1\n"); // application information
            printf("Creator: github.com/twdtech\n");
        }
        else if (command.substr(0, 4) == L"sudo") {
            printf("Coming soon ...\n");
        }
        else if (!command.empty()) {
            // execute the entered command using system
            system(convertToUTF8(inputCmd).c_str());
        }
    }

    return 0;
}