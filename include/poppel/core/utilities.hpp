#ifndef INCLUDE_POPPEL_CORE_UTILTIES_HPP
#define INCLUDE_POPPEL_CORE_UTILTIES_HPP

#include <type_traits>

namespace poppel::core {
    template< std::size_t NumBytes, bool Signed >
    struct NormalizedIntegral;
    template<> struct NormalizedIntegral<1, true> { using Type = std::int8_t; };
    template<> struct NormalizedIntegral<2, true> { using Type = std::int16_t; };
    template<> struct NormalizedIntegral<4, true> { using Type = std::int32_t; };
    template<> struct NormalizedIntegral<8, true> { using Type = std::int64_t; };
    template<> struct NormalizedIntegral<1, false> { using Type = std::uint8_t; };
    template<> struct NormalizedIntegral<2, false> { using Type = std::uint16_t; };
    template<> struct NormalizedIntegral<4, false> { using Type = std::uint32_t; };
    template<> struct NormalizedIntegral<8, false> { using Type = std::uint64_t; };

    template< typename T >
    using NormalizedIntegralOf = typename NormalizedIntegral<sizeof(T), std::is_signed_v<T>>::Type;

    // Execute the function on scope exit.
    template< typename Func >
    struct ScopeGuard {
        Func func;
        ~ScopeGuard() { func(); }
    };
    template< typename Func >
    ScopeGuard(Func) -> ScopeGuard< Func >;
} // namespace poppel::core

#endif
