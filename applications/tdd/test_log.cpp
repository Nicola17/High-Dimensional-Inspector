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


#include "catch.hpp"

/***********************************************/
#include "hdi/utils/abstract_log.h"
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
/***********************************************/


/***********************************************/
/***********************************************/
/***********************************************/

TEST_CASE( "CoutLog works properly", "[log]" ) {
  hdi::utils::CoutLog cout_log;
  std::string test_string("CoutLog test string");

  SECTION("A newly constructed CoutLog has no written char"){
    REQUIRE( cout_log.num_written_chars() == 0 );
  }

  SECTION("A CoutLog print something on the std::cout"){
    cout_log.display(test_string);
    REQUIRE( cout_log.num_written_chars() > 0 );
  }

  SECTION("A CoutLog print a number of chars which are equal to the dimension of a string plus one"){
    cout_log.display(test_string);
    REQUIRE( cout_log.num_written_chars() == test_string.size() + 1 );
  }

  SECTION("clear sets the number of written characters to zero"){
    cout_log.display(test_string);
    cout_log.clear();
    REQUIRE( cout_log.num_written_chars() == 0 );
  }
}

/***********************************************/
/***********************************************/
/***********************************************/

TEST_CASE( "CoutLog works properly in combination with the log helpers functions", "[log]" ) {
  hdi::utils::CoutLog cout_log;
  std::string test_string("CoutLog + helpers test string");
  int intValue = 42;
  float floatValue = 17.17f;
  double doubleValue = 17.17;
  std::string stringValue("Talk is cheap. Show me the code!");

  SECTION("A call to secureLog on a CoutLog object print something on the std::cout"){
    hdi::utils::secureLog(&cout_log,test_string);
    REQUIRE( cout_log.num_written_chars() > 0 );
  }

  SECTION("A call to secureLog on a CoutLog object with the enabled flag print something on the std::cout"){
    hdi::utils::secureLog(&cout_log,test_string,true);
    REQUIRE( cout_log.num_written_chars() > 0 );
  }

  SECTION("A call to secureLog on a CoutLog object without the enabled flag don't print something on the std::cout"){
    hdi::utils::secureLog(&cout_log,test_string,false);
    REQUIRE( cout_log.num_written_chars() == 0 );
  }

  SECTION("A call to secureLogValue on a CoutLog object print something on the std::cout"){
    hdi::utils::secureLogValue(&cout_log,test_string,intValue);
    REQUIRE( cout_log.num_written_chars() > 0 );
  }

  SECTION("A call to secureLogValue on a CoutLog object with the enabled flag print something on the std::cout"){
    hdi::utils::secureLogValue(&cout_log,test_string,intValue,true);
    REQUIRE( cout_log.num_written_chars() > 0 );
  }

  SECTION("A call to secureLogValue on a CoutLog object without the enabled flag don't print something on the std::cout"){
    hdi::utils::secureLogValue(&cout_log,test_string,intValue,false);
    REQUIRE( cout_log.num_written_chars() == 0 );
  }

  SECTION("A call to secureLogValue on a CoutLog object can print an integer"){
    hdi::utils::secureLogValue(&cout_log,test_string,intValue);
    REQUIRE( cout_log.num_written_chars() > 0 );
  }

  SECTION("A call to secureLogValue on a CoutLog object can print a float"){
    hdi::utils::secureLogValue(&cout_log,test_string,floatValue);
    REQUIRE( cout_log.num_written_chars() > 0 );
  }

  SECTION("A call to secureLogValue on a CoutLog object can print a double"){
    hdi::utils::secureLogValue(&cout_log,test_string,doubleValue);
    REQUIRE( cout_log.num_written_chars() > 0 );
  }

  SECTION("A call to secureLogValue on a CoutLog object can print a string"){
    hdi::utils::secureLogValue(&cout_log,test_string,stringValue);
    REQUIRE( cout_log.num_written_chars() > 0 );
  }
}

/***********************************************/
/***********************************************/
/***********************************************/
