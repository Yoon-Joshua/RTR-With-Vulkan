#ifndef TIMER_H
#define TIMER_H

#include <chrono>
using std::chrono::steady_clock;
using std::chrono::nanoseconds;

class Timer {
public:
	Timer();
	void reset();
	void start();
	void stop();
	void tick();

private:
	nanoseconds deltaTime{};
	nanoseconds pausedTime{};
	steady_clock::time_point prevTime;
	steady_clock::time_point baseTime;
	steady_clock::time_point stopTime;
	bool mStopped;
};

#endif
