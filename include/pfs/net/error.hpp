////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [net-lib](https://github.com/semenovf/net-lib) library.
//
// Changelog:
//      2021.06.21 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <system_error>

namespace pfs {
namespace net {

////////////////////////////////////////////////////////////////////////////////
// Error codes, category, exception class
////////////////////////////////////////////////////////////////////////////////
using error_code = std::error_code;

enum class errc
{
      success = 0
    , check_errno
    , device_not_found
    , permissions_denied
    , name_too_long
};

class error_category : public std::error_category
{
public:
    virtual char const * name () const noexcept override
    {
        return "net_category";
    }

    virtual std::string message (int ev) const override
    {
        switch (ev) {
            case static_cast<int>(errc::success):
                return std::string{"no error"};
            case static_cast<int>(errc::check_errno):
                return std::string{"system specific error, check errno value"};
            case static_cast<int>(errc::device_not_found):
                return std::string{"device not found"};
            case static_cast<int>(errc::permissions_denied):
                return std::string{"permissions denied"};
            case static_cast<int>(errc::name_too_long):
                return std::string{"name too long"};

            default: return std::string{"unknown net error"};
        }
    }
};

inline std::error_category const & get_error_category ()
{
    static error_category instance;
    return instance;
}

inline std::error_code make_error_code (errc e)
{
    return std::error_code(static_cast<int>(e), get_error_category());
}

inline std::system_error make_exception (errc e)
{
    return std::system_error(make_error_code(e));
}

}} // namespace pfs::net

