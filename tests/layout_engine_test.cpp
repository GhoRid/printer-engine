#include "layout_engine.h"

#include <cassert>
#include <string>

int main()
{
    pe::layout::LayoutConfig config;
    config.printWidthDots = 120;
    config.paddingLeftDots = 12;
    config.paddingRightDots = 12;
    config.asciiCharWidthDots = 12;
    config.minColumnGapDots = 24;

    assert(config.contentWidthDots() == 96);

    const auto right = pe::layout::alignedText(
        "123456789",
        pe::layout::TextAlignment::Right,
        config
    );
    assert(right.size() == 2);
    assert(right[0][0].xDots == 0);
    assert(right[0][0].text == "12345678");
    assert(right[1][0].xDots == 84);
    assert(right[1][0].text == "9");

    const auto columns = pe::layout::twoColumns("A", "123456789", config);
    assert(columns.size() == 2);
    assert(columns[0][0].xDots == 0);
    assert(columns[0][1].xDots == 36);
    assert(columns[0][1].text == "12345");
    assert(columns[1][0].xDots == 48);
    assert(columns[1][0].text == "6789");

    const auto rightOnly = pe::layout::twoColumns("", "123456789", config);
    assert(rightOnly.size() == 2);
    assert(rightOnly[0][0].xDots == 0);
    assert(rightOnly[1][0].xDots == 84);

    return 0;
}
