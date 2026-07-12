// test_favstore — favorite-food store: toggle semantics, the full-refusal
// rule, hash stability, and blob round-trip (the NVS persistence path).
#include <unity.h>
#include "core/favstore.h"

void setUp() {}
void tearDown() {}

static void test_toggle_and_has(void) {
  FavStore f;
  const uint32_t eggs = fav_hash("Eggs", "fried");
  TEST_ASSERT_FALSE(f.has(eggs));
  TEST_ASSERT_TRUE(f.toggle(eggs));    // starred
  TEST_ASSERT_TRUE(f.has(eggs));
  TEST_ASSERT_EQUAL_INT(1, f.count());
  TEST_ASSERT_FALSE(f.toggle(eggs));   // unstarred
  TEST_ASSERT_FALSE(f.has(eggs));
  TEST_ASSERT_EQUAL_INT(0, f.count());
}

static void test_full_refuses_add_but_still_removes(void) {
  FavStore f;
  for (int i = 0; i < FavStore::MAX; ++i)
    TEST_ASSERT_TRUE(f.toggle(fav_hash("food", (const char[]){char('a' + i), 0})));
  TEST_ASSERT_EQUAL_INT(FavStore::MAX, f.count());
  TEST_ASSERT_FALSE(f.toggle(fav_hash("one", "more")));   // refused
  TEST_ASSERT_EQUAL_INT(FavStore::MAX, f.count());
  TEST_ASSERT_FALSE(f.has(fav_hash("one", "more")));
  // removal still works at capacity
  TEST_ASSERT_FALSE(f.toggle(fav_hash("food", "a")));
  TEST_ASSERT_EQUAL_INT(FavStore::MAX - 1, f.count());
}

static void test_hash_distinguishes_name_and_variant(void) {
  // The separator matters: ("ab","c") must not collide with ("a","bc").
  TEST_ASSERT_NOT_EQUAL(fav_hash("ab", "c"), fav_hash("a", "bc"));
  TEST_ASSERT_NOT_EQUAL(fav_hash("Pancakes", "4-inch"),
                        fav_hash("Pancakes", "6-inch"));
  TEST_ASSERT_EQUAL_UINT32(fav_hash("Eggs", "fried"), fav_hash("Eggs", "fried"));
}

static void test_blob_roundtrip(void) {
  FavStore a;
  a.toggle(fav_hash("Eggs", "fried"));
  a.toggle(fav_hash("Smash burger", "2oz"));
  uint8_t buf[FavStore::MAX * 4];
  std::memcpy(buf, a.blob(), a.blobBytes());
  FavStore b;
  b.loadBlob(buf, a.blobBytes());
  TEST_ASSERT_EQUAL_INT(2, b.count());
  TEST_ASSERT_TRUE(b.has(fav_hash("Eggs", "fried")));
  TEST_ASSERT_TRUE(b.has(fav_hash("Smash burger", "2oz")));
  TEST_ASSERT_FALSE(b.has(fav_hash("Eggs", "scrambled")));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_toggle_and_has);
  RUN_TEST(test_full_refuses_add_but_still_removes);
  RUN_TEST(test_hash_distinguishes_name_and_variant);
  RUN_TEST(test_blob_roundtrip);
  return UNITY_END();
}
