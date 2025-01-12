/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include "gtest/gtest.h"

#include "mozilla/RefPtr.h"
#include "nsIOutputStream.h"
#include "nsIPipe.h"
#include "nsTArray.h"

#include "Helpers.h"

extern "C" void Rust_Future(bool* aItWorked);

TEST(RustMozTask, Future)
{
  bool itWorked = false;
  Rust_Future(&itWorked);
  EXPECT_TRUE(itWorked);
}

extern "C" void Rust_ReadFromStream(nsIInputStream*);

TEST(RustMozTask, InputStream)
{
  uint32_t aNumBytes = 64 * 1024;
  uint32_t aSegmentSize = 32 * 1024;

  RefPtr<nsIInputStream> reader;
  RefPtr<nsIOutputStream> writer;

  uint32_t maxSize = std::max(aNumBytes, aSegmentSize);

  NS_NewPipe(getter_AddRefs(reader), getter_AddRefs(writer), aSegmentSize,
             maxSize);

  nsTArray<char> inputData;
  testing::CreateData(aNumBytes, inputData);
  testing::WriteAllAndClose(writer, inputData);


  Rust_ReadFromStream(reader.get());
}
