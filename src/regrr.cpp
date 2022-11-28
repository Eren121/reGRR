#include "regrr.h"
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <string>
#include <iostream>
#include <cstdarg>
#include <utility>

#define REGRR_DIR "REGRR_DIR"
#define REGRR_EXT "REGRR_EXT"
#define REGRR_LISTS "lists.txt"

namespace fs = std::filesystem;

/**
 * Utility macro to transform a printf-like varargs to a string.
 * Limited to 1000 characters.
 *
 * @param output The string variable output where to store the result.
 * @param fmt The argument of the function which contain the format string, before the varargs parameters.
 */
#define REGRR_VARARGS_TO_STRING(output, fmt) \
        {char buf[1000]; \
        va_list args; \
        va_start(args, fmt); \
        vsnprintf(buf, sizeof(buf), fmt, args); \
        va_end(args); \
        output = std::string(buf);} do{}while(false)

namespace regrr
{
    namespace
    {
        /**
         * @{
         * Global variables for the internal state of the library.
         */

        /**
         * Structure to store managed matrices.
         * Store the call count when the matrice was created,
         * as well with the scope, to know which filename to give when saved,
         * because they could have changed between the creation and the release.
         */
        struct Managed
        {
            cv::Mat mat;
            std::vector<std::string> scopes;
            int call;
        };

        /**
         * Current scopes pushed.
         * scopes[0] stores the most outer scope.
         */
        std::vector<std::string> scopes;

        /**
         * Store for each matrix how many time it has been serialized.
         * Useful when some part of the algorithm is run multiple time but we want to check each separately.
         */
        std::unordered_map<std::string, int> callCounts;

        /**
         * Managed matrices.
         */
        std::unordered_map<std::string, Managed> managedMats;

        /**
         * If the library is enabled at runtime.
         * Set to true in the initialization if the user has enabled the library.
         */
        bool runtimeEnabled = false;

        /**
         * Output extension.
         * Default is XML file.
         */
        std::string outputExtension = ".xml";

        /**
         * Output directory.
         */
        std::string outputDir;

        /**
         * The path to the lists file.
         */
        std::string listsPath;

        /**
         * Ensure all the variables of the library are initialized.
         * Must be called in every function of the library.
         *
         * @return The value of runtimeEnabled.
         */
        bool ensure_initialized();

        /**
         * Called by `ensure_initialized()` on first call.
         */
        void initialize();

        /**
         * @}
         */

        /**
         * @{
         * Utility functions.
         */

        /**
         * Concat arguments into a string.
         * Utility function.
         */
        template<typename... Args>
        std::string concat(Args&& ... args)
        {
            return (std::ostringstream{} << ... << args).str();
        }

        /**
         * Concat arguments into a file path exactly like `os.path.join()` in Python.
         * Utility function.
         */
        template<typename First, typename... Args>
        std::string joinPaths(First&& first, Args&& ... args)
        {
            std::ostringstream os;
            os << std::forward<First>(first);
            ((os << "/" << std::forward<Args>(args)), ...);

            return os.str();
        }

        /**
         * Stringify a container with a separator between the entries.
         *
         * @return The joined string.
         *
         * @tparam T The container.
         */
        template<typename T>
        std::string join(const T& container, const std::string& separator)
        {
            std::ostringstream ss;
            bool first = true;

            for(const auto& entry: container)
            {
                if(!first)
                {
                    ss << separator;
                }
                else
                {
                    first = false;
                }

                ss << entry;
            }

            return ss.str();
        }

        /**
         * Create an empty file from a path.
         * If the file already exists, the previous content is erased.
         */
        void createEmptyFile(const std::string& filePath)
        {
            std::ofstream out(filePath, std::ios_base::out | std::ios_base::trunc);
        }

        /**
         * Append a line to a file.
         *
         * @param line The line to add.
         * A newline character '\n' is automatically added at the end of the line.
         *
         * @throw std::runtime_error If the file could not be written.
         */
        void appendLineToFile(const std::string& fileName, const std::string& line)
        {
            std::ofstream out(fileName, std::ios_base::app);
            if(!out)
            {
                throw std::runtime_error("Cannot open file for write: " + fileName);
            }

            out << line << std::endl;
        }

        /**
         * @}
         */

        // Implementation of initialization functions

        bool ensure_initialized()
        {
            static bool initialized = false;
            if(!initialized)
            {
                initialized = true;
                initialize();
            }

            return initialized;
        }

        void initialize()
        {
            // Check if the user set an output directory.
            // This will define if the library is enabled or not.
            if(const char *dir = std::getenv(REGRR_DIR); dir)
            {
                outputDir = dir;

                try
                {
                    // If the output directory does not exist, try to create it (noop if it already exists)
                    fs::create_directories(outputDir);

                    // Get the path of the lists file
                    listsPath = joinPaths(outputDir, REGRR_LISTS);

                    // Clear or create the lists file
                    createEmptyFile(listsPath);

                    // Check if custom file extension
                    if(const char *ext = std::getenv(REGRR_EXT); ext)
                    {
                        outputExtension = ext;
                    }

                    // First line is the file extension
                    appendLineToFile(listsPath, outputExtension);

                    // If no error, enable the library
                    runtimeEnabled = true;
                }
                catch(const fs::filesystem_error& error)
                {
                    std::cerr << "CANNOT INITIALIZE REGRESSION TESTS. ERROR IS:" << std::endl;
                    std::cerr << error.what() << std::endl;
                }
            }

            std::cout << "***** REGRR library initialized:" << std::endl;
            std::cout << "    enabled: " << runtimeEnabled << std::endl;
            std::cout << "    file extension: \"" << outputExtension << "\"" << std::endl;
            std::cout << "    output directory: \"" << outputDir << "\"" << std::endl;
            std::cout << "    lists path: \"" << listsPath << "\"" << std::endl;
        }
    }

    bool enabled()
    {
        return runtimeEnabled;
    }

    void enter_scope(const char *fmt, ...)
    {
        if(!ensure_initialized())
        {
            return;
        }

        std::string scopeName;
        REGRR_VARARGS_TO_STRING(scopeName, fmt);

        // Append the scope in memory
        scopes.push_back(scopeName);

        // Register we enter a scope in the lists file
        appendLineToFile(listsPath, concat("+ ", scopeName));
    }

    void exit_scope()
    {
        if(!ensure_initialized())
        {
            return;
        }

        if(scopes.empty())
        {
            throw std::runtime_error("Outside any scope");
        }

        // Pop the scope in memory
        scopes.pop_back();

        // Register we exit a scope in the lists file
        appendLineToFile(listsPath, "-");
    }

    void store_mat(cv::Mat mat, const char *fmt, ...)
    {
        if(!ensure_initialized())
        {
            return;
        }

        std::string matName;
        REGRR_VARARGS_TO_STRING(matName, fmt);

        if(managedMats.count(matName))
        {
            throw std::runtime_error("Managed matrix with the same name already exist: " + matName);
        }

        // Increase the call count
        const int call = (++callCounts[matName]);

        // Store the managed matrix in memory
        managedMats[matName] = Managed{
            .mat = std::move(mat),
            .scopes = scopes,
            .call = call,
        };

        // Append immediately to the lists file, with the call count
        appendLineToFile(listsPath, concat(matName, ".", call));
    }

    cv::Mat& get_mat(const char *fmt, ...)
    {
        if(!ensure_initialized())
        {
            throw std::runtime_error("The library should be enabled to use this function");
        }

        std::string matName;
        REGRR_VARARGS_TO_STRING(matName, fmt);

        auto it = managedMats.find(matName);
        if(it == managedMats.end())
        {
            throw std::runtime_error("Managed matrix with this name does not exist: " + matName);
        }

        return it->second.mat;
    }

    void release_mat(const char *fmt, ...)
    {
        if(!ensure_initialized())
        {
            return;
        }

        std::string matName;
        REGRR_VARARGS_TO_STRING(matName, fmt);

        auto it = managedMats.find(matName);
        if(it == managedMats.end())
        {
            throw std::runtime_error("Managed matrix with this name does not exist: " + matName);
        }

        // Save the matrix to a file, not appending to the lists
        // Protect if the string contains a '%'
        save(it->second.mat, false, &it->second.call, &it->second.scopes, "%s", matName.c_str());

        // Release memory
        // If the user has reference (which he shouldn't), then the matrix continue to live, but the library consider it not managed anymore
        managedMats.erase(it);
    }

    void save(const cv::Mat& mat, bool append, const int *callPtr, const std::vector<std::string>* scopesPtr, const char *fmt, ...)
    {
        if(!ensure_initialized())
        {
            return;
        }

        std::string matName;
        REGRR_VARARGS_TO_STRING(matName, fmt);

        // Get the call count, from the argument or from the internal counter
        int call;
        if(callPtr)
        {
            call = *callPtr;
        }
        else
        {
            // Increment the call count for this matrix
            call = (++callCounts[matName]);
        }

        // Get the scopes, from the argument or from the internal counter
        const std::vector<std::string> *scopes;
        if(scopesPtr)
        {
            scopes = scopesPtr;
        }
        else
        {
            scopes = &regrr::scopes;
        }

        // Save the file with full path:
        // `scope1/scope2/.../matName.callCount.ext`


        cv::FileStorage writer;
        const std::string fileName = concat(matName, ".", call, outputExtension);

        const fs::path path = joinPaths(outputDir, join(*scopes, "/"), fileName);
        std::cout << "Saving test " << path << std::endl;

        // Create intermediate directories if needed
        fs::create_directories(path.parent_path());

        // Open the file to write
        if(!writer.open(path, cv::FileStorage::WRITE))
        {
            throw std::runtime_error("Can't open file to write: " + path.string());
        }

        // Note:
        // cv::FileStorage doesn't support matrices with more than 4 elements
        // So we just flatten the channels in this case with reshape(1)
        // See https://stackoverflow.com/a/53676859/5110937

        // Write the file to disk
        // We don't care of the name of the node, but OpenCV requires one
        // We use a default name, and not the matrix name, in case of special characters that may not be handled by OpenCV
        writer << "root" << mat.reshape(1);
        writer.release();

        if(append)
        {
            // Append the name of the matrix to the lists file
            // Permit to iterate in the same order at the execution
            // We couldn't have use reliably the timestamp because it is OS-dependant whether the file will be created at some exact time in order
            // Also save the call count in the name
            appendLineToFile(listsPath, concat(matName, ".", call));
        }
    }

    // Implementation of RAII classes

    Scope::Scope(const char *fmt, ...)
    {
        std::string scopeName;
        REGRR_VARARGS_TO_STRING(scopeName, fmt);
        // Protect if the string contains a '%'
        enter_scope("%s", scopeName.c_str());
    }

    Scope::~Scope()
    {
        exit_scope();
    }

    ManagedMatrix::ManagedMatrix(cv::Mat mat, const char *fmt, ...)
    {
        REGRR_VARARGS_TO_STRING(m_name, fmt);
        // Protect if the string contains a '%'
        store_mat(std::move(mat), "%s", m_name.c_str());
    }

    ManagedMatrix::~ManagedMatrix()
    {
        // Protect if the string contains a '%'
        release_mat("%s", m_name.c_str());
    }
}