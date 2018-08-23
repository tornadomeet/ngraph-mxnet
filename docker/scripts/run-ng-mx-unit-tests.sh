# *******************************************************************************
# * Copyright 2018 Intel Corporation
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *     http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# ********************************************************************************

#!/bin/bash
#Author:  Lam Nguyen
set -u

cd "$HOME/ng-mx"

cd python && sudo -E pip install -e . && cd ../

### tests/python/unittest/ 

## Unit tests test_module.py
cmd="OMP_NUM_THREADS=4 pytest -s -n 2 tests/python/unittest/test_module.py --verbose --capture=no --junit-xml=result_test_module.xml --junit-prefix=result_test_module"
eval $cmd

if [ ! -f "$HOME/ng-mx/result_test_module.xml" ]; then 
	(>&2 echo "ERROR: Missing test report." )
	exit 1
fi

### tests/cpp
make test -j$(nproc) USE_NGRAPH=1
export LD_LIBRARY_PATH="$HOME/ng-mx/ngraph_dist/lib"
./build/tests/cpp/mxnet_unit_tests --gtest_output="xml:result_test_cpp.xml"
