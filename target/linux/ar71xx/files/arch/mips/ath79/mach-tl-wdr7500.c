/*
 *  TP-LINK TL-WDR7500 board support
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/ar8216_platform.h>
#include <linux/gpio.h>

#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "pci.h"
#include "dev-ap9x-pci.h"
#include "dev-gpio-buttons.h"
#include "dev-eth.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-nfc.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define WDR7500_GPIO_LED_WLAN2G		12
#define WDR7500_GPIO_LED_WLAN5G		17

#define WDR7500_GPIO_LED_USB1		18
#define WDR7500_GPIO_LED_USB2		19
#define WDR7500_GPIO_LED_SYSTEM		14
#define WDR7500_GPIO_LED_QSS			15
#define WDR7500_GPIO_BTN_WPS		16

#define WDR7500_GPIO_USB1_POWER		22
#define WDR7500_GPIO_USB2_POWER		21

#define WDR7500_KEYS_POLL_INTERVAL	20	/* msecs */
#define WDR7500_KEYS_DEBOUNCE_INTERVAL	(3 * WDR7500_KEYS_POLL_INTERVAL)

#define WDR7500_MAC0_OFFSET		0
#define WDR7500_MAC1_OFFSET		6
#define WDR7500_WMAC_CALDATA_OFFSET	0x1000
#define WDR7500_PCIE_CALDATA_OFFSET	0x5000

static const char *wdr7500_part_probes[] = {
	"tp-link",
	NULL,
};

static struct flash_platform_data wdr7500_flash_data = {
	.part_probes = wdr7500_part_probes,
};

static struct gpio_led wdr7500_leds_gpio[] __initdata = {
	{
	 .name = "tp-link:blue:qss",
	 .gpio = WDR7500_GPIO_LED_QSS,
	 .active_low = 1,
	 },
	{
	 .name = "tp-link:blue:system",
	 .gpio = WDR7500_GPIO_LED_SYSTEM,
	 .active_low = 1,
	 },
	{
	 .name = "tp-link:green:usb1",
	 .gpio = WDR7500_GPIO_LED_USB1,
	 .active_low = 1,
	 },
	{
	 .name = "tp-link:green:usb2",
	 .gpio = WDR7500_GPIO_LED_USB2,
	 .active_low = 1,
	 },
	{
	 .name = "tp-link:blue:wlan2g",
	 .gpio = WDR7500_GPIO_LED_WLAN2G,
	 .active_low = 1,
	 },
	{
	 .name = "tp-link:blue:wlan5g",
	 .gpio = WDR7500_GPIO_LED_WLAN5G,
	 .active_low = 1,
	 },
};

static struct gpio_keys_button wdr7500_gpio_keys[] __initdata = {
	{
	 .desc = "QSS button",
	 .type = EV_KEY,
	 .code = KEY_WPS_BUTTON,
	 .debounce_interval = WDR7500_KEYS_DEBOUNCE_INTERVAL,
	 .gpio = WDR7500_GPIO_BTN_WPS,
	 .active_low = 1,
	 },
};

/* GMAC0 of the AR8327 switch is connected to GMAC1 via SGMII */
static struct ar8327_pad_cfg wdr7500_ar8327_pad0_cfg = {

	.mode = AR8327_PAD_MAC_SGMII,
	.sgmii_delay_en = true,
};

/* GMAC6 of the AR8327 switch is connected to GMAC0 via RGMII */
static struct ar8327_pad_cfg wdr7500_ar8327_pad6_cfg = {
	.mode = AR8327_PAD_MAC_RGMII,
	.txclk_delay_en = true,
	.rxclk_delay_en = true,
	.txclk_delay_sel = AR8327_CLK_DELAY_SEL1,
	.rxclk_delay_sel = AR8327_CLK_DELAY_SEL2,
};

static struct ar8327_platform_data wdr7500_ar8327_data = {
	.pad0_cfg = &wdr7500_ar8327_pad0_cfg,
	.pad6_cfg = &wdr7500_ar8327_pad6_cfg,
	.port0_cfg = {
		      .force_link = 1,
		      .speed = AR8327_PORT_SPEED_1000,
		      .duplex = 1,
		      .txpause = 1,
		      .rxpause = 1,
		      },
	.port6_cfg = {
		      .force_link = 1,
		      .speed = AR8327_PORT_SPEED_1000,
		      .duplex = 1,
		      .txpause = 1,
		      .rxpause = 1,
		      },
};

static struct mdio_board_info wdr7500_mdio0_info[] = {
	{
	 .bus_id = "ag71xx-mdio.0",
	 .phy_addr = 0,
	 .platform_data = &wdr7500_ar8327_data,
	 },
};

static void __init ath79_setup_qca955x_eth_cfg(u32 mask)
{
	void __iomem *base;
	u32 t;

	base = ioremap(QCA955X_GMAC_BASE, QCA955X_GMAC_SIZE);

	t = __raw_readl(base + QCA955X_GMAC_REG_ETH_CFG);

	t &= ~(QCA955X_ETH_CFG_RGMII_EN | QCA955X_ETH_CFG_GE0_SGMII);
	t |= mask;

	__raw_writel(t, base + QCA955X_GMAC_REG_ETH_CFG);
	/* flush write */
	__raw_readl(base + QCA955X_GMAC_REG_ETH_CFG);

	iounmap(base);
}

static void __init wdr7500_setup(void)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);

	ath79_register_m25p80(&wdr7500_flash_data);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(wdr7500_leds_gpio),
				 wdr7500_leds_gpio);

	ath79_register_gpio_keys_polled(-1, WDR7500_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(wdr7500_gpio_keys),
					wdr7500_gpio_keys);

	gpio_request_one(WDR7500_GPIO_USB1_POWER,
			 GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_FIXED,
			 "USB1 power");
	gpio_request_one(WDR7500_GPIO_USB2_POWER,
			 GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_FIXED,
			 "USB2 power");
	ath79_register_usb();

	ath79_register_nfc();

	ath79_register_wmac(art + WDR7500_WMAC_CALDATA_OFFSET, NULL);

	ath79_setup_qca955x_eth_cfg(QCA955X_ETH_CFG_RGMII_EN);

	ath79_register_mdio(0, 0x0);

	ath79_init_mac(ath79_eth0_data.mac_addr, art + WDR7500_MAC0_OFFSET, 0);

	mdiobus_register_board_info(wdr7500_mdio0_info,
				    ARRAY_SIZE(wdr7500_mdio0_info));

	/* GMAC0 is connected to the RMGII interface */
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_RGMII;
	ath79_eth0_data.phy_mask = BIT(0);
	ath79_eth0_data.mii_bus_dev = &ath79_mdio0_device.dev;
	ath79_eth0_pll_data.pll_1000 = 0x56000000;

	ath79_register_eth(0);

	/* GMAC1 is connected to the SGMII interface */
	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_SGMII;
	ath79_eth1_data.speed = SPEED_1000;
	ath79_eth1_data.duplex = DUPLEX_FULL;
	ath79_eth1_pll_data.pll_1000 = 0x03000101;

	ath79_register_eth(1);

	ath79_register_pci();
}

MIPS_MACHINE(ATH79_MACH_TL_WDR7500, "TL-WDR7500",
	     "TP-LINK TL-WDR7500", wdr7500_setup);
