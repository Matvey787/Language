module;

#include <array>
#include <string_view>

export module project_info;

export namespace project_info
{


class Author
{
public:
    std::string_view name_;
    std::string_view git_link_;
};
constexpr std::string_view project_name_c = "@PROJECT_NAME@";
constexpr std::string_view project_version_c = "@PROJECT_VERSION@";

constexpr std::string_view project_overview_c =
    "ParaCL is a custom programming language compiler developed as an educational project during\n"
    "the second year of MIPT university. The project was proposed by Konstantin Vladimirovich as\n"
    "part of the YADRO course curriculum. For questions, suggestions, or contributions, feel free\n"
    "to contact: matveyklg@gmail.com If you encounter any bugs, issues, or have interesting \n"
    "findings, please open an issue at: https://github.com/Matvey787/Language\n";

constexpr std::array<Author, @AUTHORS_COUNT@> authors_c = { @AUTHORS_LIST_CPP@ };


} // namespace project_info
