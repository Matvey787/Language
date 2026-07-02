module;

#include <concepts>

export module ast_extensions;

export import ast_location_ext;
export import ast_error_handler_ext;

namespace ast
{

export template <typename ExtensionT>
concept isExtension = requires(ExtensionT ext) {
    { ext.used() } -> std::same_as<bool>;
    { ext } -> std::convertible_to<bool>;
};

} // namespace ast
