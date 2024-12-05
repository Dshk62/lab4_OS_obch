//Sender
#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {

    setlocale(LC_ALL, "rus");

    if (argc < 3) { // Ожидаем два аргумента: имя файла и индекс процесса
        cout << "Недостаточно аргументов. Ожидается: имя файла и индекс процесса." << endl;
        return 1;
    }

    string filename = argv[1];
    int senderIndex = stoi(argv[2]); // Получаем индекс процесса из аргументов

    // Открытие файла для передачи сообщений
    ofstream out(filename, ios::binary | ios::app);
    out.close();

    // Получение синхронизирующих объектов
    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, L"SyncMutex");
    HANDLE hFullSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"FullSemaphore");
    HANDLE hEmptySemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"EmptySemaphore");

    if (!hMutex) {
        cout << "Ошибка открытия синхронизирующих объектов" << endl;
        return GetLastError();
    }

    wstring eventName = L"SenderReady_" + to_wstring(senderIndex);
    HANDLE readyEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, eventName.c_str());
    if (!readyEvent) {
        cout << "Ошибка открытия события готовности: " << GetLastError() << endl;
        return GetLastError();
    }
    SetEvent(readyEvent); // Сигнализируем о готовности
    CloseHandle(readyEvent);
    cout << "Sender готов." << endl;

    // Цикл для отправки сообщений
    ReleaseMutex(hMutex);
    string comand;
    string mess;
    while (true) {
        cout << "Выберите операцию exit или sent: " << endl;
        cin >> comand;

        if (comand == "sent") {
            WaitForSingleObject(hEmptySemaphore, INFINITE);
            WaitForSingleObject(hMutex, INFINITE);

            cout << "Введите сообщение: " << endl;
            cin >> mess;

            // Запись нового сообщения в конец файла
            out.open(filename, ios::binary | ios::app); // Открытие в режиме добавления
            string paddedMessage = mess + string(20 - mess.size(), '\0'); // Сообщение фиксированной длины
            out.write(paddedMessage.c_str(), 20);
            out.close();

            ReleaseMutex(hMutex);
            ReleaseSemaphore(hFullSemaphore, 1, NULL);
        }

        else {
            if (comand == "exit") {
                break;
            }
            else {
                cout << "Некорректное имя операции" << endl;
            }
        }
    }

    CloseHandle(hMutex);
    CloseHandle(hFullSemaphore);
    CloseHandle(hEmptySemaphore);
    return 0;
}
