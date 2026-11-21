#include "unity.h"
#include "app_light_logic.h"

void setUp(void) {}
void tearDown(void) {}

void test_first_reading_is_published(void)
{
    TEST_ASSERT_TRUE(
        app_light_should_publish(100.0f, 0.0f, false, 0, 1.0f, 45000)
    );
}

void test_changed_reading_is_published(void)
{
    TEST_ASSERT_TRUE(
        app_light_should_publish(105.0f, 100.0f, true, 5000, 1.0f, 45000)
    );
}

void test_small_change_is_not_published(void)
{
    TEST_ASSERT_FALSE(
        app_light_should_publish(100.5f, 100.0f, true, 5000, 1.0f, 45000)
    );
}

void test_heartbeat_forces_publication(void)
{
    TEST_ASSERT_TRUE(
        app_light_should_publish(100.0f, 100.0f, true, 45000, 1.0f, 45000)
    );
}

void test_decreasing_light_is_detected(void)
{
    TEST_ASSERT_TRUE(
        app_light_should_publish(90.0f, 100.0f, true, 5000, 1.0f, 45000)
    );
}

void test_just_below_heartbeat_does_not_publish(void)
{
    TEST_ASSERT_FALSE(
        app_light_should_publish(100.0f, 100.0f, true, 44999, 1.0f, 45000)
    );
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_first_reading_is_published);
    RUN_TEST(test_changed_reading_is_published);
    RUN_TEST(test_small_change_is_not_published);
    RUN_TEST(test_heartbeat_forces_publication);
    RUN_TEST(test_decreasing_light_is_detected);
    RUN_TEST(test_just_below_heartbeat_does_not_publish);

    return UNITY_END();
}
