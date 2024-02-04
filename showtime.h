/* Copyright (c) 2024 Jörgen Grahn
 * All rights reserved.
 *
 * 	Lyrically unorthodox, I flow continuous
 * 	Never on a straight path, I'm known to bend and twist
 * 	Put it down from the suburbs to the tenement
 * 	You betting 'gainst me, but wanna wonder where your money went
 *
 * 	-- Mr. Fantastik & MF DOOM, "Anti-Matter"
 */
#ifndef SHOWTIME_SHOWTIME_H_
#define SHOWTIME_SHOWTIME_H_

#include <chrono>
#include <vector>
#include <map>

namespace showtime {

    class Timer;

    /* f(x) = kx + m
     * A linear function of time.
     */
    template <class Clock>
    class Linear {
    public:
	using time_point = typename Clock::time_point;
	using duration = typename Clock::duration;
	using ddur = std::chrono::duration<double, typename duration::period>;

	Linear() = default;
	Linear(const Linear&, duration dt, double v);

	time_point operator() (time_point x) const {
	    double t = x.time_since_epoch().count();
	    ddur dt(k*t);
	    duration tt = std::chrono::duration_cast<duration>(dt + m);
	    return time_point {tt};
	}

    private:
	double k = 1;
	duration m {0};
    };

    template <class Clock>
    Linear<Clock>::Linear(const Linear<Clock>& other, duration dt, double v)
	: k {v},
	  m {other.m + dt}
    {}

    /**
     * A clock on top of std::chrono::system_clock (the reference
     * clock). By default it follows its reference, but it can change
     * speed, stop, or jump back or forward. At any given time, it's a
     * linear function of the reference clock.
     *
     * Supports timers, expressed in Clock time. Back in the reference
     * world you can wait for the first timer to strike, and if you
     * jump the clock you can find out which timers elapsed during the
     * jump.
     *
     * 	 Most of the interface is independent of the movements
     * 	 of the reference clock, to ease testing.
     *
     * Meets the 'Clock' requirements, although only a fraction of
     * the features belong there:
     * https://en.cppreference.com/w/cpp/named_req/Clock
     */
    class Clock {
    public:
	using ref = std::chrono::system_clock;

	using rep = ref::rep;
	using period = ref::period;
	using duration = ref::duration;
	using time_point = ref::time_point;
	static bool is_steady() { return false; }

	Clock();
	Clock(const Clock&) = delete;
	Clock& operator= (const Clock&) = delete;

	/**
	 * ...
	 */
	struct Ramifications {
	    std::vector<Timer*> elapsed;
	    ref::duration snooze;
	};

	void change(time_point a, time_point b, double v);
	Ramifications set(time_point t);

	time_point at(ref::time_point ref) const;
	time_point now() const;

	void add(time_point t, Timer*);
	void remove(Timer*);

    private:
	Linear<Clock> f;
	std::map<time_point, Timer*> timers;
    };

    class Timer {
    public:
	explicit Timer(Clock::duration dt, bool repeat = false) : dt{dt}, repeat{repeat} {}

	const Clock::duration dt;
	const bool repeat;
	bool cancelled = false;
    };
}
#endif
