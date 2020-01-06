#pragma once

#include "config.hpp"
#include <exception>
#include <ostream>
#include <string>

namespace program {

    struct explainer
    {
        std::exception_ptr ep_;

        static void
        emit(std::ostream &os, std::string const &message, std::size_t level)
        {
            static thread_local std::string padding;
            padding.clear();
            if (level) padding += '\n';
            padding.append(level, ' ');
            os << padding << message;
        }

        static void
        process(std::ostream &os, beast::system_error &e, std::size_t level = 0)
        {
            using namespace std::literals;

            emit(os, "exception: "s + e.code().category().name()
                     + " : " + std::to_string(e.code().value())
                     + " : " + e.what(), level);

            try
            {
                std::rethrow_if_nested(e);
            }
            catch (beast::system_error &child)
            {
                process(os, child, level + 1);
            }
            catch (std::exception &child)
            {
                process(os, child, level + 1);
            }
            catch (...)
            {
                emit(os, "nonstandard exception", level + 1);
            }
        }

        static void
        process(std::ostream &os, std::exception &e, std::size_t level = 0)
        {
            using namespace std::literals;

            emit(os, "exception: "s + e.what(), level);

            try
            {
                std::rethrow_if_nested(e);
            }
            catch (beast::system_error &child)
            {
                process(os, child, level + 1);
            }
            catch (std::exception &child)
            {
                process(os, child, level + 1);
            }
            catch (...)
            {
                emit(os, "nonstandard exception", level + 1);
            }
        }

        static void
        process(std::ostream &os, std::exception_ptr ep, std::size_t level = 0)
        {
            try
            {
                std::rethrow_exception(ep);
            }
            catch (beast::system_error &child)
            {
                process(os, child, level);
            }
            catch (std::exception &child)
            {
                process(os, child, level);
            }
            catch (...)
            {
                emit(os, "nonstandard exception", level + 1);
            }
        }

        friend auto
        operator<<(std::ostream &os, explainer const &ex) -> std::ostream &
        {
            ex.process(os, ex.ep_);

            return os;
        }
    };

    inline auto
    explain(std::exception_ptr ep = std::current_exception()) -> explainer
    {
        return explainer{ep};
    }
}
