#include "showtime.h"

using showtime::Clock;
using showtime::Timer;

/**
 * The clock by default follows the reference clock.
 */
Clock::Clock() = default;

/**
 * Change the clock so that what is time 'a' now becomes time 'b', and
 * change its speed to 'v' (0 for a stopped clock, 1 for normal speed
 * and so on.)
 *
 * Note that the a->b change is relative to the current clock, not to
 * the reference clock. 'a' is a showtime. The speed, however, is
 * absolute: saying 2 twice doesn't make the speed 4 times the
 * reference.
 *
 * Also note that neither a nor b has to be (in some sense) current
 * time. They're only a way to form a duration, and "current time" has
 * no meaning in this part of the interface.
 */
void Clock::change(time_point a, time_point b, double v)
{
    f = {f, b-a, v};
}

namespace {
    /* From a range in a map<t, Timer*>, return the timers in order.
     * Skips cancelled timers (although this is probably unnecessary).
     * Bug: should expand repeating timers.
     */
    template <class It>
    std::vector<Timer*> elapsed(It a, It b)
    {
	std::vector<Timer*> acc;
	while (a!=b) {
	    Timer* const tm = a->second;
	    if (!tm->cancelled) acc.push_back(tm);
	    a++;
	}
	return acc;
    }
}

/**
 * Move the clock to 't'. Oddly, this function, too, doesn't really
 * change time. It consumes and returns timers which elapsed before t,
 * schedules repeating timers, and tells the caller how long to wait
 * (in reference time) until the next timer ought to strike.
 *
 * - The elapsed set is sorted by time.
 * - Cancelled timers are absent (but more might be cancelled as a side
 *   effect of processing the set)
 * - Removed timers are (of course) absent.
 * - Repeating timers may be present multiple times. A once-a-day timer
 *   will be listed ~365 times if time moves forward a year.
 * - Moving backwards doesn't make any timers elapse. There are no timers
 *   in the past, since timers are consumed by moving forward past them.
 *   There's not even any repeating timers.
 */
Clock::Ramifications Clock::set(time_point t)
{
    auto it = timers.upper_bound(t);
    while (it != end(timers) && it->second->cancelled) it++;

    Clock::duration snooze = std::chrono::hours{1};
    if (it != end(timers)) snooze = it->first - t;

    Ramifications r { elapsed(begin(timers), it), f(snooze)};
    timers.erase(begin(timers), it);
    return r;
}

/**
 * Translating reference time to clock time.
 */
Clock::time_point Clock::at(Clock::ref::time_point ref) const
{
    return f(ref);
}

/**
 * Assuming time is now t, add a timer. Timers are expressed in terms
 * of a duration, so a 30 min timer added at 10:00 will elapse at
 * 10:30. Timers can also repeat; if that timer did, it would elapse
 * at 10:30, 11:00, 11:30 and so on, unless cancelled.
 *
 * The Clock doesn't own these Timers. They can be added multiple
 * times; if you see a reason to do so, please do.
 *
 * Returns a "snooze time" just like set(t) does, since the new timer
 * may elapse before any already scheduled.
 */
Clock::ref::duration Clock::add(Clock::time_point t, Timer* tm)
{
    t += tm->dt;
    // if t is occupied already, place the timer a tiny bit later
    while (timers.count(t)) t += duration{1};
    timers.emplace(t, tm);

    const auto head = *timers.begin();
    return f(head.first - t);
}

/**
 * Remove a timer (from all its occurences on the schedule). Not quite
 * the same thing as cancelling it.
 */
void Clock::remove(Timer* tm)
{
    for (auto& kv : timers) {
	if (kv.second==tm) kv.second = nullptr;
    }
}
