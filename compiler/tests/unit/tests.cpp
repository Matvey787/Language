#include <filesystem>
#include <format>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

const std::string exe_path_c   = "compiler/build/paracl";
const std::string tests_path_c = "compiler/tests/";

static std::string
run(std::string_view file_name)
{
    auto&& file =
        std::filesystem::path(tests_path_c) / "e2e" / "in" / file_name;

    auto exe_out = std::filesystem::path(file).replace_extension("exe");
    auto ll_out  = std::filesystem::path(file).replace_extension("ll");

    std::string compile_cmd =
        std::format("{} {} -o {}", exe_path_c, file.string(), exe_out.string());

    auto&& compile_ret = std::system(compile_cmd.c_str());

    if (compile_ret != 0)
    {
        return "";
    }

    std::string out;
    std::array<char, 4096> buf{};

    FILE* fstream = popen(exe_out.string().c_str(), "r");

    if (fstream == nullptr)
    {
        return "";
    }
    while (fgets(buf.data(), static_cast<int>(buf.size()), fstream) != nullptr)
    {
        out += buf.data();
    }
    pclose(fstream);

    std::filesystem::remove(exe_out);
    std::filesystem::remove(ll_out);

    return out;
}

static std::string
read(const std::filesystem::path& file_name)
{
    auto file = std::filesystem::path(tests_path_c) / "e2e" / "out" / file_name;

    std::ifstream f(file);
    std::ostringstream ss;

    ss << f.rdbuf();

    return ss.str();
}

TEST(E2E, FunctionsDefaultArgs)
{
    EXPECT_EQ(run("test1.myl"), read("test1.ans"));
}

TEST(E2E, ScopedVariables)
{
    EXPECT_EQ(run("test2.myl"), read("test2.ans"));
}

int
main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
