# List of all the board related files.
BOARDCPPSRC =  $(BOARD_DIR)/board_configuration.cpp

# defines SHORT_BOARD_NAME
include $(BOARD_DIR)/meta-info.env

# Override DEFAULT_ENGINE_TYPE
DDEFS += -DSTM32F407xx
DDEFS += -DFIRMWARE_ID=\"KRC_M52_2\"
DDEFS += -DHAL_TRIGGER_USE_PAL=TRUE

# reduce memory usage monitoring
DDEFS += -DRAM_UNUSED_SIZE=100

# Not use critical LED
DDEFS += -DLED_CRITICAL_ERROR_BRAIN_PIN=Gpio::Unassigned
