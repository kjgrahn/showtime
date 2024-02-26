#include <showtime.h>

#include <orchis.h>

/* For testing (and reasoning about showtime::Clock in general) let's
 * imagine a Sunday morning, from 10:00 onwards.
 *
 * Let's also have a static mix of timers, so only how we jump the
 * clock varies.
 *
 * 10:00      10        20        30        40        50       11:00
 *   o----o----o----o----o----o----o----o----o----o----o----o----o
 *        A         A         A         A         A         A         A
 *                  B              C              D
 *   o----o----o----o----o----o----o----o----o----o----o----o----o
 *
 *
 *
 */

namespace {

    using std::chrono::minutes;
    using std::chrono::seconds;

    class Day {
    public:
	explicit Day(const char* s);
	showtime::Clock::time_point at(const char* s) const;
	struct tm ymd;
    };

    Day::Day(const char* s)
    {
	strptime(s, "%Y-%m-%d", &ymd);
    }

    showtime::Clock::time_point Day::at(const char* s) const
    {
	struct tm hhmm;
	strptime(s, "%R", &hhmm);
	auto tm = ymd;
	tm.tm_hour = hhmm.tm_hour;
	tm.tm_min = hhmm.tm_min;
	const time_t t = mktime(&tm);
	return showtime::Clock::ref::from_time_t(t);
    }

    const Day sun {"2024-02-11"};

    struct Mix {
	showtime::Timer A {minutes{10}, true};
	showtime::Timer B {minutes{15}};
	showtime::Timer C {minutes{30}};
	showtime::Timer D {minutes{45}};
    };

    void prepare(showtime::Clock& clock, Mix& mix)
    {
	clock.add(sun.at("10:00"), &mix.B);
	clock.add(sun.at("10:00"), &mix.C);
	clock.add(sun.at("10:00"), &mix.D);
	clock.add(sun.at("09:55"), &mix.A);
    }

    void assert_eq(const Mix& timers, const showtime::Clock::Ramifications& res,
		   const char* elapsed, showtime::Clock::duration snooze)
    {
	const std::map<const showtime::Timer*,
		       char> names = {{&timers.A, 'A'},
				      {&timers.B, 'B'},
				      {&timers.C, 'C'},
				      {&timers.D, 'D'}};
	std::string s;
	for (const auto& tm : res.elapsed) { if (!tm->cancelled) s.push_back(names.at(tm)); }
	orchis::assert_eq(s, elapsed);
	orchis::assert_eq(res.snooze.count(), snooze.count());
    }
}

namespace showtime {

    using orchis::TC;
    using orchis::assert_true;
    using orchis::assert_gt;

    void empty(TC)
    {
	Clock clock;
	const auto res = clock.set(sun.at("09:00"));
	orchis::assert_eq(res.elapsed.size(), 0);
	assert_true(res.snooze > seconds{0});
    }

    void walkthrough(TC)
    {
	Clock clock;
	Mix timers;
	prepare(clock, timers);

	auto assert_res = [&timers] (const Clock::Ramifications& res,
				     const char* elapsed, Clock::duration snooze) {
	    assert_eq(timers, res, elapsed, snooze);
	};

	auto res = clock.set(sun.at("10:14"));
	assert_res(res, "A", minutes{1});

	res = clock.set(sun.at("10:20"));
	assert_res(res, "BA", minutes{5});

	res = clock.set(sun.at("10:24"));
	assert_res(res, "", minutes{1});

	res = clock.set(sun.at("10:29"));
	assert_res(res, "A", minutes{1});

	res = clock.set(sun.at("10:34"));
	assert_res(res, "C", minutes{1});

	res = clock.set(sun.at("10:50"));
	assert_res(res, "ADA", minutes{5});

	res = clock.set(sun.at("10:54"));
	assert_res(res, "", minutes{1});
    }

    void longjump(TC)
    {
	Clock clock;
	Mix timers;
	prepare(clock, timers);

	auto assert_res = [&timers] (const Clock::Ramifications& res,
				     const char* elapsed, Clock::duration snooze) {
	    assert_eq(timers, res, elapsed, snooze);
	};

	auto res = clock.set(sun.at("10:10"));
	assert_res(res, "A", minutes{5});

	res = clock.set(sun.at("10:40"));
	assert_res(res, "BAACA", minutes{5});
    }

    void cancel(TC)
    {
	Clock clock;
	Mix timers;
	prepare(clock, timers);

	auto assert_res = [&timers] (const Clock::Ramifications& res,
				     const char* elapsed, Clock::duration snooze) {
	    assert_eq(timers, res, elapsed, snooze);
	};

	auto res = clock.set(sun.at("10:20"));
	assert_res(res, "ABA", minutes{5});

	timers.A.cancelled = true;

	res = clock.set(sun.at("10:20"));
	assert_res(res, "", minutes{10});

	res = clock.set(sun.at("10:35"));
	assert_res(res, "C", minutes{10});
    }

    void jump_back(TC)
    {
	Clock clock;
	Mix timers;
	prepare(clock, timers);

	auto assert_res = [&timers] (const Clock::Ramifications& res,
				     const char* elapsed, Clock::duration snooze) {
	    assert_eq(timers, res, elapsed, snooze);
	};

	auto res = clock.set(sun.at("10:30"));
	assert_res(res, "ABAAC", minutes{5});

	res = clock.set(sun.at("10:00"));
	assert_res(res, "", minutes{35});

	res = clock.set(sun.at("10:40"));
	assert_res(res, "A", minutes{5});
    }

    void speed(TC)
    {
	Clock clock;
	Timer A {minutes{0}};
	clock.add(sun.at("10:30"), &A);

	auto assert_res = [] (const Clock::Ramifications& res,
			      const Clock::duration snooze) {
	    orchis::assert_eq(res.elapsed.size(), 0);
	    orchis::assert_eq(res.snooze.count(), snooze.count());
	};

	auto res = clock.set(sun.at("10:00"));
	assert_res(res, minutes{30});

	res = clock.set(sun.at("10:15"));
	assert_res(res, minutes{15});

	clock.change(sun.at("10:00"), sun.at("10:00"), 2);
	res = clock.set(sun.at("10:00"));
	assert_res(res, minutes{30});

	clock.change(sun.at("10:00"), sun.at("10:00"), 0.5);
	res = clock.set(sun.at("10:00"));
	assert_res(res, minutes{60});
    }

    namespace norepeat {

	void simple(TC)
	{
	    Clock clock;
	    Mix timers;
	    prepare(clock, timers);
	    timers.A.cancelled = true;

	    auto assert_res = [&timers] (const Clock::Ramifications& res,
					 const char* elapsed, Clock::duration snooze) {
		assert_eq(timers, res, elapsed, snooze);
	    };

	    auto res = clock.set(sun.at("10:00"));
	    assert_res(res, "", minutes{15});

	    res = clock.set(sun.at("10:20"));
	    assert_res(res, "B", minutes{10});
	}

	void walkthrough(TC)
	{
	    Clock clock;
	    Mix timers;
	    prepare(clock, timers);
	    timers.A.cancelled = true;

	    auto assert_res = [&timers] (const Clock::Ramifications& res,
					 const char* elapsed, Clock::duration snooze) {
		assert_eq(timers, res, elapsed, snooze);
	    };

	    auto res = clock.set(sun.at("10:14"));
	    assert_res(res, "", minutes{1});

	    res = clock.set(sun.at("10:20"));
	    assert_res(res, "B", minutes{10});

	    res = clock.set(sun.at("10:24"));
	    assert_res(res, "", minutes{6});

	    res = clock.set(sun.at("10:29"));
	    assert_res(res, "", minutes{1});

	    res = clock.set(sun.at("10:34"));
	    assert_res(res, "C", minutes{11});
	}
    }
}
