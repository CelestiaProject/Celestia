#include <array>

constexpr std::array<int, 0> array {};
constexpr const int *data = array.data();

int main()
{
}

