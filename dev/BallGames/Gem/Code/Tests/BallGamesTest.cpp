
#include <AzTest/AzTest.h>

class BallGamesTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
};

TEST_F(BallGamesTest, SanityTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
