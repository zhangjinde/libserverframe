/*
 * Copyright (c) 2020 YuQing <384681@qq.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "sf_configs.h"

#define DEFAULT_RETRY_MAX_INTERVAL_MS      5000
#define DEFAULT_CONNECT_RETRY_TIMES          10
#define DEFAULT_CONNECT_RETRY_INTERVAL_MS   100
#define DEFAULT_NETWORK_RETRY_TIMES          10
#define DEFAULT_NETWORK_RETRY_INTERVAL_MS   100

int sf_load_net_retry_config(SFNetRetryConfig *net_retry_cfg,
        IniFullContext *ini_ctx)
{
    char *retry_interval_mode;
    retry_interval_mode = iniGetStrValueEx(ini_ctx->section_name,
            "retry_interval_mode", ini_ctx->context, true);
    if (retry_interval_mode == NULL || *retry_interval_mode == '\0') {
        net_retry_cfg->interval_mm.mode =
            sf_net_retry_interval_mode_multiple;
    } else if (strncasecmp(retry_interval_mode, "fixed", 5) == 0) {
        net_retry_cfg->interval_mm.mode =
            sf_net_retry_interval_mode_fixed;
    } else if (strncasecmp(retry_interval_mode, "multi", 5) == 0) {
        net_retry_cfg->interval_mm.mode =
            sf_net_retry_interval_mode_multiple;
    } else {
        logWarning("file: "__FILE__", line: %d, "
                "config file: %s, unkown retry_interval_mode: %s, "
                "set to multiple", __LINE__, ini_ctx->filename,
                retry_interval_mode);
        net_retry_cfg->interval_mm.mode =
            sf_net_retry_interval_mode_multiple;
    }

    net_retry_cfg->interval_mm.max_interval_ms = iniGetIntValueEx(
            ini_ctx->section_name, "retry_max_interval_ms",
            ini_ctx->context, DEFAULT_RETRY_MAX_INTERVAL_MS, true);

    net_retry_cfg->connect.times = iniGetIntValueEx(
            ini_ctx->section_name, "connect_retry_times",
            ini_ctx->context, DEFAULT_CONNECT_RETRY_TIMES, true);

    net_retry_cfg->connect.interval_ms = iniGetIntValueEx(
            ini_ctx->section_name, "connect_retry_interval_ms",
            ini_ctx->context, DEFAULT_CONNECT_RETRY_INTERVAL_MS, true);

    net_retry_cfg->network.times = iniGetIntValueEx(
            ini_ctx->section_name, "network_retry_times",
            ini_ctx->context, DEFAULT_NETWORK_RETRY_TIMES, true);

    net_retry_cfg->network.interval_ms = iniGetIntValueEx(
            ini_ctx->section_name, "network_retry_interval_ms",
            ini_ctx->context, DEFAULT_NETWORK_RETRY_INTERVAL_MS, true);

    return 0;
}

void sf_net_retry_config_to_string(SFNetRetryConfig *net_retry_cfg,
        char *output, const int size)
{
    snprintf(output, size, "retry_interval_mode=%s, "
            "retry_max_interval_ms=%d ms, connect_retry_times=%d, "
            "connect_retry_interval_ms=%d ms, network_retry_times=%d, "
            "network_retry_interval_ms=%d ms",
            (net_retry_cfg->interval_mm.mode ==
             sf_net_retry_interval_mode_fixed ? "fixed" : "multiple"),
            net_retry_cfg->interval_mm.max_interval_ms,
            net_retry_cfg->connect.times,
            net_retry_cfg->connect.interval_ms,
            net_retry_cfg->network.times,
            net_retry_cfg->network.interval_ms);
}

void sf_load_read_rule_config_ex(SFDataReadRule *rule,
        IniFullContext *ini_ctx, const SFDataReadRule def_rule)
{
    char *read_rule;
    read_rule = iniGetStrValueEx(ini_ctx->section_name,
            "read_rule", ini_ctx->context, true);
    if (read_rule == NULL || *read_rule == '\0') {
        *rule = def_rule;
    } else if (strncasecmp(read_rule, "any", 3) == 0) {
        *rule = sf_data_read_rule_any_available;
    } else if (strncasecmp(read_rule, "slave", 5) == 0) {
        *rule = sf_data_read_rule_slave_first;
    } else if (strncasecmp(read_rule, "master", 6) == 0) {
        *rule = sf_data_read_rule_master_only;
    } else {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, unkown read_rule: %s, set to any",
                __LINE__, ini_ctx->filename, read_rule);
        *rule = sf_data_read_rule_any_available;
    }
}
