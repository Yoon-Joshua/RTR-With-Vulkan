#include "timer.h"

#include <iostream>
#include <Windows.h>

Timer::Timer() : mStopped(false) {}	

void Timer::reset() {
	auto currentTime = steady_clock::now();
	baseTime = currentTime;
	prevTime = currentTime;
	stopTime = {};
	mStopped = false;
}

void Timer::start() {
	auto currentTime = steady_clock::now();
	// Accumulate the time elapsed between stop and	start pairs.
	//
	// |<每每-d每每->|
	// 每每每每每*每每每每每〞*每每每每> time
	// mStopTime startTime
	// If we are resuming the timer from a stopped state＃
	if (mStopped)
	{
		// then accumulate the paused time.
		pausedTime += (currentTime - stopTime);
		// since we are starting the timer back up, the	current
		// previous time is not valid, as it occurred while paused.
		// So reset it to the current time.
		prevTime = currentTime;
		// no longer stopped＃
		stopTime = {};
		mStopped = false;
	}
}

void Timer::stop() {
	// If we are already stopped, then don＊t do anything.
	if (!mStopped)
	{
		steady_clock::time_point currentTime = steady_clock::now();
		// Otherwise, save the time we stopped at, and set
		// the Boolean flag indicating the timer is	stopped.
		stopTime = currentTime;
		mStopped = true;
	}
}

void Timer::tick() {
	if (mStopped) {
		deltaTime = nanoseconds::zero();
		return;
	}
	// Get the time this frame.
	auto currentTime = steady_clock::now();
	// Time difference between this frame and the previous.
	deltaTime = currentTime - prevTime;

	// Prepare for next frame. 
	prevTime = currentTime;

	// Force nonnegative. The DXSDK＊s CDXUTTimer mentions that if the
	// processor goes into a power save mode or we get shuffled to
	// another processor, then mDeltaTime can be negative.
	if (deltaTime.count() < 0.0) {
		deltaTime = nanoseconds::zero();
	}
}