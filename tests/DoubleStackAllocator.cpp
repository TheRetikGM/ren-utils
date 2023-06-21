/**
 * @brief This file contains tests for DoubleStackAllocator
 * @file StackAllocator.cpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#include <gtest/gtest.h>
#include <ren_utils/alloc/allocator.hpp>
#include <ren_utils/alloc/DoubleStackAllocator.h>

using namespace ren_utils;

class DoubleStackAllocatorTest : public ::testing::Test {
protected:
  DoubleStackAllocator *a;
  void SetUp() override {
    a = new DoubleStackAllocator(100);
  }
  void TearDown() override {
    delete a;
  }
};

TEST(DoubleStackAllocator, constructor) {
  auto a1 = DoubleStackAllocator(2);
  auto a2 = DoubleStackAllocator(1);
  auto a3 = DoubleStackAllocator(100);
}

TEST_F(DoubleStackAllocatorTest, Alloc_allocates) {
  auto p1 = a->Alloc(AllocSide::LEFT, 10);
  auto p2 = a->Alloc(AllocSide::RIGHT, 20);
  auto p3 = a->Alloc(AllocSide::LEFT, 30);
  ASSERT_NE(p1, nullptr);
  ASSERT_NE(p2, nullptr);
  ASSERT_NE(p3, nullptr);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::LEFT), 10 + 30);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::RIGHT), 20);
}

TEST_F(DoubleStackAllocatorTest, Alloc_sides) {
  auto p1 = (uint8_t*)a->Alloc(AllocSide::LEFT, 10);
  auto p2 = (uint8_t*)a->Alloc(AllocSide::RIGHT, 20);
  auto p3 = (uint8_t*)a->Alloc(AllocSide::LEFT, 30);
  auto p4 = (uint8_t*)a->Alloc(AllocSide::RIGHT, 5);
  ASSERT_NE(p1, nullptr);
  ASSERT_NE(p2, nullptr);
  ASSERT_NE(p3, nullptr);
  ASSERT_EQ(p1 + 10, p3);
  ASSERT_EQ(p4 + 5, p2);
}

TEST_F(DoubleStackAllocatorTest, Alloc_aligned) {
  auto p1 = a->Alloc(AllocSide::LEFT, 10, Align(4));
  auto p2 = a->Alloc(AllocSide::RIGHT, 20, Align(8));
  auto p3 = a->Alloc(AllocSide::LEFT, 5, Align(32));
  ASSERT_NE(p1, nullptr);
  ASSERT_NE(p2, nullptr);
  ASSERT_NE(p3, nullptr);
  ASSERT_TRUE(Align::IsAligned(p1, 4));
  ASSERT_TRUE(Align::IsAligned(p2, 8));
  ASSERT_TRUE(Align::IsAligned(p3, 32));
}

TEST_F(DoubleStackAllocatorTest, Alloc_not_enough_space) {
  auto p1 = a->Alloc(AllocSide::LEFT, 10);
  auto p2 = a->Alloc(AllocSide::RIGHT, 25);
  auto p3 = a->Alloc(AllocSide::RIGHT, 25);
  auto p4 = a->Alloc(AllocSide::LEFT, 50);
  auto p5 = a->Alloc(AllocSide::LEFT, 40);
  auto p6 = a->Alloc(AllocSide::LEFT, 41);
  ASSERT_NE(p1, nullptr);
  ASSERT_NE(p2, nullptr);
  ASSERT_NE(p3, nullptr);
  ASSERT_EQ(p4, nullptr);
  ASSERT_NE(p5, nullptr);
  ASSERT_EQ(p6, nullptr);
}

TEST_F(DoubleStackAllocatorTest, New) {
  auto s = a->New<std::string>(AllocSide::LEFT, "test");
  auto i = a->New<int>(AllocSide::RIGHT);
  *i = 2;
  ASSERT_EQ(*s, "test");
  ASSERT_EQ(*i, 2);
}

TEST_F(DoubleStackAllocatorTest, FreeToMarker) {
  auto marker_empty = std::make_pair( a->GetMarker(AllocSide::LEFT), a->GetMarker(AllocSide::RIGHT));
  a->Alloc(AllocSide::LEFT, 10);
  auto m1 = std::make_pair( a->GetMarker(AllocSide::LEFT), a->GetMarker(AllocSide::RIGHT));
  a->Alloc(AllocSide::RIGHT, 25);
  a->Alloc(AllocSide::RIGHT, 25);
  auto m3 = std::make_pair( a->GetMarker(AllocSide::LEFT), a->GetMarker(AllocSide::RIGHT));
  a->Alloc(AllocSide::LEFT, 40);
  auto m4 = std::make_pair( a->GetMarker(AllocSide::LEFT), a->GetMarker(AllocSide::RIGHT));

  a->FreeToMarker(m4.first);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::LEFT), 50);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::RIGHT), 50);
  a->FreeToMarker(m4.second);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::LEFT), 50);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::RIGHT), 50);
  a->FreeToMarker(m3.first);
  a->FreeToMarker(m3.second);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::LEFT), 10);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::RIGHT), 50);
  a->FreeToMarker(m1.second);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::LEFT), 10);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::RIGHT), 0);
  a->FreeToMarker(marker_empty.first);
  a->FreeToMarker(marker_empty.second);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::LEFT), 0);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::RIGHT), 0);
}

TEST_F(DoubleStackAllocatorTest, ClearAll) {
  a->Alloc(AllocSide::LEFT, 10);
  a->Alloc(AllocSide::LEFT, 10, Align(8));
  a->Alloc(AllocSide::RIGHT, 10);
  ASSERT_GE(a->GetCurrentSize(AllocSide::LEFT), 20);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::RIGHT), 10);
  a->ClearAll();
  ASSERT_EQ(a->GetCurrentSize(AllocSide::LEFT), 0);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::RIGHT), 0);
}

TEST_F(DoubleStackAllocatorTest, Clear) {
  a->Alloc(AllocSide::LEFT, 10);
  a->Alloc(AllocSide::LEFT, 10, Align(8));
  a->Alloc(AllocSide::RIGHT, 10);
  ASSERT_GE(a->GetCurrentSize(AllocSide::LEFT), 20);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::RIGHT), 10);
  a->Clear(AllocSide::LEFT);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::LEFT), 0);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::RIGHT), 10);
  a->Clear(AllocSide::RIGHT);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::LEFT), 0);
  ASSERT_EQ(a->GetCurrentSize(AllocSide::RIGHT), 0);
}

TEST_F(DoubleStackAllocatorTest, Empty) {
  ASSERT_TRUE(a->Empty(AllocSide::LEFT));
  ASSERT_TRUE(a->Empty(AllocSide::RIGHT));
  a->Alloc(AllocSide::LEFT, 10);
  a->Alloc(AllocSide::RIGHT, 20, Align(2));
  a->Alloc(AllocSide::LEFT, 20);
  a->ClearAll();
  ASSERT_TRUE(a->Empty(AllocSide::LEFT));
  ASSERT_TRUE(a->Empty(AllocSide::RIGHT));
}

TEST_F(DoubleStackAllocatorTest, EmptyBoth) {
  ASSERT_TRUE(a->EmptyBoth());
  a->Alloc(AllocSide::LEFT, 10);
  a->Alloc(AllocSide::RIGHT, 20, Align(2));
  a->Alloc(AllocSide::LEFT, 20);
  a->ClearAll();
  ASSERT_TRUE(a->EmptyBoth());
}

struct ConTester {
  bool* con;
  bool* dest;
  ConTester(bool* p_con, bool* p_dest) : con(p_con), dest(p_dest) {
    *p_con = true;
  }
  ~ConTester() { *dest = true; }
};

TEST(DoubleStackAllocator_Ptr, new_ptr_values) {
  DoubleStackAllocator a(100);
  auto p1 = new_ptr<uint32_t>(a, AllocSide::LEFT, Align(8), 123);
  auto p2 = new_ptr<std::string>(a, AllocSide::LEFT, Align(16), "test");
  auto p3 = new_ptr<char>(a, AllocSide::LEFT, Align(2), 'a');
  ASSERT_NE(p1.m_Ptr, nullptr);
  ASSERT_NE(p2.m_Ptr, nullptr);
  ASSERT_NE(p3.m_Ptr, nullptr);
  EXPECT_EQ(*p1, 123);
  EXPECT_EQ(*p2, "test");
  EXPECT_EQ(p3.Get(), 'a');
  delete_ptr(p2);
}

TEST(DoubleStackAllocator_Ptr, new_ptr_constructor_called) {
  DoubleStackAllocator a(100);
  bool c1 = false, d1 = false;
  bool c2 = false, d2 = false;

  new_ptr<ConTester>(a, AllocSide::LEFT, &c1, &d1);
  new_ptr<ConTester>(a, AllocSide::LEFT, &c2, &d2);

  ASSERT_TRUE(c1);
  ASSERT_TRUE(c2);
}

TEST(DoubleStackAllocator_Ptr, new_ptr_no_enough_memory) {
  DoubleStackAllocator a(5);

  new_ptr<uint32_t>(a, AllocSide::LEFT, 10);
  EXPECT_THROW(new_ptr<uint16_t>(a, AllocSide::LEFT), std::runtime_error);
  EXPECT_THROW(new_ptr<uint16_t>(a, AllocSide::RIGHT), std::runtime_error);
  a.ClearAll();
  new_ptr<uint8_t>(a, AllocSide::RIGHT, 1);
  new_ptr<uint32_t>(a, AllocSide::RIGHT, 1);
  EXPECT_THROW(new_ptr<uint8_t>(a, AllocSide::LEFT), std::runtime_error);
  EXPECT_THROW(new_ptr<uint8_t>(a, AllocSide::RIGHT), std::runtime_error);
}

TEST(DoubleStackAllocator_Ptr, new_ptr_aligned) {
  DoubleStackAllocator a(100);
  auto p1 = new_ptr<uint32_t>(a, AllocSide::LEFT, Align(8), 123);
  auto p2 = new_ptr<std::string>(a, AllocSide::RIGHT, Align(16), "test");
  auto p3 = new_ptr<char>(a, AllocSide::LEFT, Align(2), 'a');
  ASSERT_TRUE(Align::IsAligned(p1.m_Ptr, 8));
  ASSERT_TRUE(Align::IsAligned(p2.m_Ptr, 16));
  ASSERT_TRUE(Align::IsAligned(p3.m_Ptr, 2));
  delete_ptr(p2);
}

TEST(DoubleStackAllocator_Ptr, delete_ptr) {
  DoubleStackAllocator a(100);
  auto p1 = new_ptr<uint32_t>(a, AllocSide::LEFT, Align(8), 123);
  auto p2 = new_ptr<std::string>(a, AllocSide::RIGHT, Align(16), "test");
  auto p3 = new_ptr<char>(a, AllocSide::LEFT, Align(2), 'a');
  delete_ptr(p2);
  delete_ptr(p3);
  delete_ptr(p1);
}

TEST(DoubleStackAllocator_Ptr, delete_ptr_destructor_called) {
  DoubleStackAllocator a(100);
  bool c = false, d = false;
  auto p = new_ptr<ConTester>(a, AllocSide::LEFT, &c, &d);
  delete_ptr(p);
  EXPECT_TRUE(d);
}

TEST(DoubleStackAllocator_Ptr, delete_ptr_wrong_order) {
  DoubleStackAllocator a(100);
  auto p1 = new_ptr<uint32_t>(a, AllocSide::LEFT, Align(8), 123);
  auto p2 = new_ptr<std::string>(a, AllocSide::RIGHT, Align(16), "test");
  auto p3 = new_ptr<char>(a, AllocSide::LEFT, Align(2), 'a');
  delete_ptr(p1);
  delete_ptr(p2);
  EXPECT_THROW(delete_ptr(p3), std::invalid_argument);
}
