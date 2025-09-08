#include "sdhci-msm.h"
#include <linux/module.h>

#define QOS_REMOVE_DELAY_MS	10

static int sdhci_msm_get_cpu_group(struct sdhci_msm_host *msm_host, int cpu)
{
	int i;
	struct sdhci_msm_cpu_group_map *map =
			&msm_host->pdata->pm_qos_data.cpu_group_map;

	if (cpu < 0)
		goto not_found;

	for (i = 0; i < map->nr_groups; i++)
		if (cpumask_test_cpu(cpu, &map->mask[i]))
			return i;

not_found:
	return -EINVAL;
}

void sdhci_msm_pm_qos_irq_vote(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_msm_host *msm_host = pltfm_host->priv;
	struct sdhci_msm_pm_qos_latency *latency =
		&msm_host->pdata->pm_qos_data.irq_latency;
	int counter;

	if (!msm_host->pm_qos_irq.enabled)
		return;
	if (host->power_policy > SDHCI_POWER_SAVE_MODE)
		return;

	counter = atomic_inc_return(&msm_host->pm_qos_irq.counter);
	/* Make sure to update the voting in case power policy has changed */
	if (msm_host->pm_qos_irq.latency == latency->latency[host->power_policy]
		&& counter > 1)
		return;

	cancel_delayed_work_sync(&msm_host->pm_qos_irq.unvote_work);
	msm_host->pm_qos_irq.latency = latency->latency[host->power_policy];
	pm_qos_update_request(&msm_host->pm_qos_irq.req,
				msm_host->pm_qos_irq.latency);
}
EXPORT_SYMBOL(sdhci_msm_pm_qos_irq_vote);

void sdhci_msm_pm_qos_cpu_vote(struct sdhci_host *host,
		struct sdhci_msm_pm_qos_latency *latency, int cpu)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_msm_host *msm_host = pltfm_host->priv;
	int group = sdhci_msm_get_cpu_group(msm_host, cpu);
	struct sdhci_msm_pm_qos_group *pm_qos_group;
	int counter;

	if (!msm_host->pm_qos_group_enable || group < 0)
		return;

	pm_qos_group = &msm_host->pm_qos[group];
	counter = atomic_inc_return(&pm_qos_group->counter);

	/* Make sure to update the voting in case power policy has changed */
	if (pm_qos_group->latency == latency->latency[host->power_policy]
		&& counter > 1)
		return;

	cancel_delayed_work_sync(&pm_qos_group->unvote_work);

	pm_qos_group->latency = latency->latency[host->power_policy];
	pm_qos_update_request(&pm_qos_group->req, pm_qos_group->latency);
}
EXPORT_SYMBOL(sdhci_msm_pm_qos_cpu_vote);

void sdhci_msm_pm_qos_irq_unvote(struct sdhci_host *host, bool async)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_msm_host *msm_host = pltfm_host->priv;
	int counter;

	if (!msm_host->pm_qos_irq.enabled)
		return;

	if (atomic_read(&msm_host->pm_qos_irq.counter)) {
		counter = atomic_dec_return(&msm_host->pm_qos_irq.counter);
	} else {
		WARN(1, "attempt to decrement pm_qos_irq.counter when it's 0");
		return;
	}

	if (counter)
		return;

	if (async) {
		queue_delayed_work(msm_host->pm_qos_wq,
				&msm_host->pm_qos_irq.unvote_work,
				msecs_to_jiffies(QOS_REMOVE_DELAY_MS));
		return;
	}

	msm_host->pm_qos_irq.latency = PM_QOS_DEFAULT_VALUE;
	pm_qos_update_request(&msm_host->pm_qos_irq.req,
			msm_host->pm_qos_irq.latency);
}
EXPORT_SYMBOL(sdhci_msm_pm_qos_irq_unvote);

bool sdhci_msm_pm_qos_cpu_unvote(struct sdhci_host *host, int cpu, bool async)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_msm_host *msm_host = pltfm_host->priv;
	int group = sdhci_msm_get_cpu_group(msm_host, cpu);

	if (!msm_host->pm_qos_group_enable || group < 0 ||
		atomic_dec_return(&msm_host->pm_qos[group].counter))
		return false;

	if (async) {
		queue_delayed_work(msm_host->pm_qos_wq,
				&msm_host->pm_qos[group].unvote_work,
				msecs_to_jiffies(QOS_REMOVE_DELAY_MS));
		return true;
	}

	msm_host->pm_qos[group].latency = PM_QOS_DEFAULT_VALUE;
	pm_qos_update_request(&msm_host->pm_qos[group].req,
				msm_host->pm_qos[group].latency);
	return true;
}
EXPORT_SYMBOL(sdhci_msm_pm_qos_cpu_unvote);
