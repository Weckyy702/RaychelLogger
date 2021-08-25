#include "Logger.h"

using namespace Logger;

struct Streamable
{};

std::ostream& operator<<(std::ostream& os, const Streamable& /*unused*/)
{
    return os << "Streamable";
}

struct NonStreamable
{};

int main(int /*unused*/, const char** /*unused*/)
{
    setMinimumLogLevel(LogLevel::debug);

    debug("Debug level\n");
    info("Info level\n");
    warn("Warn level\n");
    error("Error level\n");
    critical("Critical level\n");
    fatal("fatal level\n");

    Streamable s;
    debug(s, '\n');

    info(NonStreamable{}, '\n');
    critical(Streamable{}, '\n');

    const NonStreamable c_n;
    volatile NonStreamable v_n;
    const volatile NonStreamable cv_n;

    const volatile NonStreamable& ref_cv_n = cv_n;

    info(c_n, "\n");
    info(v_n, "\n");
    info(cv_n, '\n');
    info(ref_cv_n, '\n');

    char bad_style_string[] = "char[]";
    const char* worse_style_string = "const char*";

    info(bad_style_string, '\n');
    info(worse_style_string, '\n');

    return 0;
}