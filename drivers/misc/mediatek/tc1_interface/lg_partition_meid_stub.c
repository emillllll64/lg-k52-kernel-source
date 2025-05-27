// SPDX-License-Identifier: GPL-2.0
#include <linux/kernel.h>
#include "lg_partition.h"

/*
 * Stub for MEID support.
 * MTK MT6765 devices are GSM/LTE only, so simply tell the caller that the
 * operation is unsupported.
 */
unsigned char LGE_FacReadMeid(unsigned char idx, unsigned char *meid)
{
        pr_info("%s: MEID not supported on this platform (idx=%u)\n",
                __func__, idx);

        /* If callers treat non-zero as failure you can return 1 instead. */
        return 0;          /* 0 = success, change to 1 for “fail” */
}
EXPORT_SYMBOL_GPL(LGE_FacReadMeid);
