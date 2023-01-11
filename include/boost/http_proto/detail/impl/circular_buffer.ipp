//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_DETAIL_IMPL_CIRCULAR_BUFFER_IPP
#define BOOST_HTTP_PROTO_DETAIL_IMPL_CIRCULAR_BUFFER_IPP

#include <boost/http_proto/detail/circular_buffer.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace http_proto {
namespace detail {

circular_buffer::
circular_buffer(
    void* base,
    std::size_t capacity) noexcept
    : base_(reinterpret_cast<
        unsigned char*>(base))
    , cap_(capacity)
{
}

bool
circular_buffer::
empty() const noexcept
{
    return in_len_ == 0;
}

auto
circular_buffer::
data() const noexcept ->
    const_buffers_pair
{
    if(in_pos_ + in_len_ <= cap_)
        return {
            const_buffer{
                base_ + in_pos_, in_len_ },
            const_buffer{ base_, 0} };
    return {
        const_buffer{
            base_ + in_pos_, cap_ - in_pos_},
        const_buffer{
            base_, in_len_- (cap_ - in_pos_)}};
}

auto
circular_buffer::
prepare() noexcept ->
    mutable_buffers_pair
{
    auto const n = cap_ - in_len_;
    auto const out_pos =
        (in_pos_ + in_len_) % cap_;
    if(out_pos + n <= cap_)
        return {
            mutable_buffer{
                base_ + out_pos, n},
            mutable_buffer{base_,
                n - (cap_ - out_pos)} };
    return {
        mutable_buffer{
            base_ + out_pos, cap_ - out_pos},
        mutable_buffer{
            base_, n - (cap_ - out_pos)}};
}

void
circular_buffer::
commit(std::size_t n) noexcept
{
    BOOST_ASSERT(n <= cap_ - in_len_);
    in_len_ += n;
}

void
circular_buffer::
consume(std::size_t n) noexcept
{
    if(n < in_len_)
    {
        in_pos_ = (in_pos_ + n) % cap_;
        in_len_ -= n;
    }
    else
    {
        // make prepare return a
        // bigger single buffer
        in_pos_ = 0;
        in_len_ = 0;
    }
}

} // detail
} // http_proto
} // boost

#endif
