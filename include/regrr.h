#pragma once


#include <opencv2/core.hpp>
#include <string>
#include <type_traits>

#if ENABLE_REGRR

    /**
     * @{
     * Generate unique identifiers for variables to avoid clashes.
     * We need like "anonymous variable" to trigger the destructor when we leave the scope.
     * Internal to the library.
     */

    #define REGRR_UNIQUE3(x, y) x##y
    #define REGRR_UNIQUE2(x, y) REGRR_UNIQUE3(x, y)
    #define REGRR_UNIQUE REGRR_UNIQUE2(regrr_12345654321abcdef, __LINE__)
    /**
     * @}
     */

    /**
     * Enter a scope.
     *
     * Example:
     * ```
     * REGRR_SCOPED("MySuperScopeCount-%d", 3);
     * ```
     */
    #define REGRR_SCOPED(...) regrr::Scope REGRR_UNIQUE(__VA_ARGS__)

    /**
     * Save a matrix.
     *
     * Example:
     * ```
     * cv::Mat m;
     * REGRR_SAVE(m, "MySuperName-%d", 10);
     * ```
     */
    #define REGRR_SAVE(mat, ...) do { regrr::save(cv::Mat(mat), true, nullptr, nullptr, __VA_ARGS__); } while(false)

    /**
     * Create a managed matrix.
     * Saved when exit code scope.
     *
     * Example:
     * ```
     * REGRR_CREATE_MAT(cv::Vec3f, 10, 10, "MyMatrix");
     * ```
     */
    #define REGRR_CREATE_MAT(type, rows, cols, ...) regrr::ManagedMatrix REGRR_UNIQUE(cv::Mat_<type>(rows, cols), __VA_ARGS__)

    /**
     * Set a pixel of a managed matrix.
     *
     * Example:
     * ```
     * REGRR_SET_PX(1, 2, Vec3f(1.0f, 2.0f, 3.0f), "MyMatrix");
     * ```
     */
    #define REGRR_SET_PX(row, col, value, ...) regrr::get_mat(__VA_ARGS__).template at<std::decay_t<decltype(value)>>(row, col) = (value)
#else

    // Disable all macros
    #define REGRR_SCOPED(...) do {} while(0)
    #define REGRR_SAVE(...) do {} while(0)
    #define REGRR_CREATE_MAT(...) do {} while(0)
    #define REGRR_SET_PX(...) do {} while(0)

#endif


namespace regrr
{
    /**
     * Check if the library is enabled.
     *
     * @return true If the library is enabled.
     */
    bool enabled();

    /**
     * Save a matrix to a file.
     * Save it immediately.
     *
     * @param append If the matrix should be added to the lists file.
     * For example, it should be disabled for managed matrices as this is added beforehand.
     *
     * @param[in] call The call count when the function was called. If nullptr, increment the internal call counter.
     * @param[in] scopes The scopes when the function was called. If nullptr, use the current scopes.
     *
     * @throw std::runtime_error If the file could not be written.
     */
    void save(const cv::Mat& mat, bool append, const int *call, const std::vector<std::string>* scopes, const char *fmt, ...);

    /**
     * Enter a scope.
     */
    void enter_scope(const char *fmt, ...);

    /**
     * Exit a scope.
     *
     * @throws std::runtime_error If not inside a scope.
     */
    void exit_scope();

    /**
     * Store in-memory a managed matrix.
     * Saving is deferred: it is written to a file when the corresponding `release_mat()` is called with the same name.
     * However, the line is added immediately to the lists file, to represent better the execution order.
     *
     * @param mat The matrix to store internally as a managed matrix.
     * It should not be modified nor used by the user, as OpenCV use sharing mechanism.
     * The user should release all references to this variable.
     *
     * @throw std::runtime_error If a matrix with the same name is already stored or the library is not enabled.
     */
    void store_mat(cv::Mat mat, const char *fmt, ...);

    /**
     * Get a managed matrix.
     *
     * @throw std::runtime_error If no managed matrix with this name exist.
     */
    [[nodiscard]] cv::Mat& get_mat(const char *fmt, ...);

    /**
     * Release a managed matrix.
     *
     * @throw std::runtime_error If no managed matrix with this name exist.
     */
    void release_mat(const char *fmt, ...);

    /**
     * RAII for scope.
     */
    class Scope
    {
    public:
        /**
         * Will call `enter_scope()`.
         * @param fmt The same argument as `enter_scope()`.
         */
        explicit Scope(const char *fmt, ...);

        /**
         * Will call `exit_scope()`.
         */
        ~Scope();

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    };

    /**
     * RAII for managed matrices.
     */
    class ManagedMatrix
    {
    public:
        /**
         * Will call `store_mat()`.
         * @param mat The same argument as `store_mat()`.
         * @param fmt The same argument as `store_mat()`.
         */
        explicit ManagedMatrix(cv::Mat mat, const char *fmt, ...);

        /**
         * Will call `release_mat()`.
         */
        ~ManagedMatrix();

        ManagedMatrix(const ManagedMatrix&) = delete;
        ManagedMatrix& operator=(const ManagedMatrix&) = delete;

    private:
        std::string m_name;
    };
}