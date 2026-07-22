#include "hft/io/parser.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

using namespace hft;

TEST(ParserTest, EmptyLineReturnsNullopt) {
    Parser p;
    EXPECT_EQ(p.parse(""), std::nullopt);
    EXPECT_EQ(p.parse("   "), std::nullopt);
}

TEST(ParserTest, CommentReturnsNullopt) {
    Parser p;
    EXPECT_EQ(p.parse("# this is a comment"), std::nullopt);
}

TEST(ParserTest, ParseSubmitGtc) {
    Parser p;
    auto result = p.parse("SUBMIT 1 BUY 15000 100");
    ASSERT_TRUE(result.has_value());
    auto* sub = std::get_if<SubmitAction>(&*result);
    ASSERT_NE(sub, nullptr);
    EXPECT_EQ(sub->id, 1);
    EXPECT_EQ(sub->side, Side::Buy);
    EXPECT_EQ(sub->type, OrderType::GTC);
    EXPECT_EQ(sub->price, 15000);
    EXPECT_EQ(sub->quantity, 100);
}

TEST(ParserTest, ParseSubmitSellIoc) {
    Parser p;
    auto result = p.parse("SUBMIT 2 SELL 20000 50 IOC");
    ASSERT_TRUE(result.has_value());
    auto* sub = std::get_if<SubmitAction>(&*result);
    ASSERT_NE(sub, nullptr);
    EXPECT_EQ(sub->side, Side::Sell);
    EXPECT_EQ(sub->type, OrderType::IOC);
}

TEST(ParserTest, ParseSubmitFok) {
    Parser p;
    auto result = p.parse("SUBMIT 3 BUY 30000 10 FOK");
    auto* sub = std::get_if<SubmitAction>(&*result);
    ASSERT_NE(sub, nullptr);
    EXPECT_EQ(sub->type, OrderType::FOK);
}

TEST(ParserTest, ParseCancel) {
    Parser p;
    auto result = p.parse("CANCEL 42");
    auto* c = std::get_if<CancelAction>(&*result);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->order_id, 42);
}

TEST(ParserTest, ParseModify) {
    Parser p;
    auto result = p.parse("MODIFY 7 15050 100");
    auto* m = std::get_if<ModifyAction>(&*result);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->order_id, 7);
    EXPECT_EQ(m->new_price, 15050);
    EXPECT_EQ(m->new_qty, 100);
}

TEST(ParserTest, ParseMarket) {
    Parser p;
    auto result = p.parse("MARKET 8 BUY 200");
    auto* m = std::get_if<MarketAction>(&*result);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->id, 8);
    EXPECT_EQ(m->side, Side::Buy);
    EXPECT_EQ(m->qty, 200);
}

TEST(ParserTest, ParsePrint) {
    Parser p;
    auto result = p.parse("PRINT");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<PrintAction>(*result));
}

TEST(ParserTest, UnknownCommandThrows) {
    Parser p;
    EXPECT_THROW(p.parse("FOOBAR 1 2 3"), std::runtime_error);
}

TEST(ParserTest, MalformedSubmitThrows) {
    Parser p;
    EXPECT_THROW(p.parse("SUBMIT"), std::runtime_error);
    EXPECT_THROW(p.parse("SUBMIT 1 BUY 10000"), std::runtime_error);
    EXPECT_THROW(p.parse("SUBMIT 1 BLAH 10000 100"), std::runtime_error);
}

TEST(ParserTest, CaseInsensitiveCommands) {
    Parser p;
    EXPECT_TRUE(p.parse("submit 1 BUY 15000 100").has_value());
    EXPECT_TRUE(p.parse("cancel 1").has_value());
    EXPECT_TRUE(p.parse("market 1 BUY 100").has_value());
    EXPECT_TRUE(p.parse("print").has_value());
}
