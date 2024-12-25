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

// Функция генерации случайного числа в диапазоне [a, b)
size_t rnd(size_t a = 0, size_t b = INT32_MAX) {
    static auto now = system_clock::now().time_since_epoch().count(); // Используем текущее время для генератора
    static default_random_engine generator(now); // Генератор случайных чисел
    static uniform_int_distribution<size_t> distribution(0, UINT64_MAX); // Универсальное распределение

    return a + distribution(generator) % (b - a); // Возвращаем случайное число в пределах от a до b
}

// Функция генерации случайной строки длиной symbolCnt
string random_string(size_t symbolCnt) {
    string rndStr(symbolCnt, ' '); // Инициализируем строку заданной длины

    // Заполняем строку случайными символами
    for (auto& symb : rndStr) {
        symb = 'a' + rnd('A', 'Z') % ('z' - 'a'); // Генерация символов от 'a' до 'z'
    }

    return rndStr;
}

// Мьютекс для синхронизации вывода в консоль и файл
mutex output_mutex;
condition_variable cv; // Условная переменная для синхронизации
bool outIsFree = true; // Флаг, указывающий, свободен ли поток для записи в файл
ofstream out("output.txt"); // Открываем файл для записи результатов

// Рабочая функция для потока
void worker(int id, mutex& mtx, int symbolCnt) {
    auto start = high_resolution_clock::now(); // Фиксируем время начала работы потока

    {
        unique_lock<mutex> lock(mtx); // Захватываем мьютекс, чтобы избежать гонки данных

        // Если файл занят (outIsFree == false), ждем его освобождения
        if (not outIsFree) {
            cv.wait(lock, [&] { return outIsFree; });
        }

        // Запись в файл
        outIsFree = false; // Устанавливаем флаг, что файл занят
        out << "поток " << id << ": " << random_string(symbolCnt) << endl;
        outIsFree = true; // Освобождаем файл
        cv.notify_one(); // Уведомляем другие потоки, что файл теперь свободен
    }

    auto finish = high_resolution_clock::now(); // Фиксируем время завершения работы потока
    duration<double> duration = finish - start; // Вычисляем время выполнения потока

    // Вывод времени выполнения потока в консоль
    {
        lock_guard<mutex> lock(output_mutex); // Блокируем вывод в консоль
        cout << "поток " << id << ", время: " << duration.count() << " сек.\n";
    }
}

int main() {
    int symbolCnt, threadsCnt;
    cin >> symbolCnt >> threadsCnt; // Вводим параметры: количество символов в строке и количество потоков
    
    mutex mtx; // Создаём мьютекс для синхронизации

    vector<thread> threads(threadsCnt); // Создаём вектор потоков

    int i = 0;
    for (auto& th : threads) {
        th = thread(worker, i++, ref(mtx), symbolCnt); // Запускаем потоки
    }

    for (auto& th : threads) {
        th.join(); // Ожидаем завершения всех потоков
    }
}