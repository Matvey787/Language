module;

#include <cstddef>

export module ast_location_ext;

namespace ast
{

export template <typename Derived> class LocationExt
{
    using locT = size_t;

    locT line_{ 0 };
    locT col_{ 0 };
    locT width_{ 0 };
    locT height_{ 0 };

public:
    LocationExt() = default;

    LocationExt(locT line, locT col, locT width = 0, locT height = 0) :
        line_{ line }, col_{ col }, width_{ width }, height_{ height }
    {}

    [[nodiscard]] locT
    getLine() const
    {
        return line_;
    }
    [[nodiscard]] locT
    getCol() const
    {
        return col_;
    }
    [[nodiscard]] locT
    getWidth() const
    {
        return width_;
    }
    [[nodiscard]] locT
    getHeight() const
    {
        return height_;
    }

    [[nodiscard]] bool
    used() const noexcept
    {
        return (line_ > 0) && (col_ > 0);
    }

    operator bool() const noexcept { return used(); }
};

} // namespace ast
