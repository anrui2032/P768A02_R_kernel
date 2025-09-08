/*
 * platform indepent driver interface
 *
 * Coypritht (c) 2017 Goodix
 */
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/err.h>

#include "gf_spi.h"

#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif

int gf_parse_dts(struct gf_dev *gf_dev)
{
	int ret = 0;
	struct device *dev = &gf_dev->spi->dev;
	struct device_node *np = dev->of_node;

	gf_dev->pwr_gpio = of_get_named_gpio(np, "fp-gpio-pwr", 0);
	if (gf_dev->pwr_gpio < 0) {
		pr_err("failed to get pwr gpio!\n");
		ret = gf_dev->pwr_gpio;
		goto err;
	}

	pr_info("gf_dev->pwr_gpio, gpio = %d\n", gf_dev->pwr_gpio);
	ret = gpio_request(gf_dev->pwr_gpio, "goodix_pwr");
	if (ret < 0) {
		pr_err("pwr gpio request error! ");
		goto err;
	}

	ret = gpio_direction_output(gf_dev->pwr_gpio, 1);
	if (ret < 0) {
		pr_err("pwr gpio set output error! ");
		goto err_pwr;
	}

	usleep_range(15000, 15100);

	gf_dev->reset_gpio = of_get_named_gpio(np, "fp-gpio-reset", 0);
	if (gf_dev->reset_gpio < 0) {
		pr_err("failed to get reset gpio!\n");
		ret = gf_dev->reset_gpio;
		goto err_pwr;
	}

	pr_info("gf_dev->reset_gpio, gpio = %d\n", gf_dev->reset_gpio);
	ret = gpio_request(gf_dev->reset_gpio, "goodix_reset");
	if (ret) {
		pr_err("failed to request reset gpio, rc = %d\n", ret);
		goto err_pwr;
	}

	ret = gpio_direction_output(gf_dev->reset_gpio, 1);
	if (ret < 0) {
		pr_err("reset gpio set output error! ");
		goto err_reset;
	}

	gf_dev->irq_gpio = of_get_named_gpio(np, "fp-gpio-irq", 0);
	if (gf_dev->irq_gpio < 0) {
		pr_err("failed to get irq gpio!\n");
		ret = gf_dev->irq_gpio;
		goto err_reset;
	}

	pr_info("gf_dev->irq_gpio, gpio = %d\n", gf_dev->irq_gpio);
	ret = gpio_request(gf_dev->irq_gpio, "goodix_irq");
	if (ret) {
		pr_err("failed to request irq gpio, rc = %d\n", ret);
		goto err_reset;
	}

	ret = gpio_direction_input(gf_dev->irq_gpio);
	if (ret < 0) {
		pr_err("reset gpio set output error! ");
		goto err_irq;
	}
	return ret;

err_irq:
	gpio_free(gf_dev->irq_gpio);
err_reset:
	gpio_free(gf_dev->reset_gpio);
err_pwr:
	gpio_free(gf_dev->pwr_gpio);

err:
	return ret;
}

void gf_cleanup(struct gf_dev *gf_dev)
{
	pr_info("[gf info] %s\n", __func__);

	if (gpio_is_valid(gf_dev->irq_gpio)) {
		gpio_free(gf_dev->irq_gpio);
		pr_info("gf remove irq_gpio success\n");
	}
	if (gpio_is_valid(gf_dev->reset_gpio)) {
		gpio_free(gf_dev->reset_gpio);
		pr_info("gf remove reset_gpio success\n");
	}
	if (gpio_is_valid(gf_dev->pwr_gpio)) {
		gpio_free(gf_dev->pwr_gpio);
		pr_info("gf remove pwr_gpio success\n");
	}
}

int gf_power_on(struct gf_dev *gf_dev)
{
	int rc = 0;

	/* TODO: add your power control here */
	if (gpio_is_valid(gf_dev->pwr_gpio)) {
		gpio_set_value(gf_dev->pwr_gpio, 1);
		msleep(20);
		pr_info("----gf power on ok ----\n");
	}
	return rc;
}

int gf_power_off(struct gf_dev *gf_dev)
{
	int rc = 0;

	/* TODO: add your power control here */
	if (gpio_is_valid(gf_dev->pwr_gpio)) {
		gpio_set_value(gf_dev->pwr_gpio, 0);
		pr_info("----gf power off ok----\n");
	}
	return rc;
}

int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms)
{
	if (gf_dev == NULL) {
		pr_err("Input buff is NULL.\n");
		return -ENODEV;
	}
	pr_info("----gf hw reset----\n");
	if (gpio_is_valid(gf_dev->reset_gpio)) {
		/* gpio_direction_output(gf_dev->reset_gpio, 1); */
		gpio_set_value(gf_dev->reset_gpio, 0);
		msleep(20);
		gpio_set_value(gf_dev->reset_gpio, 1);
		msleep(delay_ms);
		pr_info("----gf hw reset ok----\n");
	}
	return 0;
}

int gf_irq_num(struct gf_dev *gf_dev)
{
	if (gf_dev == NULL) {
		pr_err("Input buff is NULL.\n");
		return -ENODEV;
	} else {
		return gpio_to_irq(gf_dev->irq_gpio);
	}
}
