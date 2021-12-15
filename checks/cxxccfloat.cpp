#include <charconv>

int main()
{
    const char* src = "123";
    float x;
    std::from_chars_result result = std::from_chars(src, src + 3, x);
}
