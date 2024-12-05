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

    // 1. Ввести с консоли имя бинарного файла и количество записей в бинарном файле
    cout << "Введите имя файла: " << endl;
    wcin >> filename;

    cout << "Введите количество записей: " << endl;
    wcin >> kol_sent;
    bool flag = true;
    if (kol_sent <= 0) {
        while (flag) {
            cout << "Некорректное количество записей, введите число >=1" << endl;
            wcin >> kol_sent;
            if (kol_sent > 0) flag = false;
        }
    }

    // Создаём бинарный файл
    ofstream out(filename, ios::binary | ios::app);
    out.close();
    ifstream in(filename, ios::binary);
    in.close();

    // Создание синхронизирующих объектов
    HANDLE hMutex;
    HANDLE hFullSemaphore, hEmptySemaphore;
    HANDLE* readyEvents = nullptr;

    hMutex = CreateMutex(NULL, FALSE, L"SyncMutex");
    hFullSemaphore = CreateSemaphore(NULL, 0, kol_sent, L"FullSemaphore");
    hEmptySemaphore = CreateSemaphore(NULL, kol_sent, kol_sent, L"EmptySemaphore");

    if (!hMutex) {
        cout << "Ошибка создания мьютекса" << endl;
        return GetLastError();
    }
    if (!hFullSemaphore || !hEmptySemaphore) {
        cout << "Ошибка создания семафоров" << endl;
        return GetLastError();
    }

    WaitForSingleObject(hMutex, INFINITE);

    // Ввод количества процессов Sender
    cout << "Введите количество процессов Sender: " << endl;
    cin >> kolSenders;
    flag = true;

    if (kolSenders <= 0) {
        while (flag) {
            cout << "Некорректное количество процессов Sender, введите число >=1" << endl;
            cin >> kolSenders;
            if (kolSenders > 0) flag = false;
        }
    }

    readyEvents = new HANDLE[kolSenders];

    // Создание событий для каждого процесса Sender
    for (int i = 0; i < kolSenders; i++) {
        wstring eventName = L"SenderReady_" + to_wstring(i);
        readyEvents[i] = CreateEvent(NULL, TRUE, FALSE, eventName.c_str());
        if (!readyEvents[i]) {
            cout << "Ошибка создания события " << i << ": " << GetLastError() << endl;
            return GetLastError();
        }
    }

    STARTUPINFO* si = new STARTUPINFO[kolSenders];
    PROCESS_INFORMATION* pi = new PROCESS_INFORMATION[kolSenders];
    HANDLE* hProcessors = new HANDLE[kolSenders];
    ZeroMemory(si, sizeof(STARTUPINFO) * kolSenders);

    wstring s = L"Sender.exe " + filename;
    lstrcpyW(fileName, s.data());

    // Запуск процессов Sender
    for (int i = 0; i < kolSenders; i++) {
        si[i].cb = sizeof(STARTUPINFO);
        wstring command = L"Sender.exe " + filename + L" " + to_wstring(i); // Передаем индекс
        lstrcpyW(fileName, command.c_str());

        if (!CreateProcess(NULL, fileName, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si[i], &pi[i])) {
            cout << "Ошибка создания процесса " << i << endl;
            return GetLastError();
        }
        hProcessors[i] = pi[i].hProcess;
    }

    // Ожидаем сигнала готовности от всех процессов Sender
    WaitForMultipleObjects(kolSenders, readyEvents, TRUE, INFINITE);
    cout << "Все Sender процессы готовы к работе." << endl;

    // Цикл чтения сообщений
    ReleaseMutex(hMutex);
    string comand;
    
    char* messeng = new char[20];
    string del;
    while (true) {
        cout << "Выберите операцию exit или read: " << endl;
        cin >> comand;

        if (comand == "read") {
            WaitForSingleObject(hFullSemaphore, INFINITE);
            WaitForSingleObject(hMutex, INFINITE);

            // Чтение первого сообщения
            in.open(filename, ios::binary);
            char buffer[20] = {};
            in.read(buffer, 20);

            if (in.gcount() == 0) {
                cout << "Файл пуст, сообщения отсутствуют." << endl;
                in.close();
                ReleaseMutex(hMutex);
                ReleaseSemaphore(hEmptySemaphore, 1, NULL);
                continue;
            }

            string message(buffer);
            cout << "Прочитано сообщение: " << message << endl;

            // Сохранение оставшихся сообщений
            vector<string> remainingMessages;
            while (in.read(buffer, 20)) {
                remainingMessages.emplace_back(buffer);
            }
            in.close();

            // Перезапись файла без прочитанного сообщения
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
                cout << "Некорректное имя операции" << endl;
            }
        }
    }

    // Очистка ресурсов
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
