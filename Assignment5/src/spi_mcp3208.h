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
typedef struct spi_mcp3208 spi_mcp3208_st;
struct spi_mcp3208;

/* ==============================================
	device module initialize and finish function 
   ============================================== */
/**
 * @brief Initialize the module MCP3208
 * @param chip_number chip number on Raspberrypi pin, 0 or 1.
 * @param speed chip communication speed, in Hz.
 * @return mcp3208 a initialized, valid spi_mcp3208_st.
 */
spi_mcp3208_st *mcp3208_module_init(unsigned int chip_number, unsigned int speed);

/**
 * @brief Clean up the module mcp3208
 * @param mcp3208 a valid module.
 */
void mcp3208_module_fini(spi_mcp3208_st *mcp3208);

/* =================
    device function 
   ================= */
/**
 * @brief Read the value from ADC based on the specified channel.
 * @param mcp3208 initialized module.
 * @param channel valid channel number[0-7].
 * @return 0-4096 on success; otherwise exit with an error number.
 */
int mcp3208_read_data(spi_mcp3208_st *mcp3208, unsigned int channel);
#endif