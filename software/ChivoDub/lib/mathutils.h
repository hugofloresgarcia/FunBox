#pragma once
#include <cmath>
#include <algorithm>

inline double linlin(double x, double a, double b, double c, double d) {
    if (a == b)
        return c; // avoid division by zero

    // Clamp input
    if ((b > a && x <= a) || (b < a && x >= a))
        return c;
    if ((b > a && x >= b) || (b < a && x <= b))
        return d;

    double t = (x - a) / (b - a);
    return c + t * (d - c);
}

inline double explin(double x, double a, double b, double c, double d) {
    if (x <= 0.0 || a <= 0.0 || b <= 0.0 || a == b)
        return c; // invalid domain

    if ((b > a && x <= a) || (b < a && x >= a))
        return c;
    if ((b > a && x >= b) || (b < a && x <= b))
        return d;

    double t = log(x / a) / log(b / a);
    return c + t * (d - c);
}

inline double expexp(double x, double a, double b, double c, double d) {
    if (x <= 0.0 || a <= 0.0 || b <= 0.0 || c <= 0.0 || d <= 0.0 || a == b)
        return c; // invalid domain

    if ((b > a && x <= a) || (b < a && x >= a))
        return c;
    if ((b > a && x >= b) || (b < a && x <= b))
        return d;

    double t = log(x / a) / log(b / a);
    return c * pow(d / c, t);
}

inline double linexp(double x, double a, double b, double c, double d) {
    if (c <= 0.0 || d <= 0.0 || a == b)
        return c; // invalid output domain or division by zero

    if ((b > a && x <= a) || (b < a && x >= a))
        return c;
    if ((b > a && x >= b) || (b < a && x <= b))
        return d;

    double t = (x - a) / (b - a);
    return c * pow(d / c, t);
}
