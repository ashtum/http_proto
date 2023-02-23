//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/http_proto
//

// Test that header file is self-contained.
#include <boost/http_proto/request_parser.hpp>

#include <boost/http_proto/context.hpp>
#include <boost/http_proto/rfc/combine_field_values.hpp>

#include "test_suite.hpp"

#include <algorithm>
#include <iostream>
#include <string>

namespace boost {
namespace http_proto {

struct request_parser_test
{
    bool
    feed(
        parser& p,
        string_view s)
    {
        while(! s.empty())
        {
            auto b = *p.prepare().begin();
            auto n = b.size();
            if( n > s.size())
                n = s.size();
            std::memcpy(b.data(),
                s.data(), n);
            p.commit(n);
            error_code ec;
            p.parse(ec);
            s.remove_prefix(n);
            if(ec == error::end_of_message
                || ! ec)
                break;
            if(! BOOST_TEST(
                ec == grammar::error::need_more))
            {
                test_suite::log <<
                    ec.message() << "\n";
                return false;
            }
        }
        return true;
    }

    bool
    valid(
        context& ctx,
        string_view s,
        std::size_t nmax)
    {
        request_parser p(ctx);
        p.start();
        while(! s.empty())
        {
            auto b = *p.prepare().begin();
            auto n = b.size();
            if( n > s.size())
                n = s.size();
            if( n > nmax)
                n = nmax;
            std::memcpy(b.data(),
                s.data(), n);
            p.commit(n);
            error_code ec;
            p.parse(ec);
            s.remove_prefix(n);
            if(ec == grammar::error::need_more)
                continue;
            auto const es = ec.message();
            return ! ec.failed();
        }
        return false;
    }

    void
    good(context& ctx, string_view s)
    {
        for(std::size_t nmax = 1;
            nmax < s.size(); ++nmax)
            BOOST_TEST(valid(ctx, s, nmax));
    }

    void
    bad(context& ctx, string_view s)
    {
        for(std::size_t nmax = 1;
            nmax < s.size(); ++nmax)
            BOOST_TEST(! valid(ctx, s, nmax));
    }

    void
    check(
        method m,
        string_view t,
        version v,
        string_view const s)
    {
        auto const f =
            [&](request_parser const& p)
        {
            auto const req = p.get();
            BOOST_TEST(req.method() == m);
            BOOST_TEST(req.method_text() ==
                to_string(m));
            BOOST_TEST(req.target_text() == t);
            BOOST_TEST(req.version() == v);
        };

        context ctx;
        request_parser::config cfg;
        install_parser_service(ctx, cfg);

        // single buffer
        {
            request_parser p(ctx);
            p.start();
            auto const b = *p.prepare().begin();
            auto const n = (std::min)(
                b.size(), s.size());
            BOOST_TEST(n == s.size());
            std::memcpy(
                b.data(), s.data(), n);
            p.commit(n);
            error_code ec;
            p.parse(ec);
            BOOST_TEST(! ec);
            //BOOST_TEST(p.is_done());
            if(! ec)
                f(p);
        }

        // two buffers
        for(std::size_t i = 1;
            i < s.size(); ++i)
        {
            request_parser p(ctx);
            p.start();
            // first buffer
            auto b = *p.prepare().begin();
            auto n = (std::min)(
                b.size(), i);
            BOOST_TEST(n == i);
            std::memcpy(
                b.data(), s.data(), n);
            p.commit(n);
            error_code ec;
            p.parse(ec);
            if(! BOOST_TEST(
                ec == grammar::error::need_more))
                continue;
            // second buffer
            b = *p.prepare().begin();
            n = (std::min)(
                b.size(), s.size());
            BOOST_TEST(n == s.size());
            std::memcpy(
                b.data(), s.data() + i, n - i);
            p.commit(n);
            p.parse(ec);
            if(ec.failed())
                continue;
            //BOOST_TEST(p.is_done());
            f(p);
        }
    }

    //--------------------------------------------

    void
    testSpecial()
    {
        // request_parser()
        {
            context ctx;
            request_parser::config cfg;
            install_parser_service(ctx, cfg);
            request_parser pr(ctx);
        }
    }

    //--------------------------------------------

    void
    testParse()
    {
        check(method::get, "/",
            version::http_1_1,
            "GET / HTTP/1.1\r\n"
            "Connection: close\r\n"
            "Content-Length: 42\r\n"
            "\r\n");
    }

    void
    testParseField()
    {
        auto f = [](string_view f)
        {
            return std::string(
                "GET / HTTP/1.1\r\n") +
                std::string(
                    f.data(), f.size()) +
                "\r\n\r\n";
        };

        context ctx;
        request_parser::config cfg;
        install_parser_service(ctx, cfg);

        bad(ctx, f(":"));
        bad(ctx, f(" :"));
        bad(ctx, f(" x:"));
        bad(ctx, f("x :"));
        bad(ctx, f("x@"));
        bad(ctx, f("x@:"));

        good(ctx, f(""));
        good(ctx, f("x:"));
        good(ctx, f("x: "));
        good(ctx, f("x:\t "));
        good(ctx, f("x:y"));
        good(ctx, f("x: y"));
        good(ctx, f("x:y "));
        good(ctx, f("x: y "));
        good(ctx, f("x:yy"));
        good(ctx, f("x: yy"));
        good(ctx, f("x:yy "));
        good(ctx, f("x: y y "));
        good(ctx, f("x:"));
        good(ctx, f("x: \r\n "));
        good(ctx, f("x: \r\n x"));
        good(ctx, f("x: \r\n \t\r\n "));
        good(ctx, f("x: \r\n \t\r\n x"));
        good(ctx, f("x: y \r\n "));

        // errata eid4189
        good(ctx, f("x: , , ,"));
        good(ctx, f("x: abrowser/0.001 (C O M M E N T)"));
        good(ctx, f("x: gzip , chunked"));
    }

    void
    testGet()
    {
        context ctx;
        request_parser::config cfg;
        install_parser_service(ctx, cfg);
        request_parser p(ctx);
        string_view s = 
            "GET / HTTP/1.1\r\n"
            "User-Agent: x\r\n"
            "Connection: close\r\n"
            "Transfer-Encoding: chunked\r\n"
            "a: 1\r\n"
            "b: 2\r\n"
            "a: 3\r\n"
            "c: 4\r\n"
            "\r\n";

        p.start();
        feed(p, s);

        auto const rv = p.get();
        BOOST_TEST(
            rv.method() == method::get);
        BOOST_TEST(
            rv.method_text() == "GET");
        BOOST_TEST(rv.target_text() == "/");
        BOOST_TEST(rv.version() ==
            version::http_1_1);

        BOOST_TEST(rv.buffer() == s);
        BOOST_TEST(rv.size() == 7);
        BOOST_TEST(
            rv.exists(field::connection));
        BOOST_TEST(! rv.exists(field::age));
        BOOST_TEST(rv.exists("Connection"));
        BOOST_TEST(rv.exists("CONNECTION"));
        BOOST_TEST(! rv.exists("connector"));
        BOOST_TEST(rv.count(
            field::transfer_encoding) == 1);
        BOOST_TEST(
            rv.count(field::age) == 0);
        BOOST_TEST(
            rv.count("connection") == 1);
        BOOST_TEST(rv.count("a") == 2);
        BOOST_TEST(rv.find(
            field::connection)->id ==
                field::connection);
        BOOST_TEST(
            rv.find("a")->value == "1");
        grammar::recycled_ptr<std::string> temp;
        BOOST_TEST(combine_field_values(rv.find_all(
            field::user_agent), temp) == "x");
        BOOST_TEST(combine_field_values(rv.find_all(
            "b"), temp) == "2");
        BOOST_TEST(combine_field_values(rv.find_all(
            "a"), temp) == "1,3");
    }

    void
    run()
    {
        testSpecial();
        testParse();
        testParseField();
        testGet();
    }
};

TEST_SUITE(
    request_parser_test,
    "boost.http_proto.request_parser");

} // http_proto
} // boost


