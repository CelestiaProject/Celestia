#if 1
#include <celcompat/fs.h>
namespace fs = celestia::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include <catch.hpp>

TEST_CASE("filesystem", "[filesystem]")
{
    SECTION("fs::path::extension()")
    {
        REQUIRE(fs::path("/foo/bar.txt").extension() == ".txt");
        REQUIRE(fs::path("/foo/bar.").extension() == ".");
        REQUIRE(fs::path("/foo/bar").extension().empty() == true);
        REQUIRE(fs::path("/foo/bar.txt/bar.cc").extension() == ".cc");
        REQUIRE(fs::path("/foo/bar.txt/bar.").extension() == ".");
        REQUIRE(fs::path("/foo/bar.txt/bar").extension().empty() == true);
        REQUIRE(fs::path("/foo/.").extension().empty() == true);
        REQUIRE(fs::path("/foo/..").extension().empty() == true);
        REQUIRE(fs::path("/foo/.hidden").extension().empty() == true);
        REQUIRE(fs::path("/foo/..bar").extension() == ".bar");
        REQUIRE(fs::path("/foo/bar.txt").stem() == "bar");
        REQUIRE(fs::path("/foo/.bar").stem() == ".bar");
    }

    SECTION("fs::path::stem()")
    {
        fs::path p = "foo.bar.baz.tar";
        REQUIRE(p.extension() == ".tar");
        p = p.stem();
        REQUIRE(p.extension() == ".baz");
        REQUIRE(p == "foo.bar.baz");
        p = p.stem();
        REQUIRE(p.extension() == ".bar");
        REQUIRE(p == "foo.bar");
        p = p.stem();
        REQUIRE(p.extension().empty() == true);
        REQUIRE(p == "foo");
    }

    SECTION("path separators")
    {
        REQUIRE(fs::path("/foo/bar.txt") == "/foo/bar.txt");
        REQUIRE(fs::path("baz/foo/bar.txt") == "baz/foo/bar.txt");
        // These two fail on Unix/GCC both with C++11 fs and our own.
        // But they are successful with MinGW.
#ifdef _WIN32
        REQUIRE(fs::path("c:\\foo\\bar.txt") == "c:/foo/bar.txt");
        REQUIRE(fs::path(L"c:\\foo\\bar.txt") == "c:/foo/bar.txt");
#endif
    }

    SECTION("fs::path::replace_extension()")
    {
        REQUIRE(fs::path("/foo/bar.jpg").replace_extension(".png") == "/foo/bar.png");
        REQUIRE(fs::path("/foo/bar.jpg").replace_extension("png") == "/foo/bar.png");
        REQUIRE(fs::path("/foo/bar.jpg").replace_extension(".") == "/foo/bar.");
        REQUIRE(fs::path("/foo/bar.jpg").replace_extension("") == "/foo/bar");

        REQUIRE(fs::path("/foo/bar.").replace_extension("png") == "/foo/bar.png");

        REQUIRE(fs::path("/foo/bar").replace_extension(".png") == "/foo/bar.png");
        REQUIRE(fs::path("/foo/bar").replace_extension("png") == "/foo/bar.png");
        REQUIRE(fs::path("/foo/bar").replace_extension(".") == "/foo/bar.");
        REQUIRE(fs::path("/foo/bar").replace_extension("") == "/foo/bar");

        REQUIRE(fs::path("/foo/.").replace_extension(".png") == "/foo/..png");
        REQUIRE(fs::path("/foo/.").replace_extension("png") == "/foo/..png");
        REQUIRE(fs::path("/foo/.").replace_extension(".") == "/foo/..");
        REQUIRE(fs::path("/foo/.").replace_extension("") == "/foo/.");

        REQUIRE(fs::path("/foo/").replace_extension(".png") == "/foo/.png");
        REQUIRE(fs::path("/foo/").replace_extension("png") == "/foo/.png");
    }
}
