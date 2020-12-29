// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2020
// MIT License

#include "FailingAllocator.hpp"

#include "StreamUtils/Prints/StringPrint.hpp"
#include "StreamUtils/Streams/MemoryStream.hpp"
#include "StreamUtils/Streams/SpyingStream.hpp"
#include "StreamUtils/Streams/WriteLoggingStream.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("WriteLoggingStream") {
  MemoryStream upstream(4);

  StringPrint log;
  SpyingStream upstreamSpy{upstream, log};

  StringPrint output;
  WriteLoggingStream loggingStream{upstreamSpy, output};

  SUBCASE("available()") {
    upstream.print("ABC");

    size_t n = loggingStream.available();

    CHECK(n == 3);
    CHECK(log.str() == "available() -> 3");
    CHECK(output.str() == "");
  }

  SUBCASE("peek()") {
    upstream.print("ABC");

    int n = loggingStream.peek();

    CHECK(n == 'A');
    CHECK(log.str() == "peek() -> 65");
    CHECK(output.str() == "");
  }

  SUBCASE("read()") {
    upstream.print("ABC");

    int n = loggingStream.read();

    CHECK(n == 'A');
    CHECK(log.str() == "read() -> 65");
    CHECK(output.str() == "");
  }

  SUBCASE("readBytes()") {
    upstream.print("ABC");

    char s[4] = {0};
    size_t n = loggingStream.readBytes(s, 4);

    CHECK(n == 3);
    CHECK(output.str() == "");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
    CHECK(log.str() == "readBytes(4) -> 3 [timeout]");
#endif
  }

  SUBCASE("write(char)") {
    int n = loggingStream.write('A');

    CHECK(n == 1);
    CHECK(log.str() == "write('A') -> 1");
    CHECK(output.str() == "A");
  }

  SUBCASE("write(char*,size_t)") {
    int n = loggingStream.write("ABCDEF", 6);

    CHECK(n == 4);
    CHECK(log.str() == "write('ABCDEF', 6) -> 4");
    CHECK(output.str() == "ABCD");
  }

  SUBCASE("flush()") {
    loggingStream.flush();

    CHECK(log.str() == "flush()");
    CHECK(output.str() == "");
  }
}
