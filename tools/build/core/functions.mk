##
 # Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #   http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
##

define cur-dir
    $(strip \
        $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST)))) \
    )
endef

define list-all-makefiles-under
    $(wildcard $(1)/*/build.mk)
endef

define include-all-subdir-makefiles
    $(foreach one_mk, $(call list-all-makefiles-under, $(1)), \
        $(eval include $(one_mk)) \
    )
endef
