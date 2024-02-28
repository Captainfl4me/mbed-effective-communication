/*
 * Implementation of std::chrono::round for C++11 since it is only implemented in C++17
 *
 * Source code: https://howardhinnant.github.io/duration_io/chrono_util.html
 */

#pragma once
#include <chrono>

namespace util_detail {
    template <class T>
    struct choose_trunc_type
    {
        static const int digits = std::numeric_limits<T>::digits;
        using type = typename std::conditional
                    <
                        digits < 32,
                        std::int32_t,
                        typename std::conditional
                        <
                            digits < 64,
                            std::int64_t,
    #ifdef __SIZEOF_INT128__
                            __int128
    #else
                            std::int64_t
    #endif
                        >::type
                    >::type;
    };

    template <class T>
    constexpr
    inline
    typename std::enable_if
    <
        !std::chrono::treat_as_floating_point<T>::value,
        T
    >::type
    trunc(T t) noexcept
    {
        return t;
    }

    template <class T>
    constexpr
    inline
    typename std::enable_if
    <
        std::chrono::treat_as_floating_point<T>::value,
        T
    >::type
    trunc(T t) noexcept
    {
        using namespace std;
        using I = typename choose_trunc_type<T>::type;
        constexpr auto digits = numeric_limits<T>::digits;
        static_assert(digits < numeric_limits<I>::digits, "");
        constexpr auto max = I{1} << (digits-1);
        constexpr auto min = -max;
        const auto negative = t < T{0};
        if (min <= t && t <= max && t != 0 && t == t)
        {
            t = static_cast<T>(static_cast<I>(t));
            if (t == 0 && negative)
                t = -t;
        }
        return t;
    }
}  // detail

// trunc towards zero
template <class To, class Rep, class Period>
constexpr
inline
To
trunc(const std::chrono::duration<Rep, Period>& d)
{
    return To{util_detail::trunc(std::chrono::duration_cast<To>(d).count())};
}

// round down
template <class To, class Rep, class Period>
constexpr
inline
To
floor(const std::chrono::duration<Rep, Period>& d)
{
    auto t = trunc<To>(d);
    if (t > d)
        return t - To{1};
    return t;
}

// round to nearest, to even on tie
template <class To, class Rep, class Period>
constexpr
inline
To
round(const std::chrono::duration<Rep, Period>& d)
{
    auto t0 = floor<To>(d);
    auto t1 = t0 + To{1};
    if (t1 == To{0} && t0 < To{0})
        t1 = -t1;
    auto diff0 = d - t0;
    auto diff1 = t1 - d;
    if (diff0 == diff1)
    {
        if (t0 - trunc<To>(t0/2)*2 == To{0})
            return t0;
        return t1;
    }
    if (diff0 < diff1)
        return t0;
    return t1;
}

// round up
template <class To, class Rep, class Period>
constexpr
inline
To
ceil(const std::chrono::duration<Rep, Period>& d)
{
    auto t = trunc<To>(d);
    if (t < d)
        return t + To{1};
    return t;
}

// trunc towards zero
template <class To, class Clock, class FromDuration>
constexpr
inline
std::chrono::time_point<Clock, To>
trunc(const std::chrono::time_point<Clock, FromDuration>& tp)
{
    using std::chrono::time_point;
    return time_point<Clock, To>{trunc<To>(tp.time_since_epoch())};
}

// round down
template <class To, class Clock, class FromDuration>
constexpr
inline
std::chrono::time_point<Clock, To>
floor(const std::chrono::time_point<Clock, FromDuration>& tp)
{
    using std::chrono::time_point;
    return time_point<Clock, To>{floor<To>(tp.time_since_epoch())};
}

// round to nearest, to even on tie
template <class To, class Clock, class FromDuration>
constexpr
inline
std::chrono::time_point<Clock, To>
round(const std::chrono::time_point<Clock, FromDuration>& tp)
{
    using std::chrono::time_point;
    return time_point<Clock, To>{round<To>(tp.time_since_epoch())};
}

// round up
template <class To, class Clock, class FromDuration>
constexpr
inline
std::chrono::time_point<Clock, To>
ceil(const std::chrono::time_point<Clock, FromDuration>& tp)
{
    using std::chrono::time_point;
    return time_point<Clock, To>{ceil<To>(tp.time_since_epoch())};
}