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

// Класс Monitor реализует механизм синхронизации для управления доступом к ресурсу
class Monitor {
public:
    // Метод блокировки ресурса
    void lock() {
        unique_lock<mutex> lk(mtx); // Блокируем мьютекс
        // Ожидаем, пока ресурс не будет освобождён (is_locked == false)
        cv.wait(lk, [this]() { return !is_locked; });
        is_locked = true; // Устанавливаем флаг блокировки
    }

    // Метод разблокировки ресурса
    void unlock() {
        {
            lock_guard<mutex> lk(mtx); // Защищаем изменение состояния
            is_locked = false; // Освобождаем ресурс
        }
        cv.notify_one(); // Пробуждаем один из ожидающих потоков
    }

private:
    mutex mtx; // Мьютекс для синхронизации доступа
    condition_variable cv; // Условная переменная для ожидания
    bool is_locked = false; // Флаг, указывающий, заблокирован ли ресурс
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
    string rndStr(symbolCnt, ' '); // Инициализируем строку

    // Заполняем строку случайными символами
    for (auto& symb : rndStr) {
        symb = 'a' + rnd('A', 'Z') % ('z' - 'a'); // Генерация случайных символов (ошибка с диапазоном символов)
    }

    return rndStr;
}

// Мьютекс для синхронизации вывода в консоль и файл
mutex output_mutex;
ofstream out("output.txt"); // Открываем файл для записи результатов

// Рабочая функция для потока
void worker(int id, Monitor& monitor, int symbolCnt) {
    auto start = high_resolution_clock::now(); // Фиксируем время начала работы потока

    monitor.lock(); // Блокируем ресурс для текущего потока
    // Генерация случайной строки и запись в файл
    out << "поток " << id << ": " << random_string(symbolCnt) << endl;
    monitor.unlock(); // Освобождаем ресурс

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
    
    Monitor monitor; // Создаём объект для синхронизации потоков
    
    vector<thread> threads(threadsCnt); // Создаём вектор потоков

    int i = 0;
    for (auto& th : threads) {
        th = thread(worker, i++, ref(monitor), symbolCnt); // Запускаем потоки
    }

    for (auto& th : threads) {
        th.join(); // Ожидаем завершения всех потоков
    }
}