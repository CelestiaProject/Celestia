#include <experimental/filesystem>

int main()
{
    std::error_code ec;
    throw std::experimental::filesystem::filesystem_error("test", ec);
}
