#ifndef LAYOUT_ENGINE_H
#define LAYOUT_ENGINE_H

#include <string>
#include <string_view>
#include <vector>

namespace pe::layout {

// 레이아웃 계산에 필요한 프린터 정보
struct LayoutConfig {
    int printWidthDots = 576;        // 전체 인쇄 가능 가로 폭
    int paddingLeftDots = 24;        // 왼쪽 여백
    int paddingRightDots = 24;       // 오른쪽 여백
    int asciiCharWidthDots = 12;     // ASCII 한 글자의 대략적인 폭
    int minColumnGapDots = 24;       // 컬럼 사이 최소 간격

    int contentWidthDots() const {
        return printWidthDots - paddingLeftDots - paddingRightDots;
    }
};

// 한 줄 안에서 출력될 텍스트 하나
struct PositionedText {
    int xDots = 0;      // 왼쪽으로부터 시작 위치
    std::string text;   // 출력할 문자열
};

// 한 줄에 여러 텍스트가 들어갈 수 있음
using LayoutLine = std::vector<PositionedText>;


// 문자열의 실제 출력 폭 계산
int textWidthDots(
    std::string_view text,
    const LayoutConfig& config
);


// 지정된 폭을 넘어가면 문자열 줄바꿈
std::vector<std::string> wrapTextByDots(
    std::string_view text,
    int maxWidthDots,
    const LayoutConfig& config
);


// 2열 레이아웃
std::vector<LayoutLine> twoColumns(
    std::string_view left,
    std::string_view right,
    const LayoutConfig& config
);


// 3열 레이아웃
std::vector<LayoutLine> threeColumns(
    std::string_view left,
    std::string_view middle,
    std::string_view right,
    const LayoutConfig& config
);

} // namespace pe::layout

#endif