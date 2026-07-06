// test_profilestore — 8-profile store: add/active, capacity, remove + active
// fixup, rename, and blob round-trip (Phase 3).
#include <unity.h>
#include "core/profilestore.h"
#include "core/profiles.cpp"   // make_profile / learn_lag_from_rate

void setUp() {}
void tearDown() {}

static PanProfile prof(const char* name, float rate) {
  return make_profile(name, rate);
}

static void test_add_and_active(void) {
  ProfileStore s;
  TEST_ASSERT_EQUAL_INT(-1, s.active());
  TEST_ASSERT_EQUAL_INT(0, s.add(prof("Cast iron", 40)));
  TEST_ASSERT_EQUAL_INT(1, s.add(prof("Nonstick", 20)));
  TEST_ASSERT_EQUAL_INT(2, s.count());
  TEST_ASSERT_EQUAL_INT(1, s.active());                 // last added is active
  TEST_ASSERT_EQUAL_STRING("Nonstick", s.activeProfile()->name);
  s.setActive(0);
  TEST_ASSERT_EQUAL_STRING("Cast iron", s.activeProfile()->name);
}

static void test_capacity(void) {
  ProfileStore s;
  for (int i = 0; i < ProfileStore::MAX; ++i)
    TEST_ASSERT_TRUE(s.add(prof("p", 15)) >= 0);
  TEST_ASSERT_EQUAL_INT(-1, s.add(prof("overflow", 15)));
  TEST_ASSERT_EQUAL_INT(ProfileStore::MAX, s.count());
}

static void test_remove_fixes_active(void) {
  ProfileStore s;
  s.add(prof("a", 10)); s.add(prof("b", 10)); s.add(prof("c", 10));  // active=2
  s.remove(2);                                   // removed the active one
  TEST_ASSERT_EQUAL_INT(2, s.count());
  TEST_ASSERT_EQUAL_INT(1, s.active());          // clamped into range
  s.remove(0); s.remove(0);
  TEST_ASSERT_EQUAL_INT(0, s.count());
  TEST_ASSERT_EQUAL_INT(-1, s.active());         // empty
  TEST_ASSERT_NULL(s.activeProfile());
}

static void test_rename(void) {
  ProfileStore s;
  s.add(prof("Pan 1", 12));
  s.rename(0, "Carbon steel");
  TEST_ASSERT_EQUAL_STRING("Carbon steel", s.at(0).name);
}

static void test_blob_roundtrip(void) {
  ProfileStore s;
  s.add(prof("Cast iron", 45));
  s.add(prof("Nonstick", 18));
  s.setActive(0);
  ProfileStore t;
  t.loadBlob(s.blob(), s.blobBytes(), s.active());
  TEST_ASSERT_EQUAL_INT(2, t.count());
  TEST_ASSERT_EQUAL_INT(0, t.active());
  TEST_ASSERT_EQUAL_STRING("Cast iron", t.activeProfile()->name);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, s.at(1).lagMinutes, t.at(1).lagMinutes);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_add_and_active);
  RUN_TEST(test_capacity);
  RUN_TEST(test_remove_fixes_active);
  RUN_TEST(test_rename);
  RUN_TEST(test_blob_roundtrip);
  return UNITY_END();
}
