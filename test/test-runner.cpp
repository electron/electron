#include <string>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "atom/utility/atom_content_utility_client.h"

using std::string;

namespace {
  
/*
  Classes with virtual functions can be mocked  

  Example:
*/
class Turtle {
public:
  virtual ~Turtle() {}
  virtual void PenUp() = 0;
  virtual void PenDown() = 0;
  virtual void Forward(int distance) = 0;
  virtual void Turn(int degrees) = 0;
  virtual void GoTo(int x, int y) = 0;
  virtual int GetX() const = 0;
  virtual int GetY() const = 0;
};

class MockTurtle : public Turtle {
public:
  MOCK_METHOD0(PenUp, void());
  MOCK_METHOD0(PenDown, void());
  MOCK_METHOD1(Forward, void(int distance));
  MOCK_METHOD1(Turn, void(int degrees));
  MOCK_METHOD2(GoTo, void(int x, int y));
  MOCK_CONST_METHOD0(GetX, int());
  MOCK_CONST_METHOD0(GetY, int());
};

TEST(MockTurtle, Test) {
  using ::testing::AtLeast;

  MockTurtle turtle;
  EXPECT_CALL(turtle, PenDown()).Times(AtLeast(1));

  turtle.PenDown();
}

/*
  Let's test a class with a fixture
*/

// Let's test this class
class Foo {
public:
    int Add(int i, int j) {
        return i + j;
    }
};

// The fixture for testing class Foo.
class FooTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if its body
  // is empty.

  FooTest() {
    // You can do set-up work for each test here.
  }

  virtual ~FooTest() {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  virtual void SetUp() {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  virtual void TearDown() {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test case for Foo.

  Foo foo1;
  Foo foo2;
};

// Tests that the Foo::Add() method works
TEST_F(FooTest, MethodAdd) {
  EXPECT_EQ(2, foo1.Add(1, 1));
  EXPECT_EQ(0, foo2.Add(-1, 1));
}

/*
  Let's try testing something from atom namespace
*/

TEST(AtomContentUtilityClient, Create) {
  atom::AtomContentUtilityClient acuc;
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
