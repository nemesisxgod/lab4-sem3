#include <iostream>
#include <fstream>
#include <thread>
#include <random>
#include <chrono>
#include <string>
#include <mutex>
#include <condition_variable>
using namespace std;
using namespace chrono;



// Функция для генерации случайного числа в диапазоне [a, b)
size_t rnd(size_t a = 0, size_t b = INT32_MAX) {
    static auto now = system_clock::now().time_since_epoch().count(); // Используем текущее время для генератора
    static default_random_engine generator(now); // Генератор случайных чисел
    static uniform_int_distribution<size_t> distribution(0, UINT64_MAX); // Универсальное распределение для генерации чисел

    return a + distribution(generator) % (b - a); // Возвращаем случайное число в диапазоне от a до b
}

// Функция для генерации случайной строки длиной symbolCnt
string random_string(size_t symbolCnt) {
    string rndStr(symbolCnt, ' '); // Инициализируем строку заданной длины

    // Заполняем строку случайными символами
    for (auto& symb : rndStr) {
        symb = 'a' + rnd('A', 'Z') % ('z' - 'a'); // Ошибка с диапазоном символов
    }

    return rndStr;
}

// Мьютекс для синхронизации вывода в консоль и файл
mutex output_mutex;
ofstream out("output.txt"); // Открываем файл для записи результатов

// Рабочая функция для потока
void worker(int id, mutex& mtx, int symbolCnt) {
    auto start = high_resolution_clock::now(); // Фиксируем время начала работы потока

    mtx.lock(); // Блокируем ресурс (в данном случае файл для записи)
    // Генерация случайной строки и запись в файл
    out << "поток " << id << ": " << random_string(symbolCnt) << endl;
    mtx.unlock(); // Освобождаем ресурс

    auto finish = high_resolution_clock::now(); // Фиксируем время завершения работы потока
    duration<double> duration = finish - start;

    // Вывод времени выполнения потока в консоль
    {
        lock_guard<mutex> lock(output_mutex); // Блокируем вывод в консоль
        cout << "поток " << id << ", время: " << duration.count() << " сек.\n";
    }
}

int main() {
    int symbolCnt, threadsCnt;
    cin >> symbolCnt >> threadsCnt; // Вводим параметры: количество символов в строке и количество потоков
    
    mutex mtx; // Создаём мьютекс для синхронизации доступа к файлу
    
    vector<thread> threads(threadsCnt); // Создаём вектор потоков

    int i = 0;
    for (auto& th : threads) {
        th = thread(worker, i++, ref(mtx), symbolCnt); // Запускаем потоки
    }

    for (auto& th : threads) {
        th.join(); // Ожидаем завершения всех потоков
    }
}