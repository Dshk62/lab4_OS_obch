//Receiver
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <vector>

using namespace std;

int main() {
    setlocale(LC_ALL, "rus");

    wchar_t fileName[50];
    wstring filename;
    int kol_sent, kolSenders;

    // 1. ������ � ������� ��� ��������� ����� � ���������� ������� � �������� �����
    cout << "������� ��� �����: " << endl;
    wcin >> filename;

    cout << "������� ���������� �������: " << endl;
    wcin >> kol_sent;
    bool flag = true;
    if (kol_sent <= 0) {
        while (flag) {
            cout << "������������ ���������� �������, ������� ����� >=1" << endl;
            wcin >> kol_sent;
            if (kol_sent > 0) flag = false;
        }
    }

    // ������ �������� ����
    ofstream out(filename, ios::binary | ios::app);
    out.close();
    ifstream in(filename, ios::binary);
    in.close();

    // �������� ���������������� ��������
    HANDLE hMutex;
    HANDLE hFullSemaphore, hEmptySemaphore;
    HANDLE* readyEvents = nullptr;

    hMutex = CreateMutex(NULL, FALSE, L"SyncMutex");
    hFullSemaphore = CreateSemaphore(NULL, 0, kol_sent, L"FullSemaphore");
    hEmptySemaphore = CreateSemaphore(NULL, kol_sent, kol_sent, L"EmptySemaphore");

    if (!hMutex) {
        cout << "������ �������� ��������" << endl;
        return GetLastError();
    }
    if (!hFullSemaphore || !hEmptySemaphore) {
        cout << "������ �������� ���������" << endl;
        return GetLastError();
    }

    WaitForSingleObject(hMutex, INFINITE);

    // ���� ���������� ��������� Sender
    cout << "������� ���������� ��������� Sender: " << endl;
    cin >> kolSenders;
    flag = true;

    if (kolSenders <= 0) {
        while (flag) {
            cout << "������������ ���������� ��������� Sender, ������� ����� >=1" << endl;
            cin >> kolSenders;
            if (kolSenders > 0) flag = false;
        }
    }

    readyEvents = new HANDLE[kolSenders];

    // �������� ������� ��� ������� �������� Sender
    for (int i = 0; i < kolSenders; i++) {
        wstring eventName = L"SenderReady_" + to_wstring(i);
        readyEvents[i] = CreateEvent(NULL, TRUE, FALSE, eventName.c_str());
        if (!readyEvents[i]) {
            cout << "������ �������� ������� " << i << ": " << GetLastError() << endl;
            return GetLastError();
        }
    }

    STARTUPINFO* si = new STARTUPINFO[kolSenders];
    PROCESS_INFORMATION* pi = new PROCESS_INFORMATION[kolSenders];
    HANDLE* hProcessors = new HANDLE[kolSenders];
    ZeroMemory(si, sizeof(STARTUPINFO) * kolSenders);

    wstring s = L"Sender.exe " + filename;
    lstrcpyW(fileName, s.data());

    // ������ ��������� Sender
    for (int i = 0; i < kolSenders; i++) {
        si[i].cb = sizeof(STARTUPINFO);
        wstring command = L"Sender.exe " + filename + L" " + to_wstring(i); // �������� ������
        lstrcpyW(fileName, command.c_str());

        if (!CreateProcess(NULL, fileName, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si[i], &pi[i])) {
            cout << "������ �������� �������� " << i << endl;
            return GetLastError();
        }
        hProcessors[i] = pi[i].hProcess;
    }

    // ������� ������� ���������� �� ���� ��������� Sender
    WaitForMultipleObjects(kolSenders, readyEvents, TRUE, INFINITE);
    cout << "��� Sender �������� ������ � ������." << endl;

    // ���� ������ ���������
    ReleaseMutex(hMutex);
    string comand;
    
    char* messeng = new char[20];
    string del;
    while (true) {
        cout << "�������� �������� exit ��� read: " << endl;
        cin >> comand;

        if (comand == "read") {
            WaitForSingleObject(hFullSemaphore, INFINITE);
            WaitForSingleObject(hMutex, INFINITE);

            // ������ ������� ���������
            in.open(filename, ios::binary);
            char buffer[20] = {};
            in.read(buffer, 20);

            if (in.gcount() == 0) {
                cout << "���� ����, ��������� �����������." << endl;
                in.close();
                ReleaseMutex(hMutex);
                ReleaseSemaphore(hEmptySemaphore, 1, NULL);
                continue;
            }

            string message(buffer);
            cout << "��������� ���������: " << message << endl;

            // ���������� ���������� ���������
            vector<string> remainingMessages;
            while (in.read(buffer, 20)) {
                remainingMessages.emplace_back(buffer);
            }
            in.close();

            // ���������� ����� ��� ������������ ���������
            out.open(filename, ios::binary | ios::trunc);
            for (const auto& msg : remainingMessages) {
                out.write(msg.c_str(), 20);
            }
            out.close();

            ReleaseMutex(hMutex);
            ReleaseSemaphore(hEmptySemaphore, 1, NULL);
        }

        else {
            if (comand == "exit") {
                for (int i = 0; i < kolSenders; i++) {
                    TerminateProcess(pi[i].hProcess, 1);
                    TerminateProcess(pi[i].hThread, 1);
                }
                break;
            }
            else {
                cout << "������������ ��� ��������" << endl;
            }
        }
    }

    // ������� ��������
    for (int i = 0; i < kolSenders; i++) {
        CloseHandle(pi[i].hThread);
        CloseHandle(pi[i].hProcess);
    }

    delete[] readyEvents;
    delete[] si;
    delete[] pi;
    delete[] hProcessors;

    CloseHandle(hMutex);
    CloseHandle(hFullSemaphore);
    CloseHandle(hEmptySemaphore);
    in.close();
    out.close();

    return 0;
}
