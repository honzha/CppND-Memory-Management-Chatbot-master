#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.

    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mtx);

    _condition.wait(uLock, [this] { return !_que.empty();});

    T message = std::move(_que.front());
    _que.pop_front();

    return message;

}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    /* Prevent data race */
    std::lock_guard<std::mutex> uLock(_mtx);

    // add message to queue
    _que.push_back(std::move(msg));
    _condition.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _trcurrentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto message = _queue_traffic.receive();
        if (message == TrafficLightPhase::green){

            return;

        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _trcurrentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    std::mt19937_64 eng{std::random_device{}()};
    std::uniform_int_distribution<> dis{4000, 6000};

    int duration = dis(eng);

    // begin time measurement
    auto timeLastChanged = std::chrono::system_clock::now();

    while(true){
        // wait 1ms between two cycles
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // calculate time measure
        auto timeUpdateNow = std::chrono::system_clock::now();
        auto timeStatus = std::chrono::duration_cast<std::chrono::milliseconds>( timeUpdateNow - timeLastChanged ).count();
        if (timeStatus > duration){
            if (_trcurrentPhase == TrafficLightPhase::red){
                _trcurrentPhase = TrafficLightPhase::green;
            } else {
                _trcurrentPhase = TrafficLightPhase::red;
            }

            //update TrafficLightPhase to queue
            auto message_que_sent = std::async(std::launch::async,
                    &MessageQueue<TrafficLightPhase>::send,
                    &_queue_traffic, std::move(_trcurrentPhase));

            message_que_sent.wait();

            duration = dis(eng);
            timeLastChanged = std::chrono::system_clock::now();
        }

    }
}