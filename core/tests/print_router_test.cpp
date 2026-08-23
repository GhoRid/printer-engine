#include "print_router.h"

#include <cassert>

int main()
{
    assert(routePrintRequest(
        "/print/receipt",
        R"({"name":"홍길동","amount":10000})",
        nullptr
    ).statusCode == 503);

    assert(routePrintRequest(
        "/print/access-pass",
        R"({"name":"홍길동","qrValue":"ABC123"})",
        nullptr
    ).statusCode == 503);

    assert(routePrintRequest(
        "/print/receipt",
        R"({"name":"홍길동","amount":0})",
        nullptr
    ).statusCode == 400);

    assert(routePrintRequest(
        "/print/access-pass",
        R"({"name":"홍길동"})",
        nullptr
    ).statusCode == 400);

    assert(routePrintRequest(
        "/print/receipt",
        R"({"name":"홍\n길동","amount":10000})",
        nullptr
    ).statusCode == 400);

    assert(routePrintRequest(
        "/print/unknown",
        "not json",
        nullptr
    ).statusCode == 404);
}
