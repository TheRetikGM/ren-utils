/**
 * @brief This file contains tests for ren_utils/alloc/StackAllocator
 * @file StackAllocator.cpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#include <algorithm>
#include <gtest/gtest.h>
#include <ren_utils/alloc/StackAllocator.h>
#include <ren_utils/alloc/allocator.hpp>
#include <stdexcept>
using namespace ren_utils;

TEST(StackAllocator, InitialEmpty) {
  StackAllocator a1(10);
  EXPECT_TRUE(a1.Empty());
}

TEST(StackAllocator, Constructor_InvalidArg) {
  EXPECT_THROW(StackAllocator a1(0), std::invalid_argument);
}

TEST(StackAllocator, GetSize) {
  StackAllocator a1(10);
  const StackAllocator& ra = a1;
  EXPECT_EQ(a1.GetSize(), 10);
  EXPECT_EQ(ra.GetSize(), 10);
  a1.Alloc(5);
  EXPECT_EQ(a1.GetSize(), 10);
  EXPECT_EQ(ra.GetSize(), 10);
  a1.Alloc(5);
  EXPECT_EQ(a1.GetSize(), 10);
  EXPECT_EQ(ra.GetSize(), 10);
}

TEST(StackAllocator, GetCurrentSize) {
  StackAllocator a(10);
  const StackAllocator& ra = a;
  EXPECT_EQ(a.GetCurrentSize(), 0);
  EXPECT_EQ(ra.GetCurrentSize(), 0);
  a.Alloc(5);
  EXPECT_EQ(a.GetCurrentSize(), 5);
  EXPECT_EQ(ra.GetCurrentSize(), 5);
  a.Alloc(5);
  EXPECT_EQ(a.GetCurrentSize(), 10);
  EXPECT_EQ(ra.GetCurrentSize(), 10);
}

TEST(StackAllocator, Alloc) {
  StackAllocator a(10);
  void *b, *c, *d;
  b = c = d = nullptr;
  ASSERT_NE(b = a.Alloc(5), nullptr);
  ASSERT_NE(c = a.Alloc(4), nullptr);
  ASSERT_NE(d = a.Alloc(1), nullptr);
  ASSERT_EQ(a.Alloc(1), nullptr);
  ASSERT_EQ(a.Alloc(100), nullptr);
  ASSERT_TRUE(b != c && c != d && b != d);
}

TEST(StackAllocator, Alloc_aligned) {
  StackAllocator a(50);
  auto p1 = a.Alloc(5, Align(2));
  auto p2 = a.Alloc(4, Align(4));
  auto p3 = a.Alloc(1, Align(16));
  ASSERT_TRUE(Align::IsAligned(p1, 2));
  ASSERT_TRUE(Align::IsAligned(p2, 4));
  ASSERT_TRUE(Align::IsAligned(p3, 16));
}

TEST(StackAllocator, GetMarker) {
  StackAllocator a(10);
  auto m1 = a.GetMarker();
  a.Alloc(5);
  auto m2 = a.GetMarker();
  auto m2_same = a.GetMarker();
  a.Alloc(5);
  auto m3 = a.GetMarker();
  EXPECT_TRUE(m1 != m2 && m2 != m3);
  EXPECT_LT(m1, m2);
  EXPECT_LT(m2, m3);
  EXPECT_GT(m3, m2);
  EXPECT_GT(m2, m1);
  EXPECT_EQ(m2, m2_same);
}

TEST(StackAllocator, FreeToMarker_sequential_free) {
  StackAllocator alloc(10);
  auto mark_empty = alloc.GetMarker();
  alloc.Alloc(2);
  auto mark_between = alloc.GetMarker();
  alloc.Alloc(5);
  auto mark_top = alloc.GetMarker();
  size_t size_before_free = alloc.GetCurrentSize();

  alloc.FreeToMarker(mark_top);
  ASSERT_EQ(size_before_free, alloc.GetCurrentSize());
  alloc.FreeToMarker(mark_between);
  ASSERT_EQ(2, alloc.GetCurrentSize());
  alloc.FreeToMarker(mark_empty);
  ASSERT_EQ(0, alloc.GetCurrentSize());
}

TEST(StackAllocator, FreeToMarker_skipping_marker) {
  StackAllocator alloc(10);
  auto mark_empty = alloc.GetMarker();
  alloc.Alloc(2);
  alloc.Alloc(5);
  auto mark_top = alloc.GetMarker();
  (void)mark_top;

  alloc.FreeToMarker(mark_empty);
  ASSERT_EQ(0, alloc.GetCurrentSize());
}

TEST(StackAllocator, FreeToMarker_invalid_marker) {
  StackAllocator alloc(10);
  auto mark_empty = alloc.GetMarker();
  alloc.Alloc(2);
  auto mark_between = alloc.GetMarker();
  alloc.Alloc(5);
  auto mark_top = alloc.GetMarker();

  alloc.FreeToMarker(mark_between);
  EXPECT_EQ(2, alloc.GetCurrentSize());
  EXPECT_THROW(alloc.FreeToMarker(mark_top), std::invalid_argument);
  alloc.FreeToMarker(mark_empty);
  ASSERT_EQ(0, alloc.GetCurrentSize());
}

TEST(StackAllocator, FreeToMarker_full_clear) {
  StackAllocator alloc(10);
  auto mark_empty = alloc.GetMarker();
  alloc.Alloc(2);
  alloc.Alloc(2);
  alloc.Alloc(2);
  alloc.Alloc(2);
  alloc.FreeToMarker(mark_empty);
  EXPECT_EQ(alloc.GetCurrentSize(), 0);
}

TEST(StackAllocator, Clear) {
  StackAllocator alloc(10);
  alloc.Alloc(2);
  alloc.Alloc(3);
  alloc.Alloc(2);
  alloc.Alloc(2);
  alloc.Alloc(1);
  ASSERT_EQ(alloc.GetCurrentSize(), alloc.GetSize());
  alloc.Clear();
  EXPECT_EQ(alloc.GetCurrentSize(), 0);
  EXPECT_EQ(alloc.GetSize(), 10);
}

TEST(StackAllocator, Empty) {
  StackAllocator alloc(10);
  EXPECT_TRUE(alloc.Empty());
  alloc.Alloc(0);
  EXPECT_TRUE(alloc.Empty());
  alloc.Alloc(1);
  EXPECT_FALSE(alloc.Empty());
}

TEST(StackAllocator_Ptr, new_ptr_unaligned) {
  StackAllocator alloc(70);

  auto p = new_ptr<int>(alloc, 7);
  auto p_s = new_ptr<std::string>(alloc, "test");
  ASSERT_NE(p.m_Ptr, nullptr);
  ASSERT_NE(p_s.m_Ptr, nullptr);
  EXPECT_EQ(*p, 7);
  EXPECT_EQ(*p_s, "test");
  EXPECT_TRUE(p);
  EXPECT_TRUE(p_s);

  delete_ptr(p_s);
  delete_ptr(p);
}

TEST(StackAllocator_Ptr, new_ptr_aligned) {
  StackAllocator alloc(70);

  auto p_int = new_ptr<int>(alloc, Align(4), 7);
  auto p_string = new_ptr<std::string>(alloc, Align(16),  "test");
  ASSERT_NE(p_int.m_Ptr, nullptr);
  ASSERT_NE(p_string.m_Ptr, nullptr);
  ASSERT_EQ(*p_int, 7);
  ASSERT_EQ(*p_string, "test");
  ASSERT_TRUE(Align::IsAligned(p_int.m_Ptr, 4));
  ASSERT_TRUE(Align::IsAligned(p_string.m_Ptr, 16));

  delete_ptr(p_string);
  delete_ptr(p_int);
}

TEST(StackAllocator_Ptr, new_ptr_sizes) {
  StackAllocator alloc(100);
  new_ptr<int>(alloc, 7);
  auto p2 = new_ptr<std::string>(alloc, "Hey");
  new_ptr<uint16_t>(alloc, 65000);

  ASSERT_EQ(alloc.GetCurrentSize(), sizeof(int) + sizeof(std::string) + sizeof(uint16_t));

  delete_ptr(p2);
}

TEST(StackAllocator_Ptr, new_ptr_too_large) {
  StackAllocator alloc(10);
  new_ptr<int>(alloc, 1);
  EXPECT_THROW(new_ptr<std::string>(alloc, "test"), std::runtime_error);
}

TEST(StackAllocator_Ptr, delete_ptr_basic) {
  StackAllocator alloc(20);
  auto p1 = new_ptr<int>(alloc, 1);
  auto p2 = new_ptr<size_t>(alloc, 2);
  delete_ptr(p2);
  EXPECT_EQ(alloc.GetCurrentSize(), sizeof(int));
  delete_ptr(p1);
  EXPECT_EQ(alloc.GetCurrentSize(), 0U);
}

TEST(StackAllocator_Ptr, delete_ptr_wrong_order) {
  StackAllocator alloc(30);
  auto p1 = new_ptr<int>(alloc, 1);
  auto p2 = new_ptr<char>(alloc, 'a');
  auto p3 = new_ptr<size_t>(alloc, 2);

  delete_ptr(p2);
  EXPECT_THROW(delete_ptr(p3), std::invalid_argument);
  delete_ptr(p1);
}

