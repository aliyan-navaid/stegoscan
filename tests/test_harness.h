#ifndef STEGOSCAN_TESTS_TEST_HARNESS_H_
#define STEGOSCAN_TESTS_TEST_HARNESS_H_

#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace test_framework {

class TestAssertionFailure : public std::exception {
public:
    explicit TestAssertionFailure(std::string message) : message_(std::move(message)) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

struct TestCase {
    std::string suite_name;
    std::string test_name;
    std::function<void()> func;
};

inline std::vector<TestCase>& get_test_registry() {
    static std::vector<TestCase> registry;
    return registry;
}

struct AutoRegister {
    AutoRegister(const std::string& suite, const std::string& name, std::function<void()> func) {
        get_test_registry().push_back({suite, name, std::move(func)});
    }
};

inline int run_all_tests() {
    auto& tests = get_test_registry();
    std::size_t passed = 0;
    std::size_t failed = 0;

    std::cout << "=========================================================\n";
    std::cout << " Running " << tests.size() << " registered unit test(s)\n";
    std::cout << "=========================================================\n";

    for (const auto& test : tests) {
        std::cout << "[ RUN      ] " << test.suite_name << "." << test.test_name << "\n";
        try {
            test.func();
            std::cout << "[       OK ] " << test.suite_name << "." << test.test_name << "\n";
            passed++;
        } catch (const TestAssertionFailure& ex) {
            std::cerr << "[  FAILED  ] " << test.suite_name << "." << test.test_name << "\n";
            std::cerr << "             " << ex.what() << "\n";
            failed++;
        } catch (const std::exception& ex) {
            std::cerr << "[  FAILED  ] " << test.suite_name << "." << test.test_name << "\n";
            std::cerr << "             Unexpected std::exception: " << ex.what() << "\n";
            failed++;
        } catch (...) {
            std::cerr << "[  FAILED  ] " << test.suite_name << "." << test.test_name << "\n";
            std::cerr << "             Unexpected non-std exception thrown\n";
            failed++;
        }
    }

    std::cout << "=========================================================\n";
    std::cout << " Test Summary: " << passed << " passed, " << failed << " failed, "
              << tests.size() << " total\n";
    std::cout << "=========================================================\n";

    return (failed == 0) ? 0 : 1;
}

} // namespace test_framework

#define TEST_CASE(suite_name, test_name) \
    static void test_##suite_name##_##test_name(); \
    static ::test_framework::AutoRegister reg_##suite_name##_##test_name( \
        #suite_name, #test_name, test_##suite_name##_##test_name); \
    static void test_##suite_name##_##test_name()

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw ::test_framework::TestAssertionFailure( \
                std::string("ASSERT_TRUE failed: ") + #condition + \
                " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            throw ::test_framework::TestAssertionFailure( \
                std::string("ASSERT_FALSE failed: ") + #condition + \
                " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            throw ::test_framework::TestAssertionFailure( \
                std::string("ASSERT_EQ failed: (") + #a + " == " + #b + \
                ") at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if (!((a) != (b))) { \
            throw ::test_framework::TestAssertionFailure( \
                std::string("ASSERT_NE failed: (") + #a + " != " + #b + \
                ") at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_NEAR(a, b, eps) \
    do { \
        if (std::abs((a) - (b)) > (eps)) { \
            throw ::test_framework::TestAssertionFailure( \
                std::string("ASSERT_NEAR failed: std::abs(") + #a + " - " + #b + \
                ") <= " + #eps + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_THROW(expression, exception_type) \
    do { \
        bool caught_expected = false; \
        try { \
            (expression); \
        } catch (const exception_type&) { \
            caught_expected = true; \
        } catch (const std::exception& e) { \
            throw ::test_framework::TestAssertionFailure( \
                std::string("ASSERT_THROW expected ") + #exception_type + \
                " but caught " + e.what() + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } catch (...) { \
            throw ::test_framework::TestAssertionFailure( \
                std::string("ASSERT_THROW expected ") + #exception_type + \
                " but caught unknown exception at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
        if (!caught_expected) { \
            throw ::test_framework::TestAssertionFailure( \
                std::string("ASSERT_THROW expected ") + #exception_type + \
                " was not thrown at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_NO_THROW(expression) \
    do { \
        try { \
            (expression); \
        } catch (const std::exception& e) { \
            throw ::test_framework::TestAssertionFailure( \
                std::string("ASSERT_NO_THROW unexpected exception: ") + e.what() + \
                " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } catch (...) { \
            throw ::test_framework::TestAssertionFailure( \
                std::string("ASSERT_NO_THROW unexpected non-std exception at ") + \
                __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#endif // STEGOSCAN_TESTS_TEST_HARNESS_H_
