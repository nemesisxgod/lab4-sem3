#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <random>
using namespace std;
using namespace std::chrono;

enum class Type {
    reader,
    writer
};

Type priority = Type::reader; // Приоритет: читатели или писатели

mutex mtx;                   // Мьютекс для синхронизации доступа
condition_variable cv;       // Условная переменная для ожидания потоков

int readersCnt = 0;          // Количество активных читателей
bool writing = false;        // Флаг записи (true, если идет запись)

int writersWait = 0;         // Количество ожидающих писателей
int readersWait = 0;         // Количество ожидающих читателей

int allThreads = 0;          // Общее количество активных потоков

int sharedData = 10;         // Общий ресурс

// Генерация случайного числа в диапазоне [a, b]
size_t rnd(size_t a = 0, size_t b = INT32_MAX) {
    static auto now = system_clock::now().time_since_epoch().count();
    static default_random_engine generator(now);
    static uniform_int_distribution<size_t> distribution(0, UINT64_MAX);

    return a + distribution(generator) % (b - a);
}

// Поток читателя
void reader(int id) {
    {
        unique_lock<mutex> lock(mtx);

        // Ждать, если идет запись
        if (writing) {
            ++readersWait;
            cv.wait(lock, [&] { return !writing; });
            --readersWait;
        }

        // Ждать, если приоритет у писателей
        if (readersCnt == 0 && priority == Type::writer && writersWait != 0) {
            ++readersWait;
            cv.wait(lock, [&] {
                return !writing && writersWait == 0;
            });
            --readersWait;
        }

        // Увеличиваем счетчик активных читателей
        ++readersCnt;
    }

    // Читатель выполняет чтение
    this_thread::sleep_for(std::chrono::milliseconds(rnd(10, 1000)));

    {
        unique_lock<mutex> lock(mtx);
        cout << "Читатель " << id << " прочитал: " << sharedData << endl;

        // Уменьшаем счетчик активных читателей
        --readersCnt;

        // Уведомляем потоки, если больше нет читателей
        if (readersCnt == 0) {
            cv.notify_all();
        }

        --allThreads; // Уменьшаем общее количество потоков
    }
}

// Поток писателя
void writer(int id) {
    {
        unique_lock<mutex> lock(mtx);

        // Ждать, если кто-то пишет или читает
        if (writing || readersCnt != 0 || (priority == Type::reader && readersWait != 0)) {
            ++writersWait;

            cv.wait(lock, [&] {
                if (priority == Type::reader && readersWait != 0) {
                    return false;
                }
                return !writing && readersCnt == 0;
            });

            --writersWait;
        }

        // Начинаем запись
        writing = true;
    }

    // Писатель выполняет запись
    sharedData = rnd();
    cout << "Писатель " << id << " записал: " << sharedData << endl;
    this_thread::sleep_for(std::chrono::milliseconds(rnd(10, 1000)));

    {
        unique_lock<mutex> lock(mtx);

        // Завершаем запись
        writing = false;
        cv.notify_all(); // Уведомляем потоки

        --allThreads; // Уменьшаем общее количество потоков
    }
}

int main() {
    cin >> allThreads; // Ввод общего количества потоков
    vector<int> types(allThreads);

    // Ввод типов потоков (0 — читатель, 1 — писатель)
    for (auto& t : types) {
        cin >> t;
    }

    // Создаем потоки в зависимости от их типа
    for (int i = 0; i < allThreads; ++i) {
        auto type = static_cast<Type>(types[i]);
        thread th((type == Type::writer ? writer : reader), i);
        th.detach(); // Потоки работают в фоне
    }

    // Ожидаем завершения всех потоков
    while (allThreads > 0);
}