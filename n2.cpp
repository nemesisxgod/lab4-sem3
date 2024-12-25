#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <ctime>

struct Date {
    int day;
    int month;
    int year;

    bool operator>=(const Date& other) const {
        if (year != other.year) return year > other.year;
        if (month != other.month) return month > other.month;
        return day >= other.day;
    }

    bool operator<=(const Date& other) const {
        if (year != other.year) return year < other.year;
        if (month != other.month) return month < other.month;
        return day <= other.day;
    }
};

// Генерация случайной даты
Date generateRandomDate() {
    static std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> year_dist(1900, 2100);
    std::uniform_int_distribution<int> month_dist(1, 12);
    std::uniform_int_distribution<int> day_dist(1, 31);

    Date date;
    date.year = year_dist(generator);
    date.month = month_dist(generator);
    date.day = std::min(day_dist(generator), (date.month == 2 ? 28 : 30));
    return date;
}

// Функция поиска дат в диапазоне
void findDatesInRange(const std::vector<Date>& dates, const Date& d1, const Date& d2, std::vector<Date>& result) {
    for (const auto& date : dates) {
        if (date >= d1 && date <= d2) {
            result.push_back(date);
        }
    }
}

void findDatesInRangeParallel(const std::vector<Date>& dates, const Date& d1, const Date& d2, std::vector<Date>& result, int num_threads) {
    std::vector<std::thread> threads;
    std::vector<std::vector<Date>> partial_results(num_threads);
    size_t chunk_size = dates.size() / num_threads;

    for (int i = 0; i < num_threads; ++i) {
        size_t start_idx = i * chunk_size;
        size_t end_idx = (i == num_threads - 1) ? dates.size() : (i + 1) * chunk_size;
        threads.emplace_back([&dates, d1, d2, start_idx, end_idx, &partial_results, i]() {
            for (size_t j = start_idx; j < end_idx; ++j) {
                if (dates[j] >= d1 && dates[j] <= d2) {
                    partial_results[i].push_back(dates[j]);
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    result.reserve(dates.size()); // Предварительное резервирование памяти
    for (const auto& part : partial_results) {
        result.insert(result.end(), part.begin(), part.end());
    }
}

int main() {
    // Параметры
    size_t data_size = 10000000; // Размер массива данных
    int num_threads = 500;       // Количество потоков
    Date d1 = {1, 1, 2000};    // Начало диапазона
    Date d2 = {31, 12, 2020};  // Конец диапазона

    // Генерация массива дат
    std::vector<Date> dates(data_size);
    for (auto& date : dates) {
        date = generateRandomDate();
    }

    // Однопоточная обработка
    auto start_single = std::chrono::high_resolution_clock::now();
    std::vector<Date> result_single;
    findDatesInRange(dates, d1, d2, result_single);
    auto end_single = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_single = end_single - start_single;

    // Многопоточная обработка
    auto start_multi = std::chrono::high_resolution_clock::now();
    std::vector<Date> result_multi;
    findDatesInRangeParallel(dates, d1, d2, result_multi, num_threads);
    auto end_multi = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_multi = end_multi - start_multi;

    // Вывод результатов
    std::cout << "Однопоточная обработка: " << elapsed_single.count() << " секунд, найдено " << result_single.size() << " дат.\n";
    std::cout << "Многопоточная обработка: " << elapsed_multi.count() << " секунд, найдено " << result_multi.size() << " дат.\n";

    return 0;
}