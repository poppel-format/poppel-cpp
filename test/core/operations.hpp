#ifndef POPPEL_TEST_CORE_OPERATIONS_HPP
#define POPPEL_TEST_CORE_OPERATIONS_HPP

#include <fstream>

#include <catch2/catch.hpp>

#include <poppel/core/operations.hpp>

TEST_CASE("Poppel operations", "[operation]") {
    using namespace poppel;
    using namespace poppel::core;

    // Prepare test data.
    FileStates fs_r  { FileOpenState::ReadOnly };
    FileStates fs_rw { FileOpenState::ReadWrite };
    FileStates fs_c  { FileOpenState::Closed };

    auto temp_dir = std::filesystem::temp_directory_path();
    INFO("Temp directory path is " << temp_dir);
    auto pfile1   = temp_dir / "file1.poppel";
    auto pfile2   = temp_dir / "file2.poppel";
    auto pfile3   = temp_dir / "file3.poppel";
    auto tfile1   = temp_dir / "file1.txt";
    auto npyfile1 = temp_dir / "file1.npy";
    auto cleanup  = [&]() {
        std::filesystem::remove_all(pfile1);
        std::filesystem::remove_all(pfile2);
        std::filesystem::remove_all(pfile3);
        std::filesystem::remove_all(tfile1);
        std::filesystem::remove_all(npyfile1);
    };
    ScopeGuard cleanup_guard { cleanup };

    // Tests.
    SECTION("Utility functions.") {
        CHECK_NOTHROW(assert_file_open(fs_r));
        CHECK_NOTHROW(assert_file_open(fs_rw));
        CHECK_THROWS (assert_file_open(fs_c));

        CHECK_THROWS (assert_file_writable(fs_r));
        CHECK_NOTHROW(assert_file_writable(fs_rw));
        CHECK_THROWS (assert_file_writable(fs_c));

        CHECK(!is_valid_node_normalized_relpath(""));
        CHECK(!is_valid_node_normalized_relpath("."));
        CHECK(!is_valid_node_normalized_relpath(".."));
        CHECK(!is_valid_node_normalized_relpath("/"));
        CHECK(!is_valid_node_normalized_relpath("//"));
        CHECK(!is_valid_node_normalized_relpath("\\"));
        CHECK(!is_valid_node_normalized_relpath("C:\\"));
        CHECK(!is_valid_node_normalized_relpath("c/"));
        CHECK(!is_valid_node_normalized_relpath("../c"));
        CHECK( is_valid_node_normalized_relpath("c"));
        CHECK( is_valid_node_normalized_relpath("c/c"));

        CHECK_THROWS (assert_is_valid_node_normalized_relpath(""));
        CHECK_NOTHROW(assert_is_valid_node_normalized_relpath("c"));

        {
            Node node1 { NodeMeta { 1, NodeType::File }, };
            Node node2 { NodeMeta { 1, NodeType::Group }, };
            Node node3 { NodeMeta { 1, NodeType::Dataset }, };
            Node node4 { NodeMeta { 1, NodeType::Raw }, };
            Node node5 { NodeMeta { 1, NodeType::Unknown }, };

            CHECK( is_node_group(node1));
            CHECK( is_node_group(node2));
            CHECK(!is_node_group(node3));
            CHECK(!is_node_group(node4));
            CHECK(!is_node_group(node5));

            CHECK(!is_node_dataset(node1));
            CHECK(!is_node_dataset(node2));
            CHECK( is_node_dataset(node3));
            CHECK(!is_node_dataset(node4));
            CHECK(!is_node_dataset(node5));
        }

        cleanup();
        CHECK_NOTHROW(assert_not_exists(pfile1));
        CHECK_NOTHROW(assert_not_exists(tfile1));
        CHECK_THROWS (assert_exists_directory(pfile1));
        CHECK_THROWS (assert_exists_directory(tfile1));

        std::filesystem::create_directories(pfile1);
        { std::ofstream ofs(tfile1); REQUIRE(ofs.is_open()); }
        CHECK_THROWS (assert_not_exists(pfile1));
        CHECK_THROWS (assert_not_exists(tfile1));
        CHECK_NOTHROW(assert_exists_directory(pfile1));
        CHECK_THROWS (assert_exists_directory(tfile1));
    }

    SECTION("Node operations.") {
        SECTION("File nodes.") {
            cleanup();
            CHECK_THROWS(get_file_node(pfile1));

            auto node1 = require_file_node(pfile1);
            CHECK(node1.meta.type == NodeType::File);
            REQUIRE_NOTHROW(assert_exists_directory(pfile1));

            CHECK_THROWS(create_file_node(pfile1));
            delete_file_node(pfile1);
            CHECK(!std::filesystem::exists(pfile1));

            auto node2 = create_file_node(pfile2);
            CHECK(node2.meta.type == NodeType::File);
        }

        SECTION("Non-file (eg. group and dataset) nodes.") {
            cleanup();
            auto f1 = create_file_node(pfile1);

            // Node creation and deletion.
            CHECK       (!has_node(f1, "g1",         fs_r, NodeType::Group));
            CHECK_THROWS( has_node(f1, "/bad/path/", fs_r, NodeType::Group));
            CHECK_THROWS( has_node(f1, "g1",         fs_c, NodeType::Group));
            CHECK_THROWS(get_node(f1, "g1", fs_r, NodeType::Group));
            REQUIRE_THROWS(create_node(f1, "g1", fs_r, NodeType::Group));

            {
                auto f1n1 = create_node(f1, "g1", fs_rw, NodeType::Group);
                // 🗂️ f1 (File)
                // └─ 📂 g1
                CHECK(f1n1.meta.type == NodeType::Group);
                CHECK_THROWS(create_node(f1, "g1", fs_rw, NodeType::Group));
                CHECK_THROWS(create_node(f1, "g1", fs_rw, NodeType::Dataset));
                CHECK_THROWS(get_node(f1, "g1", fs_rw, NodeType::Dataset));

                REQUIRE_THROWS(delete_node(f1, "g1", fs_r));
                delete_node(f1, "g1", fs_rw);
            }

            {
                auto f1n1 = require_node(f1, "d1", fs_rw, NodeType::Dataset);
                // 🗂️ f1 (File)
                // └─ 🔢 d1
                CHECK(f1n1.meta.type == NodeType::Dataset);
            }

            // Nested node creation.
            {
                REQUIRE_THROWS(create_node(f1, "d1/g1", fs_rw, NodeType::Group));
                REQUIRE_THROWS(create_node(f1, "d1/d1", fs_rw, NodeType::Dataset));
                auto f1n11 = create_node(f1, "g1/g1", fs_rw, NodeType::Group);
                // 🗂️ f1 (File)
                // ├─ 🔢 d1
                // └─ 📂 g1
                //    └─ 📂 g1
                CHECK(f1n11.meta.type == NodeType::Group);
                auto f1n1 = get_node(f1, "g1", fs_r, NodeType::Group);
                CHECK(f1n1.meta.type == NodeType::Group);

                auto f1n111 = require_node(f1n1, "g1/d1", fs_rw, NodeType::Dataset);
                // 🗂️ f1 (File)
                // ├─ 🔢 d1
                // └─ 📂 g1
                //    └─ 📂 g1
                //       └─ 🔢 d1
                CHECK(f1n111.meta.type == NodeType::Dataset);
                f1n11 = get_node(f1, "g1/g1", fs_r, NodeType::Group);
                CHECK(f1n11.meta.type == NodeType::Group);
                CHECK(has_node(f1n11, "d1", fs_r, NodeType::Dataset));

                delete_node(f1, f1n11.relpath, fs_rw);
                // 🗂️ f1 (File)
                // ├─ 🔢 d1
                // └─ 📂 g1
                CHECK(!has_node(f1n11, "d1", fs_r, NodeType::Dataset));
            }
        }
    }

    SECTION("Dataset operations.") {
        DatasetMeta dm;

        // bool.
        save_from(true, npyfile1);
        dm = load_npy_meta(npyfile1);
        CHECK(dm.wordsize == sizeof(bool));
        CHECK(dm.shape.size() == 0);
        {
            bool val = false;
            load_to(val, npyfile1);
            CHECK(val);
        }

        // int.
        save_from(114514, npyfile1);
        dm = load_npy_meta(npyfile1);
        CHECK(dm.wordsize == sizeof(int));
        CHECK(dm.shape.size() == 0);
        {
            int val = 0;
            load_to(val, npyfile1);
            CHECK(val == 114514);
        }

        // float.
        save_from(1.2345f, npyfile1);
        dm = load_npy_meta(npyfile1);
        CHECK(dm.wordsize == sizeof(float));
        CHECK(dm.shape.size() == 0);
        {
            float val = 0.0f;
            load_to(val, npyfile1);
            CHECK(val == 1.2345f);
        }

        // complex<double>.
        save_from(std::complex<double>(1.2345, 2.3456), npyfile1);
        dm = load_npy_meta(npyfile1);
        CHECK(dm.wordsize == sizeof(std::complex<double>));
        CHECK(dm.shape.size() == 0);
        {
            std::complex<double> val = 0.0;
            load_to(val, npyfile1);
            CHECK(val == std::complex<double>(1.2345, 2.3456));
        }

        // vector<uint64_t>.
        {
            const std::vector<std::uint64_t> val1 { 1, 2, 3, 4, 5 };
            save_from(val1, npyfile1);
            dm = load_npy_meta(npyfile1);
            CHECK(dm.wordsize == sizeof(std::uint64_t));
            REQUIRE(dm.shape.size() == 1);
            CHECK(dm.shape[0] == 5);

            std::vector<std::uint64_t> val2;
            load_to(val2, npyfile1);
            CHECK(val2 == val1);
        }

        // vector<complex<float>>.
        {
            const std::vector<std::complex<float>> val1 {
                std::complex<float>(1.0f, 2.0f),
                std::complex<float>(3.0f, 4.0f),
                std::complex<float>(5.0f, 6.0f),
            };
            save_from(val1, npyfile1);
            dm = load_npy_meta(npyfile1);
            CHECK(dm.wordsize == sizeof(std::complex<float>));
            REQUIRE(dm.shape.size() == 1);
            CHECK(dm.shape[0] == 3);

            std::vector<std::complex<float>> val2;
            load_to(val2, npyfile1);
            CHECK(val2 == val1);
        }

        // string.
        {
            const std::string val1 = "Hallo/Hello/你好/こんにちは/안녕하세요";
            save_from(val1, npyfile1);
            dm = load_npy_meta(npyfile1);
            CHECK(dm.wordsize == sizeof(char));
            REQUIRE(dm.shape.size() == 1);
            CHECK(dm.shape[0] == val1.size());

            std::string val2;
            load_to(val2, npyfile1);
            CHECK(val2 == val1);
        }

        // 3x3 matrix of double, fortran order. Using raw buffer methods.
        {
            const std::vector<double> val1 {
                1, 4, 7,   2, 5, 8,   3, 6, 9,
            };
            const bool fortran_order = true;
            const std::vector<Size> shape { 3, 3 };
            save_from(val1.data(), fortran_order, shape, npyfile1);

            dm = load_npy_meta(npyfile1);
            CHECK(dm.wordsize == sizeof(double));
            REQUIRE(dm.shape.size() == 2);
            CHECK(dm.shape[0] == 3);
            CHECK(dm.shape[1] == 3);

            std::vector<double> val2(9);
            load_to(val2.data(), fortran_order, shape, npyfile1);
            CHECK(val2 == val1);
        }
    }

    SECTION("Attribute operations.") {
        cleanup();
        auto f1 = create_file_node(pfile1);
        // 🗂️ f1 (File)
        REQUIRE(std::filesystem::is_directory(pfile1));

        auto f1d1 = create_node(f1, "d1", fs_rw, NodeType::Dataset);
        auto f1g1 = create_node(f1, "g1", fs_rw, NodeType::Group);
        auto f1g1g1 = create_node(f1g1, "g1", fs_rw, NodeType::Group);
        // 🗂️ f1 (File)
        // ├─ 🔢 d1
        // └─ 📂 g1
        //    └─ 📂 g1

        REQUIRE_THROWS(get_attribute(f1, fs_r));
        auto attrobj_f1 = get_attribute(f1, fs_rw);
        REQUIRE_NOTHROW(get_attribute(f1, fs_r));
        CHECK(attrobj_f1.jsonfile == pfile1 / "attributes.json");
        {
            Json j;
            j["hello target"] = "world";
            j["spacetime dimension"] = 4;
            j["planck constant"] = 6.62607015e-34;
            save_attr(j, attrobj_f1);
            // 🗂️ f1 (File)
            // ├─ 🏷️ hello target
            // ├─ 🏷️ spacetime dimension
            // ├─ 🏷️ planck constant
            // ├─ 🔢 d1
            // └─ 📂 g1
            //    └─ 📂 g1
        }
        {
            const auto j = load_attr(attrobj_f1);
            CHECK(j["hello target"].get<std::string>() == "world");
            CHECK(j["spacetime dimension"].get<int>() == 4);
            CHECK(j["planck constant"].get<double>() == 6.62607015e-34);
        }

        auto attrobj_f1d1 = get_attribute(f1d1, fs_rw);
        CHECK(attrobj_f1d1.jsonfile == pfile1 / "d1/attributes.json");
        {
            Json j;
            j["teyvat nations"] = { "Mondstadt", "Liyue", "Inazuma", "Sumeru", "Fontaine", "Natlan", "Snezhnaya" };
            save_attr(j, attrobj_f1d1);
            // 🗂️ f1 (File)
            // ├─ 🏷️ hello target
            // ├─ 🏷️ spacetime dimension
            // ├─ 🏷️ planck constant
            // ├─ 🔢 d1
            // │  └─ 🏷️ teyvat nations
            // └─ 📂 g1
            //    └─ 📂 g1
        }
        {
            const auto j = load_attr(attrobj_f1d1);
            const auto v = j["teyvat nations"].get<std::vector<std::string>>();
            REQUIRE(v.size() == 7);
            CHECK(v[1] == "Liyue");
        }

        auto attrobj_f1g1g1 = get_attribute(f1g1g1, fs_rw);
        CHECK(attrobj_f1g1g1.jsonfile == pfile1 / "g1/g1/attributes.json");
        {
            Json j;
            j["TREE function table"] = {
                { "1", 1 },
                { "2", 3 },
                { "3", "BOOM" },
            };
            save_attr(j, attrobj_f1g1g1);
            // 🗂️ f1 (File)
            // ├─ 🏷️ hello target
            // ├─ 🏷️ spacetime dimension
            // ├─ 🏷️ planck constant
            // ├─ 🔢 d1
            // │  └─ 🏷️ teyvat nations
            // └─ 📂 g1
            //    └─ 📂 g1
            //       └─ 🏷️ TREE function table
        }
        {
            const auto j = load_attr(attrobj_f1g1g1);
            CHECK(j["TREE function table"]["2"].get<int>() == 3);
            CHECK(j["TREE function table"]["3"].get<std::string>() == "BOOM");
        }
    }
}

#endif
