// pr_11.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>
#include <cstdlib>
using namespace std;
CRITICAL_SECTION print_cs;

int test_program[100];
int test_count = 0;
struct Robot {
    int id;
    int energy;
    int score;
    volatile bool is_paused;
    volatile bool is_finished;
    Robot(int r_id) {
        id = r_id;
        energy = 60;
        score = 0;
        is_paused = false;
        is_finished = false;
    }
};

bool check_energy(int code, int current_energy) {
    int cost = 0;
    if (code == 1) {
        cost = 10;
    }
    else if (code == 2) {
        cost = 20;
    }
    else if (code == 4) {
        cost = 60;
    }

    if (code == 3) {
        return true;
    }
    return current_energy >= cost;
}

void do_test(Robot* r, int code) {
    string name = "";
    int duration = 0;
    if (code == 1){ 
        name = "Препятствие"; duration = 3;
    }
    else if (code == 2) {
        name = "Спринт"; duration = 4;
    }
    else if (code == 3) {
        name = "Подзарядка"; duration = rand() % 5 + 1;
    }
    else if (code == 4) {
        name = "Марафон"; duration = 10;
    }

    if (!check_energy(code, r->energy)) {
        EnterCriticalSection(&print_cs);
        cout << "Робот " << r->id << " пропуск: " << name
            << " (мало энергии: " << r->energy << ")" << endl;
        LeaveCriticalSection(&print_cs);
        Sleep(duration * 1000);
        return;
    }

    {
        EnterCriticalSection(&print_cs);
        cout << "[Робот " << r->id << "] Начал: " << name
            << " (" << duration << " сек)..." << endl;
        LeaveCriticalSection(&print_cs);
    }

    Sleep(duration * 1000);

    if (code == 1) {
        r->energy -= 10; r->score += 15;
    }
    else if (code == 2) {
        r->energy -= 20; r->score += 30;
    }
    else if (code == 3) {
        r->energy += duration * 10;
        if (r->energy > 100) r->energy = 100;
    }
    else if (code == 4) {
        r->energy -= 60; r->score += 55;
    }

    {
        EnterCriticalSection(&print_cs);
        cout << "[Робот " << r->id << " Закончил: " << name
            << ". Энергия: " << r->energy << ", Очки: " << r->score << endl;
        LeaveCriticalSection(&print_cs);
    }
}

DWORD WINAPI robot_run(LPVOID param) {
    Robot* r = (Robot*)param;
    for (int i = 0; i < test_count; i++) {
        while (r->is_paused) {
            Sleep(100);
        }
        if (r->is_finished) {
            break;
        }
        do_test(r, test_program[i]);
        int delay = rand() % 3 + 1;
        Sleep(delay * 1000);
    }

    EnterCriticalSection(&print_cs);
    r->is_finished = true;
    cout << "робот " << r->id << " выиграл и итог: "
        << r->score << " очков." << endl;
    LeaveCriticalSection(&print_cs);
    return 0;
}

void pause_robot(Robot* r) {
    r->is_paused = true;
    EnterCriticalSection(&print_cs);
    cout << "Робот " << r->id << " остановлен" << endl;
    LeaveCriticalSection(&print_cs);
}

void resume_robot(Robot* r) {
    r->is_paused = false;
    EnterCriticalSection(&print_cs);
    cout << "Робот " << r->id << " запущен" << endl;
    LeaveCriticalSection(&print_cs);
}

void make_file() {
    ifstream f("arena.dat", ios::binary);
    if (!f) {
        ofstream out("arena.dat", ios::binary);
        int tests[] = { 1, 3, 2, 3, 4, 1 };
        for (int i = 0; i < 6; i++)
            out.write((char*)&tests[i], sizeof(int));
        out.close();
        cout << "Создан файл" << endl;
    }
    f.close();
}

void load_program() {
    ifstream f("arena.dat", ios::binary);
    int code;
    test_count = 0;
    while (f.read((char*)&code, sizeof(int)) && test_count < 100) {
        test_program[test_count++] = code;
    }
    f.close();
}
int main()
{
    setlocale(LC_ALL, "Russian");
    srand(time(0));
    InitializeCriticalSection(&print_cs);
    make_file();
    load_program();
    cout << "Загружено испытаний: " << test_count << endl;
    int count;
    cout << "Сколько роботов запустить? (3-10): ";
    cin >> count;
    if (count < 3) {
        count = 3;
    }
    if (count > 10) {
        count = 10;
    }
    Robot* robots[10];
    HANDLE threads[10];
    for (int i = 0; i < count; i++)
        robots[i] = new Robot(i + 1);

    cout << "Погнали вась" << endl;
    for (int i = 0; i < count; i++) {
        threads[i] = CreateThread(NULL, 0, robot_run, robots[i], 0, NULL);
    }
    int choice = 0;
    while (choice != 3) {
        cout << "\n1 - Пауза";
        cout << "\n2 - Продолжить";
        cout << "\n3 - Ждать финиша";
        cout << "\n Выбор: ";
        cin >> choice;
        if (choice == 1 || choice == 2) {
            int id;
            cout << "ID робота: ";
            cin >> id;
            if (id >= 1 && id <= count) {
                if (choice == 1) pause_robot(robots[id - 1]);
                else resume_robot(robots[id - 1]);
            }
            else {
                cout << "Нет такого робота!" << endl;
            }
        }
    }
    cout << "Конец вась.." << endl;
    WaitForMultipleObjects(count, threads, TRUE, INFINITE);
    cout << "\nРезульты вась" << endl;
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (robots[j]->score < robots[j + 1]->score) {
                Robot* t = robots[j];
                robots[j] = robots[j + 1];
                robots[j + 1] = t;
            }
        }
    }
    int max_score = robots[0]->score;
    int winners = 0;
    for (int i = 0; i < count; i++) {
        cout << "Робот \n" << robots[i]->id;
        cout << " Очки: \n" << robots[i]->score;
        cout << "Энергия: \n" << robots[i]->energy << endl;
        if (robots[i]->score == max_score) {
            winners++;
        }
    }
    if (winners > 1) {
        cout << "\nпоровну вась! Несколька роботов набрали " << max_score << " очков." << endl;
    }
    else {
        cout << "\nВыиграл вась: робот " << robots[0]->id << " (" << max_score << " очков)" << endl;
    }
    for (int i = 0; i < count; i++) {
        CloseHandle(threads[i]);
        delete robots[i];
    }
    DeleteCriticalSection(&print_cs);
    return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
