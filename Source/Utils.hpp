#pragma once

#include <chrono>
#include <string>

#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Containers/Array.h>

struct Timer {
    Timer(const char* message = "");
    ~Timer();
    const double duration();

private:
    using Time = std::chrono::time_point<std::chrono::steady_clock>;

    const char* message;
    Time start, end;
};

Timer::Timer(const char* message) : message(message) {
    start = std::chrono::high_resolution_clock::now();
}

Timer::~Timer() {
    if (*message != 0) {
        Debug() << Corrade::Utility::formatString(message, this->duration());
    }
}

const double Timer::duration() {
    end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    return duration.count() * 1'000.0f;  // ms
}
