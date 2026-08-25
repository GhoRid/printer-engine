#include "layout_engine.h"

#include <cassert>
#include <string>

namespace {

void assertInsideContent(
    const std::vector<pe::layout::LayoutLine>& lines,
    const pe::layout::LayoutConfig& config
)
{
    for (const auto& line : lines) {
        int previousEnd = 0;
        for (const auto& part : line) {
            const int end = part.xDots + pe::layout::textWidthDots(part.text, config);
            assert(part.xDots >= previousEnd);
            assert(end <= config.contentWidthDots());
            previousEnd = end;
        }
    }
}

} // namespace

int main()
{
    pe::layout::LayoutConfig config;
    config.printWidthDots = 120;
    config.paddingLeftDots = 12;
    config.paddingRightDots = 12;
    config.asciiCharWidthDots = 12;
    config.minColumnGapDots = 24;

    assert(config.contentWidthDots() == 96);
    assert(pe::layout::textWidthDots("ABC", config) == 36);
    assert(pe::layout::textWidthDots("한글", config) == 48);

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

    assertInsideContent(right, config);
    assertInsideContent(columns, config);
    assertInsideContent(rightOnly, config);

    const auto centered = pe::layout::alignedText(
        "한글",
        pe::layout::TextAlignment::Center,
        config
    );
    assert(centered[0][0].xDots == 24);
    assertInsideContent(centered, config);

    pe::layout::LayoutConfig narrow;
    narrow.printWidthDots = 36;
    narrow.paddingLeftDots = 6;
    narrow.paddingRightDots = 6;
    narrow.asciiCharWidthDots = 12;
    narrow.minColumnGapDots = 24;
    const auto narrowColumns = pe::layout::twoColumns("A", "B", narrow);
    assert(narrowColumns.size() == 2);
    assertInsideContent(narrowColumns, narrow);

    const auto normalizedTab = pe::layout::alignedText(
        "A\tB",
        pe::layout::TextAlignment::Left,
        config
    );
    assert(normalizedTab[0][0].text == "A B");

    pe::layout::LayoutConfig characterGrid;
    characterGrid.printWidthDots = 144;
    characterGrid.asciiCharWidthDots = 12;
    characterGrid.minColumnGapDots = 24;
    characterGrid.textWidthColumns = 12;

    const auto gridColumns = pe::layout::twoColumns(
        "Issuer",
        "BC",
        characterGrid
    );
    assert(gridColumns.size() == 1);
    assert(gridColumns[0].size() == 1);
    assert(gridColumns[0][0].xDots == 0);
    assert(gridColumns[0][0].text == "Issuer    BC");

    const auto gridRight = pe::layout::alignedText(
        "42",
        pe::layout::TextAlignment::Right,
        characterGrid
    );
    assert(gridRight[0][0].xDots == 0);
    assert(gridRight[0][0].text == "          42");

    return 0;
}
