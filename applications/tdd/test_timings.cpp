/*
 *
 * Copyright (c) 2014, Nicola Pezzotti (Delft University of Technology)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *  must display the following acknowledgement:
 *  This product includes software developed by the Delft University of Technology.
 * 4. Neither the name of the Delft University of Technology nor the names of
 *  its contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY NICOLA PEZZOTTI ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLA PEZZOTTI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */


#include <catch.hpp>
#include "hdi/utils/timing_utils.h"
#include "hdi/utils/timers.h"
#include "hdi/utils/scoped_timers.h"

TEST_CASE( "Timers throws exceptions when misused", "[timers]" ) {
  hdi::utils::Timer timer;

  REQUIRE_THROWS(timer.stop());
  REQUIRE(timer.isStarted() == false);
  REQUIRE(timer.isElapsedTimeAvailable() == false);
  REQUIRE_NOTHROW(timer.start());
  REQUIRE(timer.isStarted() == true);
  REQUIRE(timer.isElapsedTimeAvailable() == false);
  REQUIRE_THROWS(timer.start());
  REQUIRE(timer.isStarted() == true);
  REQUIRE(timer.isElapsedTimeAvailable() == false);
  REQUIRE_NOTHROW(timer.stop());
  REQUIRE(timer.isStarted() == false);
  REQUIRE(timer.isElapsedTimeAvailable() == true);
}

TEST_CASE( "Timer is returning a valid result", "[timers]" ) {
  hdi::utils::Timer timer;

  timer.start();
  hdi::utils::sleepFor<hdi::utils::Milliseconds>(2500);
  timer.stop();
  double elapsed_time = timer.elapsedTime<hdi::utils::Milliseconds>();
  REQUIRE(std::abs(elapsed_time-2500) < 50);
  elapsed_time = timer.elapsedTime<hdi::utils::Seconds>();
  REQUIRE(std::abs(elapsed_time-2.5) < .05);
}


TEST_CASE( "ScopedTimer is returning a valid result", "[timers]" ) {
  double elapsed_time(0);
  {
    hdi::utils::ScopedTimer<double, hdi::utils::Milliseconds> timer(elapsed_time);
    hdi::utils::sleepFor<hdi::utils::Milliseconds>(500);
  }
  REQUIRE(std::abs(elapsed_time-500) < 50);
}

TEST_CASE( "ScopedIncrementalTimer is returning a valid result", "[timers]" ) {
  double elapsed_time(0);
  {
    hdi::utils::ScopedIncrementalTimer<double, hdi::utils::Milliseconds> timer(elapsed_time);
    hdi::utils::sleepFor<hdi::utils::Milliseconds>(500);
  }
  {
    hdi::utils::ScopedIncrementalTimer<double, hdi::utils::Milliseconds> timer(elapsed_time);
    hdi::utils::sleepFor<hdi::utils::Milliseconds>(500);
  }
  REQUIRE(std::abs(elapsed_time-1000) < 50);
}
