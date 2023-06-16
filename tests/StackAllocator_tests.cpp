/**
 * @breif This file contains tests for ren_utils/alloc/StackAllocator
 * @file StackAllocator_tests.cpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#include <gtest/gtest.h>
#include <ren_utils/alloc/stack.hpp>
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

