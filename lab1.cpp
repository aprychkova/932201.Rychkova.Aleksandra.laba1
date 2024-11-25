#include <iostream>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <memory>
using namespace std;

// Данные события
class EventData {
public:
    int id;
    explicit EventData(int id) : id(id) {}
};

// Монитор
class Monitor {
public:
    mutex mtx;                    // Мьютекс
    condition_variable cv;        // Условная переменная для блокировки потока до наступления определенного события
    bool eventReady = false;      // Флаг готовности события
    bool stopRequested = false;   // Флаг завершения работы
    shared_ptr<EventData> eventData = nullptr;  // Умный указатель на объект

public:

    bool isEventReady() {
        return eventReady;
    }

    // Метод для события (поставщик) 
    void produceEvent(int id) {
        lock_guard<mutex> lock(mtx);  // Блокирование доступа к данным на время работы с ними
        eventData = make_shared<EventData>(id); // Создание нового события
        eventReady = true;  // Устанавливаем флаг готовности события
        cout << "Producer: Event " << id << " produced." << endl;
        cv.notify_one();  // Оповещаем поток-потребитель
    }

    // Метод для обработки события (потребителем)  
    void consumeEvent() {
        unique_lock<mutex> lock(mtx);  // Блокируем доступ
        cv.wait(lock, [this]() { return eventReady || stopRequested; });// Поток-потребитель ожидает, пока событие не будет готово

        if (stopRequested) {
            cout << "Consumer: Stopping, no more events." << endl;
            return;
        }

        cout << "Consumer: Event " << eventData->id << " consumed." << endl;

        eventReady = false;
    }

    void waitForEvent() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() { return eventReady; });
        cout << "Event consumed." << endl;
        eventReady = false;
    }

    // Метод для остановки потока
    void stop() {
        lock_guard<mutex> lock(mtx);
        stopRequested = true;
        cv.notify_all();  // Оповещение всех потоков
    }
};

// Поток-поставщик
void producer(Monitor& monitor, int maxEvents) {
    int eventCounter = 0;
    while (eventCounter < maxEvents) {
        this_thread::sleep_for(chrono::seconds(1)); // Задержка 1 секунда
        monitor.produceEvent(eventCounter++); // Инициирование события
    }
    monitor.stop(); // Когда количество событий достигло максимума, останавливаем потребителя
}

// Поток-потребитель
void consumer(Monitor& monitor, int maxEvents) {
    while (true) {
        monitor.consumeEvent();  // Получение события
        if (monitor.stopRequested) {  // Если был запрашен стоп, завершаем работу
            break;
        }
    }
}

int main() {
    Monitor monitor;

    const int maxEvents = 10;

    thread producerThread(producer, ref(monitor), maxEvents);  // Поток-поставщик
    thread consumerThread(consumer, ref(monitor), maxEvents);  // Поток-потребитель

    producerThread.join();  // Ожидаем завершения потока-поставщика
    consumerThread.join();  // Ожидаем завершения потока-потребителя

    return 0;
}
