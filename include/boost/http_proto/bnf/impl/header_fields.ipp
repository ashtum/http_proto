//
// Copyright (c) 2021 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_BNF_IMPL_HEADER_FIELDS_IPP
#define BOOST_HTTP_PROTO_BNF_IMPL_HEADER_FIELDS_IPP

#include <boost/http_proto/bnf/header_fields.hpp>
#include <boost/http_proto/bnf/ctype.hpp>
#include <boost/http_proto/error.hpp>

namespace boost {
namespace http_proto {
namespace bnf {

char const*
header_fields::
increment(
    char const* start,
    char const* end,
    error_code& ec)
{
    auto it = start;
    char const* k1; // end of name
    char const* v0  // start of value
        = nullptr;
    char const* v1; // end of value

    ws_set ws_;
    tchar_set ts;
    field_vchar_set fvs;

    // [ CRLF ]
    if(it == end)
    {
        ec = error::need_more;
        return start;
    }
    if(*it == '\r')
    {
        ++it;
        if(it == end)
        {
            ec = error::need_more;
            return start;
        }
        if(*it != '\n')
        {
            // expected LF
            ec = error::bad_line_ending;
            return start;
        }
        ++it;
        ec = error::end;
        return it;
    }

    // field-name
    it = ts.skip(it, end);

    // ":"
    if(it == end)
    {
        ec = error::need_more;
        return start;
    }
    if(*it != ':')
    {
        // invalid field char
        ec = error::bad_field_name;
        return start;
    }
    if(it == start)
    {
        // missing field name
        ec = error::bad_field_name;
        return start;
    }
    k1 = it;
    ++it;

    // OWS
    it = ws_.skip(it, end);

    // *( field-content / obs-fold )
    for(;;)
    {
        if(it == end)
        {
            ec = error::need_more;
            return start;
        }

        // check field-content first, as
        // it is more frequent than CRLF
        if(fvs.contains(*it))
        {
            // field-content
            if(! v0)
                v0 = it;
            ++it;
            // *field-vchar
            it = fvs.skip(it, end);
            if(it == end)
            {
                ec = error::need_more;
                return start;
            }
            v1 = it;
            continue;
        }

        // OWS
        if(ws_.contains(*it))
        {
            ++it;
            it = ws_.skip(it, end);
            continue;
        }

        // obs-fold / CRLF
        if(it[0] == '\r')
        {
            if(end - it < 3)
            {
                ec = error::need_more;
                return start;
            }
            if(it[1] != '\n')
            {
                // expected LF
                ec = error::bad_line_ending;
                return start;
            }
            if(! ws_.contains(it[2]))
            {
                // end of line
                if(! v0)
                    v0 = it;
                v1 = it;
                it += 2;
                break;
            }
            v_.has_obs_fold = true;
            /*
            // replace CRLF with SP SP
            it[0] = ' ';
            it[1] = ' ';
            */
            it += 3;
            // *( SP / HTAB )
            it = ws_.skip(it, end);
            continue;
        }

        // illegal value
        ec = error::bad_field_value;
        return start;
    }

    v_.name = string_view(
        start, k1 - start);
    v_.value = string_view(
        v0, v1 - v0);
    return it;
}

void
replace_obs_fold(
    char* it,
    char const* const end) noexcept
{
    ws_set ws_;
    while(it != end)
    {
        if(*it != '\r')
        {
            ++it;
            continue;
        }
        if(end - it < 3)
            break;
        BOOST_ASSERT(it[1] == '\n');
        if( it[1] == '\n' &&
            ws_.contains(it[2]))
        {
            it[0] = ' ';
            it[1] = ' ';
            it += 3;
        }
        else
        {
            ++it;
        }
    }
}

} // bnf
} // http_proto
} // boost

#endif
