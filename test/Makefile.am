noinst_PROGRAMS = test_sorm test_sem

test_sorm_SOURCES = framework.c test_sorm.c test_sorm.h device_sorm.c device_sorm.h \
					volume_sorm.c volume_sorm.h text_blob_sorm.c text_blob_sorm.h
test_sorm_LDADD = $(SRC_DIR)/libsorm.la -ldl -lcunit
test_sorm_CFLAGS = -I$(ZLOG_DIR) -I$(SRC_DIR)

test_sem_SOURCES = test_sem.c test_sem.h device_sorm.c device_sorm.h
test_sem_LDADD = $(SRC_DIR)/libsorm.la -ldl -lcunit
test_sem_CFLAGS = -I$(ZLOG_DIR) -I$(SRC_DIR)
