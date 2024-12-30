// SPDX-License-Identifier: GPL-2.0-only
	set_dload_mode(dload_type);
	mutex_unlock(&tcsr_lock);
	return count;
}
#endif /* CONFIG_QCOM_MINIDUMP */

void msm_set_restart_mode(int mode)
{
	restart_mode = mode;
}
EXPORT_SYMBOL(msm_set_restart_mode);


static void msm_restart_prepare(const char *cmd)
{
	bool need_warm_reset = false;
	u8 reason = PON_RESTART_REASON_UNKNOWN;
	/* Write download mode flags if we're panic'ing
	 * Write download mode flags if restart_mode says so
	 * Kill download mode if master-kill switch is set
	 */

	if (cmd != NULL && !strcmp(cmd, "qcom_dload"))
		restart_mode = RESTART_DLOAD;

	set_dload_mode(download_mode &&
			(in_panic || restart_mode == RESTART_DLOAD));

	if (qpnp_pon_check_hard_reset_stored()) {
		/* Set warm reset as true when device is in dload mode */
		if (get_dload_mode() ||
			((cmd != NULL && cmd[0] != '\0') &&
			!strcmp(cmd, "edl")))
			need_warm_reset = true;
	} else {
		need_warm_reset = (get_dload_mode() ||
				(cmd != NULL && cmd[0] != '\0'));
	}

	if (force_warm_reboot)
		pr_info("Forcing a warm reset of the system\n");

	/* Hard reset the PMIC unless memory contents must be maintained. */
	if (true)
		qpnp_pon_system_pwr_off(PON_POWER_OFF_WARM_RESET);
	else
		qpnp_pon_system_pwr_off(PON_POWER_OFF_HARD_RESET);
	
	qpnp_pon_set_restart_reason(
		PON_RESTART_REASON_RECOVERY);
	__raw_writel(0x77665502, restart_reason);
	flush_cache_all();
	return;

	if (cmd != NULL) {
		if (!strncmp(cmd, "bootloader", 10)) {
			reason = PON_RESTART_REASON_BOOTLOADER;
			__raw_writel(0x77665500, restart_reason);
		} else if (!strncmp(cmd, "recovery", 8)) {
			reason = PON_RESTART_REASON_RECOVERY;
			__raw_writel(0x77665502, restart_reason);
		} else if (!strcmp(cmd, "rtc")) {
			reason = PON_RESTART_REASON_RTC;
			__raw_writel(0x77665503, restart_reason);
		} else if (!strcmp(cmd, "dm-verity device corrupted")) {
			reason = PON_RESTART_REASON_DMVERITY_CORRUPTED;
			__raw_writel(0x77665508, restart_reason);
		} else if (!strcmp(cmd, "dm-verity enforcing")) {
			reason = PON_RESTART_REASON_DMVERITY_ENFORCE;
			__raw_writel(0x77665509, restart_reason);
		} else if (!strcmp(cmd, "keys clear")) {
			reason = PON_RESTART_REASON_KEYS_CLEAR;
			__raw_writel(0x7766550a, restart_reason);
		} else if (!strncmp(cmd, "oem-", 4)) {
			unsigned long code;
			int ret;

			ret = kstrtoul(cmd + 4, 16, &code);
			if (!ret)
				__raw_writel(0x6f656d00 | (code & 0xff),
					     restart_reason);
		} else if (!strncmp(cmd, "edl", 3)) {
			enable_emergency_dload_mode();
		} else {
			__raw_writel(0x77665501, restart_reason);
		}

		if (reason && nvmem_cell)
			nvmem_cell_write(nvmem_cell, &reason, sizeof(reason));
		else
			qpnp_pon_set_restart_reason(
				(enum pon_restart_reason)reason);
	}

	/*outer_flush_all is not supported by 64bit kernel*/
#ifndef CONFIG_ARM64

