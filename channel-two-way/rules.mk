# Copyright 2018 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := core

MODULE_SRCS += \
    $(LOCAL_DIR)/main.cpp	\
    $(LOCAL_DIR)/parent.cpp	\
    $(LOCAL_DIR)/child.cpp

MODULE_STATIC_LIBS := system/ulib/pretty system/ulib/fbl

MODULE_LIBS := system/ulib/fdio system/ulib/c  system/ulib/zircon

include make/module.mk
