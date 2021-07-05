#include "cmdline.h"

namespace celestia
{
namespace util
{
CmdLineParser::Option::Option(const char *longOption,
                        char shortOption,
                        bool hasValue,
                        std::function<bool(const char*)> handler) :
    hasValue(hasValue),
    handler1(handler)
{
    this->longOption = fmt::format("--{}", longOption);
    this->shortOption = fmt::format("-{}", shortOption);
}

CmdLineParser::Option::Option(const char *longOption,
                        char shortOption,
                        bool hasValue,
                        std::function<void(bool)> handler) :
    hasValue(hasValue),
    handler0(handler)
{
    this->longOption = fmt::format("--{}", longOption);
    this->shortOption = fmt::format("-{}", shortOption);
}

CmdLineParser&
CmdLineParser::on(const char *longOption,
            char shortOption,
            bool hasValue,
            const char *errorMessage,
            std::function<bool(const char*)> handler)
{
    (void) errorMessage;
    m_options.emplace_back(longOption, shortOption, hasValue, handler);
    return *this;
}

CmdLineParser&
CmdLineParser::on(const char *longOption,
            char shortOption,
            bool hasValue,
            const char *errorMessage,
            std::function<void(bool)> handler)
{
    (void) errorMessage;
    m_options.emplace_back(longOption, shortOption, hasValue, handler);
    return *this;
}

CmdLineParser&
CmdLineParser::on(const CmdLineParser::Option &option)
{
    m_options.push_back(option);
    return *this;
}

const char*
CmdLineParser::getBadOption() const noexcept
{
    return m_badOption;
}

CmdLineParser::ErrorClass
CmdLineParser::getError() const noexcept
{
    return m_ec;
}

const char*
CmdLineParser::getErrorString() const noexcept
{
    switch (m_ec)
    {
    case NoError:
        return "no error";
    case ArgumentMissing:
        return "argument missing";
    case UnknownOption:
        return "unknown option";
    default:
        return "unknown error";
    }
}

bool
CmdLineParser::parse(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];
        auto op = [arg](const Option &option) {
            return option.longOption == arg || option.shortOption == arg;
        };

        auto it = std::find_if(m_options.begin(), m_options.end(), op);
        if (it == m_options.end())
        {
            m_ec = UnknownOption;
            m_badOption = argv[i];
            return false;
        }
        if (it->hasValue)
        {
            if (i == argc - 1)
            {
                m_ec = ArgumentMissing;
                m_badOption = argv[i];
                return false;
            }

            it->handler1(argv[++i]);
        }
        else
        {
            it->handler0(true);
        }
    }
    return true;
}
}
}
#if 0
using namespace celestia::util;
#include <iostream>
int main(int argc, char *argv[])
{
    CmdLineParser cmdline;
    cmdline.on("value", 'v', true, "", [](const char *value) { std::cout << value << '\n'; return true; })
           .on("bool", 'b', false, "", [](bool on) { std::cout << "bool=" << on << '\n'; });
    if (!cmdline.parse(argc, argv))
    {
        std::cerr << "Bad option: " << cmdline.getBadOption() << ", " << cmdline.getErrorString() << '\n';
    }
    std::cout << sizeof(std::function<void(const char*)>) << ' ' << sizeof(std::function<void()>)  << '\n';
    return 0;
}
#endif
