#include <iostream>
#include <fstream>
#include <thread>
#include <random>
#include <chrono>
#include <string>
#include <mutex>
#include <atomic>
#include <condition_variable>
using namespace std;
using namespace chrono;

// Класс Spinlock реализует спинлок для синхронизации потоков
class Spinlock {
public:
    // Метод для захвата блокировки
    void lock() {
        bool expected = false;
        // Цикл, который ожидает, пока блокировка не будет освобождена
        while(!_locked.compare_exchange_weak(expected, true, memory_order_acquire)) {
            expected = false; // Перезапускаем проверку, если блокировка была захвачена
        }
    }
 
    // Метод для освобождения блокировки
    void unlock() {
        _locked.store(false, memory_order_release); // Освобождаем блокировку
    }
 
private:
    atomic<bool> _locked{false}; // Переменная для хранения состояния блокировки
};

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
        symb = 'a' + rnd('A', 'Z') % ('z' - 'a'); // Ошибка с диапазоном символов
    }

    return rndStr;
}

// Мьютекс для синхронизации вывода в консоль и файл
mutex output_mutex;
ofstream out("output.txt"); // Открываем файл для записи результатов

// Рабочая функция для потока
void worker(int id, Spinlock& spin, int symbolCnt) {
    auto start = high_resolution_clock::now(); // Фиксируем время начала работы потока

    spin.lock(); // Поток захватывает блокировку
    // Генерация случайной строки и запись в файл
    out << "поток " << id << ": " << random_string(symbolCnt) << endl;
    spin.unlock(); // Освобождает блокировку

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
    
    Spinlock spin; // Создаём объект Spinlock
    
    vector<thread> threads(threadsCnt); // Создаём вектор потоков

    int i = 0;
    for (auto& th : threads) {
        th = thread(worker, i++, ref(spin), symbolCnt); // Запускаем потоки
    }

    for (auto& th : threads) {
        th.join(); // Ожидаем завершения всех потоков
    }
}