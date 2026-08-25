#include "layout_engine.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

namespace pe::layout {

namespace {

// UTF-8 문자 하나의 정보
struct Utf8CodePoint {
    char32_t value = U'\0';
    std::size_t length = 1;
};

// UTF-8 continuation byte인지 확인
bool isContinuationByte(unsigned char byte) {
    return (byte & 0xC0) == 0x80;
}

// UTF-8 문자열에서 문자 하나 읽기
Utf8CodePoint decodeUtf8(std::string_view text, std::size_t offset) {
    const auto first = static_cast<unsigned char>(text[offset]);

    // ASCII
    if (first < 0x80) {
        return {static_cast<char32_t>(first), 1};
    }

    // 2 byte UTF-8
    if ((first & 0xE0) == 0xC0 && offset + 1 < text.size()) {
        const auto b1 = static_cast<unsigned char>(text[offset + 1]);

        if (isContinuationByte(b1)) {
            const char32_t codePoint =
                ((first & 0x1F) << 6) |
                (b1 & 0x3F);

            if (codePoint >= 0x80) {
                return {codePoint, 2};
            }
        }
    }

    // 3 byte UTF-8
    if ((first & 0xF0) == 0xE0 && offset + 2 < text.size()) {
        const auto b1 = static_cast<unsigned char>(text[offset + 1]);
        const auto b2 = static_cast<unsigned char>(text[offset + 2]);

        if (isContinuationByte(b1) && isContinuationByte(b2)) {
            const char32_t codePoint =
                ((first & 0x0F) << 12) |
                ((b1 & 0x3F) << 6) |
                (b2 & 0x3F);

            if (codePoint >= 0x800 &&
                !(codePoint >= 0xD800 && codePoint <= 0xDFFF)) {
                return {codePoint, 3};
            }
        }
    }

    // 4 byte UTF-8
    if ((first & 0xF8) == 0xF0 && offset + 3 < text.size()) {
        const auto b1 = static_cast<unsigned char>(text[offset + 1]);
        const auto b2 = static_cast<unsigned char>(text[offset + 2]);
        const auto b3 = static_cast<unsigned char>(text[offset + 3]);

        if (isContinuationByte(b1) &&
            isContinuationByte(b2) &&
            isContinuationByte(b3)) {
            const char32_t codePoint =
                ((first & 0x07) << 18) |
                ((b1 & 0x3F) << 12) |
                ((b2 & 0x3F) << 6) |
                (b3 & 0x3F);

            if (codePoint >= 0x10000 && codePoint <= 0x10FFFF) {
                return {codePoint, 4};
            }
        }
    }

    // 잘못된 UTF-8이면 그냥 1 byte 문자로 처리
    return {static_cast<char32_t>(first), 1};
}

// 조합 문자인지 확인
bool isCombining(char32_t codePoint) {
    return
        (codePoint >= 0x0300 && codePoint <= 0x036F) ||
        (codePoint >= 0x1AB0 && codePoint <= 0x1AFF) ||
        (codePoint >= 0x1DC0 && codePoint <= 0x1DFF) ||
        (codePoint >= 0x20D0 && codePoint <= 0x20FF) ||
        (codePoint >= 0xFE20 && codePoint <= 0xFE2F);
}

// 한글 / 일본어 / 한자 등 ASCII보다 2배 폭으로 볼 문자
bool isWide(char32_t codePoint) {
    return
        // 한글 자모
        (codePoint >= 0x1100 && codePoint <= 0x11FF) ||
        // 한글 호환 자모
        (codePoint >= 0x3130 && codePoint <= 0x318F) ||
        // 완성형 한글
        (codePoint >= 0xAC00 && codePoint <= 0xD7A3) ||
        // CJK 기호
        (codePoint >= 0x3000 && codePoint <= 0x303F) ||
        // 일본어
        (codePoint >= 0x3040 && codePoint <= 0x30FF) ||
        // 한자
        (codePoint >= 0x3400 && codePoint <= 0x9FFF) ||
        // CJK 호환 한자
        (codePoint >= 0xF900 && codePoint <= 0xFAFF) ||
        // 전각 문자
        (codePoint >= 0xFF01 && codePoint <= 0xFF60) ||
        (codePoint >= 0xFFE0 && codePoint <= 0xFFE6);
}

// 문자 하나가 차지하는 dot 수
int codePointWidthDots(char32_t codePoint, const LayoutConfig& config) {
    const int asciiWidth = std::max(1, config.asciiCharWidthDots);

    // 조합 문자는 자체 폭 없음
    if (isCombining(codePoint)) {
        return 0;
    }

    if (isWide(codePoint)) {
        return asciiWidth * 2;
    }

    return asciiWidth;
}

// 줄바꿈 문자를 한 줄용 문자열로 변환
std::string normalizeInlineText(std::string_view text) {
    std::string result;
    result.reserve(text.size());

    for (char ch : text) {
        if (ch == '\r' || ch == '\n' || ch == '\t') {
            result.push_back(' ');
        } else {
            result.push_back(ch);
        }
    }

    // 앞뒤 공백 제거
    const auto begin = result.find_first_not_of(" \t");

    if (begin == std::string::npos) {
        return {};
    }

    const auto end = result.find_last_not_of(" \t");

    return result.substr(begin, end - begin + 1);
}

// 실제 한 줄에 들어가지 않을 때 각 컬럼 폭 자동 분배
std::vector<int> allocateResponsiveWidthsDots(
    const std::vector<int>& naturalWidths,
    int availableDots,
    int minimumWidthDots
) {
    std::vector<int> result(naturalWidths.size(), 0);
    std::vector<std::size_t> active;

    int remaining = std::max(0, availableDots);

    // 실제 내용이 있는 컬럼만 계산
    for (std::size_t i = 0; i < naturalWidths.size(); ++i) {
        if (naturalWidths[i] > 0) {
            active.push_back(i);
        }
    }

    while (!active.empty()) {
        const int share = remaining / static_cast<int>(active.size());
        bool fixedAny = false;

        // 짧은 컬럼은 자연 폭 그대로 확정
        for (std::size_t i = active.size(); i-- > 0;) {
            const std::size_t index = active[i];

            if (naturalWidths[index] <= share) {
                result[index] = naturalWidths[index];
                remaining -= naturalWidths[index];

                active.erase(active.begin() + i);
                fixedAny = true;
            }
        }

        if (fixedAny) {
            continue;
        }

        // 남은 컬럼들이 전부 길면 남은 공간 균등 배분
        const int count = static_cast<int>(active.size());
        const int baseWidth = count > 0 ? remaining / count : 0;
        const int remainder = count > 0 ? remaining % count : 0;

        for (std::size_t i = 0; i < active.size(); ++i) {
            const std::size_t index = active[i];

            result[index] = std::max(
                minimumWidthDots,
                baseWidth + (static_cast<int>(i) < remainder ? 1 : 0)
            );
        }

        active.clear();
    }

    return result;
}

// 비어있지 않은 텍스트만 LayoutLine에 추가
void appendIfNotEmpty(
    LayoutLine& line,
    int xDots,
    const std::string& text
) {
    if (!text.empty()) {
        line.push_back({xDots, text});
    }
}

} // namespace

int textWidthDots(
    std::string_view text,
    const LayoutConfig& config
) {
    int width = 0;

    for (std::size_t offset = 0; offset < text.size();) {
        const Utf8CodePoint decoded = decodeUtf8(text, offset);

        width += codePointWidthDots(decoded.value, config);
        offset += decoded.length;
    }

    return width;
}

std::vector<std::string> wrapTextByDots(
    std::string_view text,
    int maxWidthDots,
    const LayoutConfig& config
) {
    const std::string normalized = normalizeInlineText(text);
    const int minimumWidth = std::max(1, config.asciiCharWidthDots);

    maxWidthDots = std::max(minimumWidth, maxWidthDots);

    if (normalized.empty()) {
        return {""};
    }

    std::vector<std::string> lines;
    std::string current;

    int currentWidth = 0;

    for (std::size_t offset = 0; offset < normalized.size();) {
        const Utf8CodePoint decoded = decodeUtf8(normalized, offset);
        const int charWidth = codePointWidthDots(decoded.value, config);

        // 이번 문자를 넣으면 최대 폭을 넘어가는 경우
        if (!current.empty() &&
            currentWidth + charWidth > maxWidthDots) {
            lines.push_back(current);
            current.clear();
            currentWidth = 0;
        }

        // UTF-8 문자 전체 byte를 추가
        current.append(normalized, offset, decoded.length);

        currentWidth += charWidth;
        offset += decoded.length;
    }

    if (!current.empty()) {
        lines.push_back(current);
    }

    return lines;
}

std::vector<LayoutLine> alignedText(
    std::string_view text,
    TextAlignment alignment,
    const LayoutConfig& config
) {
    if (config.textWidthColumns > 0) {
        const int columns = config.textWidthColumns;
        const int asciiWidth = std::max(1, config.asciiCharWidthDots);
        const auto wrapped = wrapTextByDots(text, columns * asciiWidth, config);
        std::vector<LayoutLine> lines;
        lines.reserve(wrapped.size());

        for (const auto& part : wrapped) {
            const int partColumns = textWidthDots(part, config) / asciiWidth;
            int leadingSpaces = 0;
            if (alignment == TextAlignment::Center) {
                leadingSpaces = std::max(0, (columns - partColumns) / 2);
            } else if (alignment == TextAlignment::Right) {
                leadingSpaces = std::max(0, columns - partColumns);
            }

            LayoutLine line;
            appendIfNotEmpty(
                line,
                0,
                std::string(static_cast<std::size_t>(leadingSpaces), ' ') + part
            );
            lines.push_back(std::move(line));
        }

        return lines;
    }

    const int contentWidth = std::max(1, config.contentWidthDots());
    const auto wrapped = wrapTextByDots(text, contentWidth, config);
    std::vector<LayoutLine> lines;
    lines.reserve(wrapped.size());

    for (const auto& part : wrapped) {
        const int width = textWidthDots(part, config);
        int x = 0;
        if (alignment == TextAlignment::Center) {
            x = std::max(0, (contentWidth - width) / 2);
        } else if (alignment == TextAlignment::Right) {
            x = std::max(0, contentWidth - width);
        }
        LayoutLine line;
        appendIfNotEmpty(line, x, part);
        lines.push_back(std::move(line));
    }
    return lines;
}

std::vector<LayoutLine> twoColumns(
    std::string_view left,
    std::string_view right,
    const LayoutConfig& config
) {
    const std::string leftText = normalizeInlineText(left);
    const std::string rightText = normalizeInlineText(right);

    if (config.textWidthColumns > 0) {
        const int columns = config.textWidthColumns;
        const int asciiWidth = std::max(1, config.asciiCharWidthDots);
        const int leftColumns = textWidthDots(leftText, config) / asciiWidth;
        const int rightColumns = textWidthDots(rightText, config) / asciiWidth;
        const int minGapColumns = std::max(
            1,
            (std::max(0, config.minColumnGapDots) + asciiWidth - 1) / asciiWidth
        );

        const auto makeLine = [&](const std::string& leftPart,
                                  const std::string& rightPart) {
            const int leftPartColumns = textWidthDots(leftPart, config) / asciiWidth;
            const int rightPartColumns = textWidthDots(rightPart, config) / asciiWidth;
            std::string value = leftPart;
            if (!rightPart.empty()) {
                value.append(
                    static_cast<std::size_t>(std::max(
                        0,
                        columns - leftPartColumns - rightPartColumns
                    )),
                    ' '
                );
                value += rightPart;
            }
            LayoutLine line;
            appendIfNotEmpty(line, 0, value);
            return line;
        };

        if (leftColumns + minGapColumns + rightColumns <= columns) {
            return {makeLine(leftText, rightText)};
        }

        if (leftText.empty() || rightText.empty() || columns < 2) {
            return leftText.empty()
                ? alignedText(rightText, TextAlignment::Right, config)
                : alignedText(leftText, TextAlignment::Left, config);
        }

        const int actualGap = std::min(minGapColumns, std::max(0, columns - 2));
        const auto widths = allocateResponsiveWidthsDots(
            {leftColumns, rightColumns},
            columns - actualGap,
            1
        );
        const auto leftLines = wrapTextByDots(
            leftText,
            widths[0] * asciiWidth,
            config
        );
        const auto rightLines = wrapTextByDots(
            rightText,
            widths[1] * asciiWidth,
            config
        );
        const std::size_t lineCount = std::max(leftLines.size(), rightLines.size());
        std::vector<LayoutLine> lines;
        lines.reserve(lineCount);
        for (std::size_t i = 0; i < lineCount; ++i) {
            lines.push_back(makeLine(
                i < leftLines.size() ? leftLines[i] : std::string{},
                i < rightLines.size() ? rightLines[i] : std::string{}
            ));
        }
        return lines;
    }

    const int printWidth = std::max(1, config.contentWidthDots());
    const int minGap = std::max(0, config.minColumnGapDots);
    const int minimumColumnWidth = std::max(1, config.asciiCharWidthDots);

    const int leftWidth = textWidthDots(leftText, config);
    const int rightWidth = textWidthDots(rightText, config);

    const int requiredGap =
        (!leftText.empty() && !rightText.empty()) ? minGap : 0;

    // 한 줄에 전부 들어가는 경우
    if (leftWidth + requiredGap + rightWidth <= printWidth) {
        LayoutLine line;

        appendIfNotEmpty(line, 0, leftText);
        appendIfNotEmpty(
            line,
            std::max(0, printWidth - rightWidth),
            rightText
        );

        return {line};
    }

    // 한쪽만 존재하는 경우
    if (leftText.empty() || rightText.empty()) {
        const std::string& onlyText =
            leftText.empty() ? rightText : leftText;

        const auto wrapped = wrapTextByDots(
            onlyText,
            printWidth,
            config
        );

        std::vector<LayoutLine> lines;
        lines.reserve(wrapped.size());

        for (const auto& part : wrapped) {
            LayoutLine line;
            const int x = leftText.empty()
                ? std::max(0, printWidth - textWidthDots(part, config))
                : 0;
            appendIfNotEmpty(line, x, part);
            lines.push_back(std::move(line));
        }

        return lines;
    }

    // 실제로 한 줄에 안 들어가는 경우
    if (printWidth < minimumColumnWidth * 2) {
        auto lines = alignedText(leftText, TextAlignment::Left, config);
        auto rightLines = alignedText(rightText, TextAlignment::Right, config);
        lines.insert(lines.end(), rightLines.begin(), rightLines.end());
        return lines;
    }

    const int actualGap = std::min(
        minGap,
        std::max(0, printWidth - (minimumColumnWidth * 2))
    );
    const int availableDots = std::max(0, printWidth - actualGap);

    const auto widths = allocateResponsiveWidthsDots(
        {leftWidth, rightWidth},
        availableDots,
        minimumColumnWidth
    );

    const int leftColumnWidth = widths[0];
    const int rightColumnWidth = widths[1];
    const int rightX = leftColumnWidth + actualGap;

    const auto leftLines = wrapTextByDots(
        leftText,
        leftColumnWidth,
        config
    );

    const auto rightLines = wrapTextByDots(
        rightText,
        rightColumnWidth,
        config
    );

    const std::size_t lineCount = std::max(
        leftLines.size(),
        rightLines.size()
    );

    std::vector<LayoutLine> lines;
    lines.reserve(lineCount);

    for (std::size_t i = 0; i < lineCount; ++i) {
        LayoutLine line;

        if (i < leftLines.size()) {
            appendIfNotEmpty(line, 0, leftLines[i]);
        }

        if (i < rightLines.size()) {
            const int lineWidth = textWidthDots(rightLines[i], config);
            appendIfNotEmpty(
                line,
                rightX + std::max(0, rightColumnWidth - lineWidth),
                rightLines[i]
            );
        }

        lines.push_back(std::move(line));
    }

    return lines;
}

std::vector<LayoutLine> threeColumns(
    std::string_view left,
    std::string_view middle,
    std::string_view right,
    const LayoutConfig& config
) {
    const std::string leftText = normalizeInlineText(left);
    const std::string middleText = normalizeInlineText(middle);
    const std::string rightText = normalizeInlineText(right);

    // 가운데 값이 없으면 2열 레이아웃 사용
    if (middleText.empty()) {
        return twoColumns(leftText, rightText, config);
    }

    const int printWidth = std::max(1, config.contentWidthDots());
    const int minGap = std::max(0, config.minColumnGapDots);
    const int minimumColumnWidth = std::max(1, config.asciiCharWidthDots);

    const int leftWidth = textWidthDots(leftText, config);
    const int middleWidth = textWidthDots(middleText, config);
    const int rightWidth = textWidthDots(rightText, config);

    const int requiredWidth =
        leftWidth +
        middleWidth +
        rightWidth +
        (minGap * 2);

    // 3개 모두 한 줄에 들어가는 경우
    if (requiredWidth <= printWidth) {
        const int remaining =
            printWidth -
            leftWidth -
            middleWidth -
            rightWidth;

        // 남은 공간을 왼쪽 gap / 오른쪽 gap으로 나눔
        const int gap1 = remaining / 2;
        const int gap2 = remaining - gap1;

        const int middleX = leftWidth + gap1;
        const int rightX = middleX + middleWidth + gap2;

        LayoutLine line;

        appendIfNotEmpty(line, 0, leftText);
        appendIfNotEmpty(line, middleX, middleText);
        appendIfNotEmpty(line, rightX, rightText);

        return {line};
    }

    // 실제로 한 줄에 안 들어가는 경우
    const int activeColumnCount =
        (leftText.empty() ? 0 : 1) + 1 + (rightText.empty() ? 0 : 1);
    if (printWidth < minimumColumnWidth * activeColumnCount) {
        std::vector<LayoutLine> lines;
        const auto append = [&lines](std::vector<LayoutLine> source) {
            lines.insert(
                lines.end(),
                std::make_move_iterator(source.begin()),
                std::make_move_iterator(source.end())
            );
        };
        if (!leftText.empty()) {
            append(alignedText(leftText, TextAlignment::Left, config));
        }
        append(alignedText(middleText, TextAlignment::Center, config));
        if (!rightText.empty()) {
            append(alignedText(rightText, TextAlignment::Right, config));
        }
        return lines;
    }

    const int gapCount = std::max(0, activeColumnCount - 1);
    const int actualGap = gapCount == 0 ? 0 : std::min(
        minGap,
        std::max(0, printWidth - (minimumColumnWidth * activeColumnCount)) /
            gapCount
    );
    const int availableDots = std::max(0, printWidth - (actualGap * gapCount));

    const auto widths = allocateResponsiveWidthsDots(
        {leftWidth, middleWidth, rightWidth},
        availableDots,
        minimumColumnWidth
    );

    const int leftColumnWidth = widths[0];
    const int middleColumnWidth = widths[1];
    const int rightColumnWidth = widths[2];

    const int leftX = 0;
    const int middleX = leftColumnWidth + (leftText.empty() ? 0 : actualGap);
    const int rightX = middleX + middleColumnWidth +
        (rightText.empty() ? 0 : actualGap);

    const auto leftLines = wrapTextByDots(
        leftText,
        leftColumnWidth,
        config
    );

    const auto middleLines = wrapTextByDots(
        middleText,
        middleColumnWidth,
        config
    );

    const auto rightLines = wrapTextByDots(
        rightText,
        rightColumnWidth,
        config
    );

    const std::size_t lineCount = std::max(
        leftLines.size(),
        std::max(middleLines.size(), rightLines.size())
    );

    std::vector<LayoutLine> lines;
    lines.reserve(lineCount);

    for (std::size_t i = 0; i < lineCount; ++i) {
        LayoutLine line;

        if (i < leftLines.size()) {
            appendIfNotEmpty(line, leftX, leftLines[i]);
        }

        if (i < middleLines.size()) {
            const int lineWidth = textWidthDots(middleLines[i], config);
            appendIfNotEmpty(
                line,
                middleX + std::max(0, (middleColumnWidth - lineWidth) / 2),
                middleLines[i]
            );
        }

        if (i < rightLines.size()) {
            const int lineWidth = textWidthDots(rightLines[i], config);
            appendIfNotEmpty(
                line,
                rightX + std::max(0, rightColumnWidth - lineWidth),
                rightLines[i]
            );
        }

        lines.push_back(std::move(line));
    }

    return lines;
}

} // namespace pe::layout
