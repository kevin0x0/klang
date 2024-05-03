#include <ctime>
#include <iostream>
#include <unordered_map>

struct Val {
  size_t hash;
  char reserved[26];
  size_t val;
};

int main(void) {
  std::unordered_map<size_t, Val> map;
  for (size_t i = 0; i < 10000000; ++i)
    map.insert({ i, { i * 2, "hello", i } });
  size_t count = 0;
  clock_t t = clock();
  for (auto itr = map.begin(); itr != map.end(); ++itr)
    count++;
  std::cout << count << "    " << (clock() - t) / (float)(CLOCKS_PER_SEC) << std::endl;
  return 0;
}
