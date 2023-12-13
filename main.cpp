#include <iostream>
#include <windows.h>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <direct.h>
#include <cctype>
#include <shlobj_core.h>
#include <filesystem>
#include <fstream>


void setConsoleColor(int text, int background) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), text | (background << 4));
}

using namespace std;
char currentDirectory[MAX_PATH];
wstring userProfile;

std::string username;  // global variable for username
std::string computername;  // global variable for computername

void setWindowTitle() {
    std::string title = username + "@" + computername;
    std::wstring wTitle = std::wstring(title.begin(), title.end());
    SetConsoleTitle(wTitle.c_str());
}

void writeUI() {
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

void saveToHistory(const std::wstring& command) {
    wchar_t userProfile[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, userProfile))) {
        std::wstring historyFolder = userProfile;
        historyFolder += L"\\kaliCMD";
        std::filesystem::create_directories(historyFolder); // create folder "kaliCMD" in the current user dir if not existant
        std::wstring filePath = historyFolder + L"\\history.txt";
        std::wofstream historyFile(filePath, std::ios_base::app);
        if (historyFile.is_open()) {
            historyFile << command << std::endl;
            historyFile.close();
        }
    }
}

std::string convertToUTF8(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

int main() {
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
    cout << "Version: 1.0" << endl;
    cout << "Made by @thewindev" << endl;


    while (true) {
        std::wcout << "\n"; // new line before displaying UI
        writeUI(); // display UI

        std::wstring inputCmd;
        std::wcout << "";
        std::getline(std::wcin, inputCmd);

        saveToHistory(inputCmd);

        // convert the entered command to lowercase for case-insensitive comparison
        std::wstring command = inputCmd.substr(0, inputCmd.find(L' '));
        std::transform(command.begin(), command.end(), command.begin(), ::towlower);

        if (command == L"cls") {
            system("cls"); // Clear the console
        }
        else if (command == L"exit") {
            break; // exit the loop and end the program
        }
        else if (command == L"cd") {
            std::string directory = convertToUTF8(inputCmd.substr(inputCmd.find(L' ') + 1));
            _chdir(directory.c_str());
        }
        else if (inputCmd.length() >= 2 && iswalpha(inputCmd[0]) && inputCmd[1] == L':') {
            // if driveletter was specified (e.g. F:)
            wchar_t driveLetter = towupper(inputCmd[0]); // convert to uppercase
            std::string letter(1, static_cast<char>(driveLetter));
            letter += ":\\";
            std::filesystem::current_path(letter);
        }
        else if (!command.empty()) {
            // execute the entered command using system
            system(convertToUTF8(inputCmd).c_str());
        }
    }

    return 0;
}
