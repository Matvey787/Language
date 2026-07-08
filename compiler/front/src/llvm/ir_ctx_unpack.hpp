#pragma once

#define UNPACK_CTX_M(ctx)                                                      \
    auto&& table   = (ctx).t_;                                                 \
    auto&& builder = (ctx).b_;                                                 \
    auto&& module  = (ctx).m_;
