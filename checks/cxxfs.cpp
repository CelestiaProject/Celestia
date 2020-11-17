#include <filesystem>

int main()
{
    std::error_code ec;
    throw std::filesystem::filesystem_error("test", ec);
}
