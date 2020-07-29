#include <iostream>
#include "gmock/gmock.h"
#include "utils/custom_string.h"
#include "utils/logger.h"

SDL_CREATE_LOGGERPTR( "SDLMain")
int main(int argc, char** argv) {
  namespace custom_str = utils::custom_string;
  testing::InitGoogleMock(&argc, argv);
  ::testing::DefaultValue<custom_str::CustomString>::Set(
      custom_str::CustomString(""));
  const int result = RUN_ALL_TESTS();

  SDL_DEINIT_LOGGER()();
  return result;
}
