// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/parser/CSSSelectorParser.h"

#include "core/css/CSSSelectorList.h"
#include "core/css/parser/CSSTokenizer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

typedef struct {
    const char* input;
    const int a;
    const int b;
} ANPlusBTestCase;

TEST(CSSSelectorParserTest, ValidANPlusB)
{
    ANPlusBTestCase testCases[] = {
        {"odd", 2, 1},
        {"OdD", 2, 1},
        {"even", 2, 0},
        {"EveN", 2, 0},
        {"0", 0, 0},
        {"8", 0, 8},
        {"+12", 0, 12},
        {"-14", 0, -14},

        {"0n", 0, 0},
        {"16N", 16, 0},
        {"-19n", -19, 0},
        {"+23n", 23, 0},
        {"n", 1, 0},
        {"N", 1, 0},
        {"+n", 1, 0},
        {"-n", -1, 0},
        {"-N", -1, 0},

        {"6n-3", 6, -3},
        {"-26N-33", -26, -33},
        {"n-18", 1, -18},
        {"+N-5", 1, -5},
        {"-n-7", -1, -7},

        {"0n+0", 0, 0},
        {"10n+5", 10, 5},
        {"10N +5", 10, 5},
        {"10n -5", 10, -5},
        {"N+6", 1, 6},
        {"n +6", 1, 6},
        {"+n -7", 1, -7},
        {"-N -8", -1, -8},
        {"-n+9", -1, 9},

        {"33N- 22", 33, -22},
        {"+n- 25", 1, -25},
        {"N- 46", 1, -46},
        {"n- 0", 1, 0},
        {"-N- 951", -1, -951},
        {"-n- 951", -1, -951},

        {"29N + 77", 29, 77},
        {"29n - 77", 29, -77},
        {"+n + 61", 1, 61},
        {"+N - 63", 1, -63},
        {"+n/**/- 48", 1, -48},
        {"-n + 81", -1, 81},
        {"-N - 88", -1, -88},
    };

    for (auto testCase : testCases) {
        SCOPED_TRACE(testCase.input);

        std::pair<int, int> ab;
        CSSTokenizer::Scope scope(testCase.input);
        CSSParserTokenRange range = scope.tokenRange();
        bool passed = CSSSelectorParser::consumeANPlusB(range, ab);
        EXPECT_TRUE(passed);
        EXPECT_EQ(ab.first, testCase.a);
        EXPECT_EQ(ab.second, testCase.b);
    }
}

TEST(CSSSelectorParserTest, InvalidANPlusB)
{
    // Some of these have token range prefixes which are valid <an+b> and could
    // in theory be valid in consumeANPlusB, but this behaviour isn't needed
    // anywhere and not implemented.
    const char* testCases[] = {
        " odd",
        "+ n",
        "3m+4",
        "12n--34",
        "12n- -34",
        "12n- +34",
        "23n-+43",
        "10n 5",
        "10n + +5",
        "10n + -5",
    };

    for (auto testCase : testCases) {
        SCOPED_TRACE(testCase);

        std::pair<int, int> ab;
        CSSTokenizer::Scope scope(testCase);
        CSSParserTokenRange range = scope.tokenRange();
        bool passed = CSSSelectorParser::consumeANPlusB(range, ab);
        EXPECT_FALSE(passed);
    }
}

TEST(CSSSelectorParserTest, ShadowDomPseudoInCompound)
{
    const char* testCases[][2] = {
        { "::shadow", "::shadow" },
        { ".a::shadow", ".a::shadow" },
        { "::content", "::content" },
        { ".a::content", ".a::content" },
    };

    for (auto testCase : testCases) {
        SCOPED_TRACE(testCase[0]);
        CSSTokenizer::Scope scope(testCase[0]);
        CSSParserTokenRange range = scope.tokenRange();
        CSSSelectorList list = CSSSelectorParser::parseSelector(range, CSSParserContext(HTMLStandardMode, nullptr), nullptr);
        EXPECT_STREQ(testCase[1], list.selectorsText().ascii().data());
    }
}

TEST(CSSSelectorParserTest, PseudoElementsInCompoundLists)
{
    const char* testCases[] = {
        ":not(::before)",
        ":not(::content)",
        ":not(::shadow)",
        ":host(::before)",
        ":host(::content)",
        ":host(::shadow)",
        ":host-context(::before)",
        ":host-context(::content)",
        ":host-context(::shadow)",
        ":-webkit-any(::after, ::before)",
        ":-webkit-any(::content, span)",
        ":-webkit-any(div, ::shadow)"
    };

    for (auto testCase : testCases) {
        CSSTokenizer::Scope scope(testCase);
        CSSParserTokenRange range = scope.tokenRange();
        CSSSelectorList list = CSSSelectorParser::parseSelector(range, CSSParserContext(HTMLStandardMode, nullptr), nullptr);
        EXPECT_FALSE(list.isValid());
    }
}

TEST(CSSSelectorParserTest, ValidSimpleAfterPseudoElementInCompound)
{
    const char* testCases[] = {
        "::-webkit-volume-slider:hover",
        "::selection:window-inactive",
        "::-webkit-scrollbar:disabled",
        "::-webkit-volume-slider:not(:hover)",
        "::-webkit-scrollbar:not(:horizontal)"
    };

    for (auto testCase : testCases) {
        CSSTokenizer::Scope scope(testCase);
        CSSParserTokenRange range = scope.tokenRange();
        CSSSelectorList list = CSSSelectorParser::parseSelector(range, CSSParserContext(HTMLStandardMode, nullptr), nullptr);
        EXPECT_TRUE(list.isValid());
    }
}

TEST(CSSSelectorParserTest, InvalidSimpleAfterPseudoElementInCompound)
{
    const char* testCases[] = {
        "::before#id",
        "::after:hover",
        ".class::content::before",
        "::shadow.class",
        "::selection:window-inactive::before",
        "::-webkit-volume-slider.class",
        "::content.a",
        "::content.a.b",
        ".a::content.b",
        "::before:not(.a)",
        "::shadow:not(::after)",
        "::content:not(#id)",
        "::-webkit-scrollbar:vertical:not(:first-child)",
        "video::-webkit-media-text-track-region-container.scrolling",
        "div ::before.a"
    };

    for (auto testCase : testCases) {
        CSSTokenizer::Scope scope(testCase);
        CSSParserTokenRange range = scope.tokenRange();
        CSSSelectorList list = CSSSelectorParser::parseSelector(range, CSSParserContext(HTMLStandardMode, nullptr), nullptr);
        EXPECT_FALSE(list.isValid());
    }
}

TEST(CSSSelectorParserTest, WorkaroundForInvalidCustomPseudoInUAStyle)
{
    // See crbug.com/578131
    CSSTokenizer::Scope scope("video::-webkit-media-text-track-region-container.scrolling");
    CSSParserTokenRange range = scope.tokenRange();
    CSSSelectorList list = CSSSelectorParser::parseSelector(range, CSSParserContext(UASheetMode, nullptr), nullptr);
    EXPECT_TRUE(list.isValid());
}

} // namespace
