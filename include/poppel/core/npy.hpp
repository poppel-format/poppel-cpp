#ifndef POPPEL_SRC_CORE_NPY_HPP
#define POPPEL_SRC_CORE_NPY_HPP

// Types and functions for reading/writing .npy files.
// Inspired by https://github.com/llohse/libnpy and https://github.com/pmontalb/NpyCpp.

#include <algorithm>
#include <charconv>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef __cpp_lib_endian
    #include <bit>
#endif

#include "poppel/core/utilities.hpp"

namespace poppel::npy {

    namespace internal {
        using Index = std::int64_t;
        using Size  = std::int64_t;

        #ifdef __cpp_lib_endian
            using Endian = std::endian;
        #else
            enum class Endian {
                #ifdef _WIN32
                    little = 0,
                    big    = 1,
                    native = little
                #else
                    little = __ORDER_LITTLE_ENDIAN__,
                    big    = __ORDER_BIG_ENDIAN__,
                    native = __BYTE_ORDER__
                #endif
            };
        #endif

        constexpr char magic_string[] = "\x93NUMPY";
        constexpr Size magic_string_length = sizeof(magic_string) - 1;
        constexpr Size header_alignment = 64;
        constexpr char char_little_endian = '<';
        constexpr char char_big_endian = '>';
        constexpr char char_no_endian = '|';
        constexpr char char_host_endian = (Endian::native == Endian::big ? char_big_endian : char_little_endian);

        struct MaxAlignByteAllocator {
            using value_type = std::byte;
            using pointer = value_type*;

            template< typename T >
            struct rebind {
                using other = std::allocator<T>;
            };

            static constexpr std::align_val_t align_value { alignof(std::max_align_t) };

            pointer allocate(std::size_t n) {
                return static_cast<pointer>(::operator new(n, align_value));
            }
            void deallocate(pointer p, std::size_t n) {
                ::operator delete(p, align_value);
            }
        };
        using MaxAlignCharVector = std::vector<std::byte, MaxAlignByteAllocator>;

    } // namespace internal

    template< typename T >
    constexpr bool is_scalar = std::is_arithmetic_v<T> || std::is_same_v<T, std::complex<float>> || std::is_same_v<T, std::complex<double>>;

    struct Version {
        std::uint8_t major = 0;
        std::uint8_t minor = 0;
    };
    constexpr bool operator==(const Version& lhs, const Version& rhs) {
        return (lhs.major == rhs.major) && (lhs.minor == rhs.minor);
    }
    constexpr bool operator!=(const Version& lhs, const Version& rhs) {
        return !(lhs == rhs);
    }

    struct Dtype {
        char byteorder = 0;
        char kind = 0;
        internal::Size itemsize = 0;
    };
    constexpr bool operator==(const Dtype& lhs, const Dtype& rhs) {
        return (lhs.byteorder == rhs.byteorder) && (lhs.kind == rhs.kind) && (lhs.itemsize == rhs.itemsize);
    }
    constexpr bool operator!=(const Dtype& lhs, const Dtype& rhs) {
        return !(lhs == rhs);
    }

    struct Header {
        Dtype dtype;
        bool fortran_order = false;
        std::vector<internal::Size> shape;

        auto length() const noexcept {
            internal::Size ret = 1;
            for (auto const& s : shape) {
                ret *= s;
            }
            return ret;
        }
        auto numbytes() const noexcept {
            return length() * dtype.itemsize;
        }
    };
    constexpr bool operator==(const Header& lhs, const Header& rhs) {
        return (lhs.dtype == rhs.dtype) && (lhs.fortran_order == rhs.fortran_order) && (lhs.shape == rhs.shape);
    }
    constexpr bool operator!=(const Header& lhs, const Header& rhs) {
        return !(lhs == rhs);
    }

    struct NumpyArray {
        Header header;
        internal::MaxAlignCharVector rawdata;

        template< typename T = std::byte >
        auto data() noexcept {
            return reinterpret_cast<T*>(rawdata.data());
        }
        template< typename T = std::byte >
        auto data() const noexcept {
            return reinterpret_cast<const T*>(rawdata.data());
        }
    };


    namespace internal {

        // Get dtype kind size multiplier.
        // The itemsize is number specified in dtype, multiplied by this multiplier.
        constexpr Size kind_size_multiplier(char kind) {
            if(kind == 'U') {
                return 4;
            } else {
                return 1;
            }
        }

        // Get dtype for native types.
        constexpr Dtype dtype(std::int8_t  ) { return { char_no_endian,   'i', 1 }; }
        constexpr Dtype dtype(std::int16_t ) { return { char_host_endian, 'i', 2 }; }
        constexpr Dtype dtype(std::int32_t ) { return { char_host_endian, 'i', 4 }; }
        constexpr Dtype dtype(std::int64_t ) { return { char_host_endian, 'i', 8 }; }
        constexpr Dtype dtype(std::uint8_t ) { return { char_no_endian,   'u', 1 }; }
        constexpr Dtype dtype(std::uint16_t) { return { char_host_endian, 'u', 2 }; }
        constexpr Dtype dtype(std::uint32_t) { return { char_host_endian, 'u', 4 }; }
        constexpr Dtype dtype(std::uint64_t) { return { char_host_endian, 'u', 8 }; }
        constexpr Dtype dtype(float        ) { return { char_host_endian, 'f', 4 }; }
        constexpr Dtype dtype(double       ) { return { char_host_endian, 'f', 8 }; }
        constexpr Dtype dtype(std::complex<float> ) { return { char_host_endian, 'c', 8 }; }
        constexpr Dtype dtype(std::complex<double>) { return { char_host_endian, 'c', 16 }; }

        // All other integral types.
        template< typename T, std::enable_if_t< std::is_integral_v< T >>* = nullptr >
        constexpr Dtype dtype(T) {
            return dtype(core::NormalizedIntegralOf<T>{});
        }

        // Format Version 1.0 specification.
        // As of Dec 20, 2021. (https://numpy.org/devdocs/reference/generated/numpy.lib.format.html)
        //
        // Format Version 1.0
        // The first 6 bytes are a magic string: exactly \x93NUMPY.
        // The next 1 byte is an unsigned byte: the major version number of the file format, e.g. \x01.
        // The next 1 byte is an unsigned byte: the minor version number of the file format, e.g. \x00. Note: the version of the file format is not tied to the version of the numpy package.
        // The next 2 bytes form a little-endian unsigned short int: the length of the header data HEADER_LEN.
        // The next HEADER_LEN bytes form the header data describing the array’s format. It is an ASCII string which contains a Python literal expression of a dictionary. It is terminated by a newline (\n) and padded with spaces (\x20) to make the total of len(magic string) + 2 + len(length) + HEADER_LEN be evenly divisible by 64 for alignment purposes.
        //
        // The dictionary contains three keys:
        //
        // "descr" : dtype.descr
        // An object that can be passed as an argument to the numpy.dtype constructor to create the array’s dtype.
        //
        // "fortran_order" : bool
        // Whether the array data is Fortran-contiguous or not. Since Fortran-contiguous arrays are a common form of non-C-contiguity, we allow them to be written directly to disk for efficiency.
        //
        // "shape" : tuple of int
        // The shape of the array.
        //
        // For repeatability and readability, the dictionary keys are sorted in alphabetic order. This is for convenience only. A writer SHOULD implement this if possible. A reader MUST NOT depend on this.
        //
        // Following the header comes the array data. If the dtype contains Python objects (i.e. dtype.hasobject is True), then the data is a Python pickle of the array. Otherwise the data is the contiguous (either C- or Fortran-, depending on fortran_order) bytes of the array. Consumers can figure out the number of bytes by multiplying the number of elements given by the shape (noting that shape=() means there is 1 element) by dtype.itemsize.

        inline void write_magic(std::ostream &os, Version version) {
            os.write(magic_string, magic_string_length);
            os.put(version.major);
            os.put(version.minor);
        }

        // Returns npy version.
        inline auto read_magic(std::istream &is) {
            char buf[magic_string_length + 2];
            is.read(buf, magic_string_length + 2);

            if (!is) {
                throw std::runtime_error("io error: failed reading file");
            }

            if (0 != std::memcmp(buf, magic_string, magic_string_length)) {
                throw std::runtime_error("this file does not have a valid npy format.");
            }

            Version version;
            version.major = buf[magic_string_length];
            version.minor = buf[magic_string_length + 1];

            return version;
        }

        // Remove leading and trailing whitespaces
        inline std::string_view trim(std::string_view sv) {
            const char* whitespace = " \t";
            auto begin = sv.find_first_not_of(whitespace);

            if (begin == std::string_view::npos) {
                return "";
            }

            auto end = sv.find_last_not_of(whitespace);

            return sv.substr(begin, end - begin + 1);
        }

        constexpr Size preamble_length(Version version) {
            if(version.major == 1) {
                return magic_string_length + 2 + 2;
            } else {
                return magic_string_length + 2 + 4;
            }
        }

        // No quote.
        inline std::string gen_descr(Dtype dtype) {
            std::ostringstream oss;
            oss << dtype.byteorder << dtype.kind << (dtype.itemsize / kind_size_multiplier(dtype.kind));
            return oss.str();
        }

        // No quote.
        inline Dtype parse_descr(std::string_view sv_descr) {
            Dtype dtype;

            dtype.byteorder = sv_descr[0];
            dtype.kind = sv_descr[1];
            std::from_chars(sv_descr.data() + 2, sv_descr.data() + sv_descr.length(), dtype.itemsize);
            dtype.itemsize *= kind_size_multiplier(dtype.kind);

            return dtype;
        }

        // No parentheses.
        inline std::string gen_shape(const std::vector<Size>& shape) {
            if(shape.size() == 0) {
                return "";
            }
            else if(shape.size() == 1) {
                return std::to_string(shape.front()) + ",";
            }
            else {
                std::ostringstream ss;
                for(Index i = 0; i < shape.size() - 1; ++i) {
                    ss << shape[i] << ", ";
                }
                ss << shape.back();
                return ss.str();
            }
        }

        // No parentheses.
        inline std::vector<Size> parse_shape(std::string_view sv_shape) {
            std::vector<Size> shape;
            while(!sv_shape.empty()) {
                auto loc_comma = sv_shape.find(',');
                // loc_comma can be npos.
                auto sv_this_shape = trim(sv_shape.substr(0, loc_comma));
                if(!sv_this_shape.empty()) {
                    auto& this_shape = shape.emplace_back();
                    std::from_chars(sv_this_shape.data(), sv_this_shape.data() + sv_this_shape.size(), this_shape);
                }
                if(loc_comma == std::string_view::npos) {
                    break;
                }
                sv_shape.remove_prefix(loc_comma + 1);
            }
            return shape;
        }

        inline std::string gen_header(Version version, const Header& header) {
            std::ostringstream ss;
            ss << '{';

            ss << "'descr': '" << gen_descr(header.dtype) << "', ";
            ss << "'fortran_order': " << (header.fortran_order ? "True" : "False") << ", ";
            ss << "'shape': (" << gen_shape(header.shape) << "), ";

            ss << '}';

            auto ret = ss.str();

            const auto expected_length = preamble_length(version) + ret.length() + 1;
            const auto padding_length = (expected_length % header_alignment == 0 ? 0 : header_alignment - expected_length % header_alignment);
            ret.resize(ret.length() + padding_length, ' ');
            ret.push_back('\n');
            return ret;
        }

        inline Header parse_header(std::string_view sv) {

            // Remove trailing newline, as well as sandwiching whitespaces.
            if (sv.back() != '\n') {
                throw std::runtime_error("invalid header");
            }
            sv.remove_suffix(1);
            sv = trim(sv);

            // descr.
            //--------------------------
            auto loc_descr = sv.find("'descr': ");
            if(loc_descr == std::string_view::npos) {
                throw std::runtime_error("Cannot find descr in header.");
            }
            auto sv_descr_to_end = sv.substr(loc_descr + 9);
            auto loc_descr_prime1 = sv_descr_to_end.find("'");
            if(loc_descr_prime1 == std::string_view::npos) {
                throw std::runtime_error("Cannot find descr in header.");
            }
            auto loc_descr_prime2 = sv_descr_to_end.find("'", loc_descr_prime1 + 1);
            if(loc_descr_prime2 == std::string_view::npos) {
                throw std::runtime_error("Cannot find descr in header.");
            }
            auto sv_descr = sv_descr_to_end.substr(loc_descr_prime1 + 1, loc_descr_prime2 - loc_descr_prime1 - 1);
            if(sv_descr.length() < 3) {
                throw std::runtime_error("invalid typestring (length)");
            }
            Dtype dtype = parse_descr(sv_descr);

            // fortran_order.
            //--------------------------
            auto loc_fortran_order = sv.find("'fortran_order': ");
            if(loc_fortran_order == std::string_view::npos) {
                throw std::runtime_error("Cannot find fortran_order in header.");
            }
            const bool fortran_order = sv.substr(loc_fortran_order + 17, 4) == "True";

            // shape.
            //--------------------------
            auto loc_shape = sv.find("'shape': ");
            auto sv_shape_to_end = sv.substr(loc_shape + 9);
            auto loc_shape_l = sv_shape_to_end.find('(');
            auto loc_shape_r = sv_shape_to_end.find(')');
            if(loc_shape_l == std::string_view::npos || loc_shape_r == std::string_view::npos) {
                throw std::runtime_error("Cannot find value for shape in header.");
            }
            auto sv_shape = trim(sv_shape_to_end.substr(loc_shape_l + 1, loc_shape_r - loc_shape_l - 1));
            auto shape = parse_shape(sv_shape);

            return Header { dtype, fortran_order, std::move(shape) };
        }

        inline void write_header(std::ostream& os, Version version, std::string_view header) {

            write_magic(os, version);

            // write header length
            if (version == Version {1, 0}) {
                char header_len_le16[2];
                auto header_len = static_cast<std::uint16_t>(header.length());

                header_len_le16[0] = (header_len >> 0) & 0xff;
                header_len_le16[1] = (header_len >> 8) & 0xff;
                os.write(reinterpret_cast<char *>(header_len_le16), 2);
            } else {
                char header_len_le32[4];
                auto header_len = static_cast<std::uint32_t>(header.length());

                header_len_le32[0] = (header_len >> 0) & 0xff;
                header_len_le32[1] = (header_len >> 8) & 0xff;
                header_len_le32[2] = (header_len >> 16) & 0xff;
                header_len_le32[3] = (header_len >> 24) & 0xff;
                os.write(reinterpret_cast<char *>(header_len_le32), 4);
            }

            os << header;
        }

        inline std::string read_header(std::istream &is) {
            // check magic bytes an version number
            const auto version = read_magic(is);

            std::uint32_t header_length;
            if (version == Version {1, 0}) {
                char header_len_le16[2];
                is.read(header_len_le16, 2);
                header_length = (header_len_le16[0] << 0) | (header_len_le16[1] << 8);

                if ((preamble_length(version) + header_length) % header_alignment != 0) {
                    // TODO: display warning
                }
            } else if (version == Version {2, 0} || version == Version {3, 0}) {
                char header_len_le32[4];
                is.read(header_len_le32, 4);

                header_length = (header_len_le32[0] << 0) | (header_len_le32[1] << 8) | (header_len_le32[2] << 16) | (header_len_le32[3] << 24);

                if ((preamble_length(version) + header_length) % header_alignment != 0) {
                    // TODO: display warning
                }
            } else {
                throw std::runtime_error("unsupported file format version");
            }

            std::string header(header_length, 0);
            is.read(header.data(), header_length);

            return header;
        }
    } // namespace internal

    // Helper to create header with compile-time type information.
    template< typename T >
    inline Header create_header(bool fortran_order, std::vector<internal::Size> shape) {
        return Header { internal::dtype(T{}), fortran_order, std::move(shape) };
    }

    // Core function to save data.
    inline void save(std::ostream& os, const Header& header, const std::byte* data) {
        const Version version { 3, 0 };
        internal::write_header(os, version, internal::gen_header(version, header));

        const auto data_len = header.numbytes();
        os.write(reinterpret_cast<const char*>(data), data_len);
    }

    // Core function to load header only.
    // Precondition:
    // - is points to the beginning of the file.
    inline Header load_header(std::istream& is) {
        return internal::parse_header(internal::read_header(is));
    }

    // Core function to load data portion only.
    // Precondition:
    // - is points to the start of the data portion.
    // - data points to a buffer of at least numbytes bytes.
    inline void load_data(std::istream& is, std::byte* data, internal::Size numbytes) {
        is.read(reinterpret_cast<char*>(data), numbytes);
    }

    // Core function to load all data to a managed buffer.
    inline NumpyArray load(std::istream& is) {
        NumpyArray ret;
        ret.header = load_header(is);

        const auto numbytes = ret.header.numbytes();
        ret.rawdata.resize(numbytes);
        load_data(is, ret.rawdata.data(), numbytes);

        return ret;
    }

    // Core function to load all data to a pre-allocated buffer with known type and size.
    // Will check for header information match.
    inline void load(std::istream& is, Header header, std::byte* data) {
        const auto loaded_header = load_header(is);
        if (loaded_header != header) {
            throw std::runtime_error("header information mismatch");
        }

        const auto numbytes = header.numbytes();
        load_data(is, data, numbytes);
    }

    namespace internal {
        inline std::ofstream open_file_for_save(const std::filesystem::path& filename) {
            std::ofstream ofs(filename, std::ios::binary);
            if(!ofs) {
                throw std::runtime_error("cannot open file for save");
            }
            return ofs;
        }
        inline std::ofstream open_file_for_save(std::string_view filename) {
            return open_file_for_save(std::filesystem::path(filename));
        }

        inline std::ifstream open_file_for_load(const std::filesystem::path& filename) {
            std::ifstream ifs(filename, std::ios::binary);
            if(!ifs) {
                throw std::runtime_error("cannot open file for load");
            }
            return ifs;
        }
        inline std::ifstream open_file_for_load(std::string_view filename) {
            return open_file_for_load(std::filesystem::path(filename));
        }
    } // namespace internal

    //----------------------------------
    // Various save functions.
    //----------------------------------

    inline void save(const std::filesystem::path& filename, const Header& header, const std::byte* data) {
        auto ofs = internal::open_file_for_save(filename);
        save(ofs, header, data);
    }
    inline void save(std::string_view filename, const Header& header, const std::byte* data) {
        auto ofs = internal::open_file_for_save(filename);
        save(ofs, header, data);
    }

    // Save scalar data.
    template< typename T, std::enable_if_t< is_scalar<T> >* = nullptr >
    inline void save(std::ostream& os, T data) {
        Header header { internal::dtype(data), false, {} };
        save(os, header, reinterpret_cast<const std::byte*>(&data));
    }
    // Save vector of scalar.
    template< typename T, std::enable_if_t< is_scalar<T> >* = nullptr >
    inline void save(std::ostream& os, const std::vector<T>& data) {
        Header header { internal::dtype(T{}), false, { static_cast<internal::Size>(data.size()) } };
        save(os, header, reinterpret_cast<const std::byte*>(data.data()));
    }
    // Save string.
    inline void save(std::ostream& os, std::string_view data) {
        Header header { internal::dtype(char{}), false, { static_cast<internal::Size>(data.size()) } };
        save(os, header, reinterpret_cast<const std::byte*>(data.data()));
    }

    template< typename T >
    inline void save(const std::filesystem::path& filename, const T& data) {
        auto ofs = internal::open_file_for_save(filename);
        save(ofs, data);
    }
    template< typename T >
    inline void save(std::string_view filename, const T& data) {
        auto ofs = internal::open_file_for_save(filename);
        save(ofs, data);
    }

    //----------------------------------
    // Various load functions.
    //----------------------------------

    inline Header load_header(const std::filesystem::path& filename) {
        auto ifs = internal::open_file_for_load(filename);
        return load_header(ifs);
    }
    inline Header load_header(std::string_view filename) {
        auto ifs = internal::open_file_for_load(filename);
        return load_header(ifs);
    }

    inline NumpyArray load(const std::filesystem::path& filename) {
        auto ifs = internal::open_file_for_load(filename);
        return load(ifs);
    }
    inline NumpyArray load(std::string_view filename) {
        return load(std::filesystem::path(filename));
    }
    inline void load(const std::filesystem::path& filename, Header header, std::byte* data) {
        auto ifs = internal::open_file_for_load(filename);
        load(ifs, header, data);
    }
    inline void load(std::string_view filename, Header header, std::byte* data) {
        load(std::filesystem::path(filename), header, data);
    }

    // Load scalar data.
    template< typename T, std::enable_if_t< is_scalar<T> >* = nullptr >
    inline void load(std::istream& is, T& data) {
        Header header = load_header(is);
        if(header.shape.size() != 0) {
            throw std::runtime_error("array is not scalar (0-dimensional)");
        }
        if(header.dtype != internal::dtype(T{})) {
            throw std::runtime_error("array dtype is not match");
        }
        load_data(is, reinterpret_cast<std::byte*>(&data), header.numbytes());
    }
    // Load vector of scalar.
    template< typename T, std::enable_if_t< is_scalar<T> >* = nullptr >
    inline void load(std::istream& is, std::vector<T>& data) {
        Header header = load_header(is);
        if(header.shape.size() != 1) {
            throw std::runtime_error("array is not 1-dimensional");
        }
        if(header.dtype != internal::dtype(T{})) {
            throw std::runtime_error("array dtype is not match");
        }
        data.resize(header.shape[0]);
        load_data(is, reinterpret_cast<std::byte*>(data.data()), header.numbytes());
    }
    // Load string.
    inline void load(std::istream& is, std::string& data) {
        Header header = load_header(is);
        if(header.shape.size() != 1) {
            throw std::runtime_error("array is not 1-dimensional");
        }
        if(header.dtype != internal::dtype(char{})) {
            throw std::runtime_error("array dtype is not match");
        }
        data.resize(header.shape[0]);
        load_data(is, reinterpret_cast<std::byte*>(data.data()), header.numbytes());
    }

    template< typename T >
    inline void load(const std::filesystem::path& filename, T& data) {
        auto ifs = internal::open_file_for_load(filename);
        load(ifs, data);
    }
    template< typename T >
    inline void load(std::string_view filename, T& data) {
        auto ifs = internal::open_file_for_load(filename);
        load(ifs, data);
    }

} // namespace poppel::npy

#endif
