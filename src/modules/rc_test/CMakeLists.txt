px4_add_module(
    MODULE modules__rc_test
    MAIN rc_test
    STACK_MAIN 2000
    SRCS
        rc_test.cpp
    DEPENDS
        px4_work_queue
)

