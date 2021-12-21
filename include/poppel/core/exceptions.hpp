#ifndef INCLUDE_POPPEL_CORE_EXCEPTIONS_HPP_
#define INCLUDE_POPPEL_CORE_EXCEPTIONS_HPP_

#include <stdexcept>
#include <string>
#include <string_view>

namespace poppel {

    struct Exception : std::exception {
        std::string message;
        Exception(std::string_view message) : message(message) {}
        const char* what() const noexcept override { return message.c_str(); }
    };

    struct NotImplementedError : Exception {
        NotImplementedError(std::string_view message) : Exception(message) {}
    };

} // namespace poppel

#endif
