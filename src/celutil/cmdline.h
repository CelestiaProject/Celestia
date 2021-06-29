#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <fmt/format.h>

namespace celestia
{
namespace util
{
class CmdLineParser
{
 public:
    enum ErrorClass
    {
        NoError         = 0,
        ArgumentMissing = 1,
        UnknownOption   = 2,
    };

    struct Option
    {
        std::string longOption;
        std::string shortOption;
        bool hasValue;
        std::function<bool(const char*)> handler1;
        std::function<void(bool)> handler0;

        Option(const char *longOption,
               char shortOption,
               bool hasValue,
               std::function<bool(const char*)> handler);

        Option(const char *longOption,
               char shortOption,
               bool hasValue,
               std::function<void(bool)> handler);
    };

    bool parse(int argc, char *argv[]);
    const char* getBadOption() const noexcept;
    ErrorClass  getError() const noexcept;
    const char* getErrorString() const noexcept;

    CmdLineParser& on(const char *longOption,
                      char shortOption,
                      bool hasValue,
                      const char *errorMessage,
                      std::function<bool(const char*)> handler);

    CmdLineParser& on(const char *longOption,
                      char shortOption,
                      bool hasValue,
                      const char *errorMessage,
                      std::function<void(bool)> handler);

    CmdLineParser& on(const Option &option);

 private:
    std::vector<Option> m_options;
    const char*         m_badOption { nullptr };
    ErrorClass          m_ec        { NoError };
};
}
}
