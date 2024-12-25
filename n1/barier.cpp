#include <iostream>
#include <fstream>
#include <thread>
#include <random>
#include <chrono>
#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
using namespace std;
using namespace chrono;

// Класс Barrier реализует механизм синхронизации потоков
class Barrier {
public:
    // Конструктор принимает количество потоков, которые должны достичь барьера
    Barrier(int count)
        : count(count)
        , waiting(0) // Кол-во ожидающих на барьере
        , barrier_broken(false)
    {}

    // Метод для ожидания на барьере
    void wait() {
        unique_lock<mutex> lock(mtx);

        // Если барьер сломан, пропускаем ожидание
        if (barrier_broken)
            return;

        ++waiting; // Увеличиваем количество ожидающих потоков

        // Если все потоки достигли барьера, пробуждаем их
        if (waiting == count) {
            waiting = 0; // Сбрасываем счётчик ожидания
            cv.notify_all();
        } else {
            // Иначе текущий поток ждёт
            cv.wait(lock);
        }
    }

    // Метод для ручного "разрушения" барьера
    void break_barrier(){
        unique_lock<mutex> lock(mtx);
        barrier_broken = true; // Устанавливаем флаг, что барьер сломан
        cv.notify_all(); // Пробуждаем всех ожидающих
    }

    // Проверка, сломан ли барьер
    bool isBroken(){
        unique_lock<mutex> lock(mtx);
        return barrier_broken;
    }

private:
    int count; // Общее количество потоков, которое нужно для открытия барьера
    int waiting; // Текущее количество потоков, ожидающих на барьере
    mutex mtx; // Мьютекс для синхронизации доступа к данным
    condition_variable cv; // Условная переменная для управления ожиданием
    atomic<bool> barrier_broken; // Флаг, указывающий, сломан ли барьер
};

// Функция генерации случайного числа в диапазоне [a, b)
size_t rnd(size_t a = 0, size_t b = INT32_MAX) {
    static auto now = system_clock::now().time_since_epoch().count();
    static default_random_engine generator(now);
    static uniform_int_distribution<size_t> distribution(0, UINT64_MAX);

    return a + distribution(generator) % (b - a);
}

// Функция генерации случайной строки длиной symbolCnt
string random_string(size_t symbolCnt) {
    string rndStr(symbolCnt, ' '); // Создаём строку заданной длины

    // Заполняем строку случайными символами
    for (auto& symb : rndStr) {
        symb = 'a' + rnd('A', 'Z') % ('z' - 'a');
    }

    return rndStr;
}

// Мьютекс для синхронизации вывода в консоль и файл
mutex output_mutex;
ofstream out("output.txt"); // Файл для записи результатов

// Рабочая функция для потока
void worker(int id, Barrier& barrier, int symbolCnt) {
    auto start = high_resolution_clock::now(); // Фиксируем время начала работы потока

    barrier.wait(); // Ожидание на барьере

    // Генерация случайной строки и запись в файл
    output_mutex.lock();
    out << "поток " << id << ": " << random_string(symbolCnt) << endl;
    output_mutex.unlock();

    auto finish = high_resolution_clock::now(); // Фиксируем время завершения работы потока
    duration<double> duration = finish - start;

    // Вывод времени выполнения потока в консоль
    {
        lock_guard<mutex> lock(output_mutex);
        cout << "поток " << id << ", время: " << duration.count() << " сек.\n";
    }
}

int main() {
    int symbolCnt, threadsCnt;
    cin >> symbolCnt >> threadsCnt;
    
    Barrier barrier (threadsCnt);
    
    vector<thread> threads (threadsCnt);

    int i = 0;
    for (auto& th : threads) {
        th = thread(worker, i++, ref(barrier), symbolCnt);
    }

    for (auto& th : threads) {
        th.join();
    }
}
