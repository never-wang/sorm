noinst_PROGRAMS = test_sorm #test_sem test_transaction db_test test_pthread_transaction

test_sorm_SOURCES = framework.c device_sorm.c device_sorm.h \
					volume_sorm.c volume_sorm.h text_blob_sorm.c text_blob_sorm.h \
					test_sorm.c
test_sorm_LDADD = $(SRC_DIR)/libsorm.la -ldl -lcunit
test_sorm_CFLAGS = -I$(ZLOG_DIR) -I$(SRC_DIR) -I$(SQLITE_DIR)

#test_sem_SOURCES = test_sem.c device_sorm.c device_sorm.h 
#test_sem_LDADD = $(SRC_DIR)/libsorm.la $(UTILS_DIR)/libutils.la -ldl -lcunit
#test_sem_CFLAGS = -I$(ZLOG_DIR) -I$(SRC_DIR) -I$(UTILS_DIR) -I$(SQLITE_DIR)
#
#test_transaction_SOURCES = test_transaction.c device_sorm.c device_sorm.h
#test_transaction_LDADD = $(UTILS_DIR)/libutils.la $(SRC_DIR)/libsorm.la -ldl -lcunit
#test_transaction_CFLAGS = -I$(ZLOG_DIR) -I$(SRC_DIR) -I$(UTILS_DIR) -I$(SQLITE_DIR)
#
#db_test_SOURCES = db_test.c device_sorm.c device_sorm.h volume_sorm.c volume_sorm.h times.c times.h test_index_sorm.c test_index_sorm.h
#db_test_LDADD = $(SRC_DIR)/libsorm.la -ldl -lcunit
#db_test_CFLAGS = -I$(ZLOG_DIR) -I$(SRC_DIR) -I$(SQLITE_DIR)
#
#test_pthread_transaction_SOURCES = test_pthread_transaction.c device_sorm.c device_sorm.h
#test_pthread_transaction_LDADD = $(SRC_DIR)/libsorm.la -ldl -lcunit
#test_pthread_transaction_CFLAGS = -I$(ZLOG_DIR) -I$(SRC_DIR) -I$(SQLITE_DIR)

%_sorm.c %_sorm.h : %.in $(SRC_DIR)/sorm_generate
	$(SRC_DIR)/sorm_generate $<

