#########################################################################
# Author: Wang Wencan
# Created Time: Wed 24 Jul 2013 03:56:00 PM CST
# File Name: gen.sh
# Description: 
#########################################################################
#!/bin/bash
generate()
{
    ../src/sorm_generate $1.in
#    mv $1_sorm.c  ../
#    mv $1_sorm.h ../
}

generate device
generate volume
generate text_blob
