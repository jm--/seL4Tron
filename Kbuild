#
# Copyright (c) 2015, Josef Mihalits
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#

apps-$(CONFIG_APP_TRON) += tron

# dependencies
tron-y = common libsel4 libmuslc libsel4muslcsys libsel4vka \
         libsel4allocman libsel4simple libsel4simple-stable \
         libsel4platsupport libsel4vspace libsel4utils libutils

# dependencies
tron: kernel_elf $(tron-y)

