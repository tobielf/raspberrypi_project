/**
 * @file spi_mcp3208.h
 * @brief Public function declaration and communication data.
 * @author Xiangyu Guo
 */
#ifndef __SPI_MCP3208_H__
#define __SPI_MCP3208_H__

/**
 * @brief module structure, hiding the detail to the public
 */
typedef struct mcp3208_module mcp3208_module_st;
struct mcp3208_module;

#define MCP3208_MAX_VALUE           (4095.0)        /**< Max digital value read from ADC */
#define MCP3208_CHANNEL_0           (0)             /**< Channel 0 on ADC */
#define MCP3208_CHANNEL_1           (1)             /**< Channel 1 on ADC */
#define MCP3208_CHANNEL_2           (2)             /**< Channel 2 on ADC */
#define MCP3208_CHANNEL_3           (3)             /**< Channel 3 on ADC */
#define MCP3208_CHANNEL_4           (4)             /**< Channel 4 on ADC */
#define MCP3208_CHANNEL_5           (5)             /**< Channel 5 on ADC */
#define MCP3208_CHANNEL_6           (6)             /**< Channel 6 on ADC */
#define MCP3208_CHANNEL_7           (7)             /**< Channel 7 on ADC */
/* ==============================================
	device module initialize and finish function 
   ============================================== */
/**
 * @brief Get an instance of the module MCP3208
 * @return mcp3208 a initialized, valid mcp3208_module_st.
 */
mcp3208_module_st *mcp3208_module_get_instance();

/**
 * @brief Clean up the module mcp3208
 */
void mcp3208_module_clean_up();

/* =================
    device function 
   ================= */
/**
 * @brief Read the value from ADC based on the specified channel.
 * @param mcp3208 initialized module.
 * @param channel valid channel number[0-7].
 * @return 0-4096 on success; otherwise exit with an error number.
 */
int mcp3208_read_data(mcp3208_module_st *mcp3208, unsigned int channel);
#endif