/// Copyright 2016 Pinterest Inc.
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
/// http://www.apache.org/licenses/LICENSE-2.0

/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.


#include "common/object_lock.h"

#include <algorithm>
#include <future>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

using common::ObjectLock;
using std::promise;
using std::thread;
using std::vector;

void RunMultipleThreadTest(size_t nThread, size_t nObjects, size_t n) {
  ObjectLock<int> locks(nThread);
  promise<void> p;
  auto sf = p.get_future().share();

  vector<int> sums(nObjects);
  vector<thread> threads(nThread);

  vector<size_t> objects;
  for (size_t i = 0; i < nObjects; ++i) {
    objects.push_back(i);
  }

  for (size_t i = 0; i < nThread; ++i) {
    threads[i] = thread([n, objects, &locks, &sf, &sums]() mutable {
      sf.wait();
      while (n--) {
        for (const auto obj : objects) {
          locks.Lock(obj);
          ++sums[obj];
          locks.Unlock(obj);
        }
      }
    });

    random_shuffle(objects.begin(), objects.end());
  }

  for (size_t i = 0; i < nObjects; ++i) {
    EXPECT_EQ(sums[i], 0);
  }

  p.set_value();
  for (auto& t : threads) {
    t.join();
  }

  for (size_t i = 0; i < nObjects; ++i) {
    EXPECT_EQ(sums[i], nThread * n);
  }
}

TEST(ObjectLockTest, ObjectLockTest) {
  RunMultipleThreadTest(1, 1, 1);
  RunMultipleThreadTest(1, 1, 100);
  RunMultipleThreadTest(1, 100, 100);
  RunMultipleThreadTest(1, 10000, 100);
  RunMultipleThreadTest(100, 1, 100);
  RunMultipleThreadTest(1000, 1, 100);
  RunMultipleThreadTest(1000, 10, 999);
  RunMultipleThreadTest(100, 1000, 100);
  RunMultipleThreadTest(99, 999, 99);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
