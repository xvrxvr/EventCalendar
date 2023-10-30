#pragma once

#include "web_gadgets.h"

#define G(id, args) \
class AJAXDecoder_##id : public AnsGet { \
    V##args \
public: \
    AJAXDecoder_##id(httpd_req_t *req) : AnsGet(req) C##args {} \
    void run(); \
};

#define P(id, args) \
class AJAXDecoder_##id : public AnsPost { \
    V##args \
public: \
    AJAXDecoder_##id(httpd_req_t *req) : AnsPost(req) C##args {} \
    void run(); \
};

#define VARG(tp, id) Arg##tp arg_##id;
#define CARG(tp, id) , arg_##id(decode_##tp(#id))

#include "web_actions.inc"

