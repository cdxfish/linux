// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019 Facebook */
#include <test_progs.h>
#include "test_pkt_access.skel.h"
#include "fentry_test.skel.h"
#include "fexit_test.skel.h"

void test_fentry_fexit(void)
{
	struct test_pkt_access *pkt_skel = NULL;
	struct fentry_test *fentry_skel = NULL;
	struct fexit_test *fexit_skel = NULL;
	__u64 *fentry_res, *fexit_res;
	__u32 duration = 0, retval;
	int err, pkt_fd, i;

	pkt_skel = test_pkt_access__open_and_load();
	if (CHECK(!pkt_skel, "pkt_skel_load", "pkt_access skeleton failed\n"))
		return;
	fentry_skel = fentry_test__open_and_load();
	if (CHECK(!fentry_skel, "fentry_skel_load", "fentry skeleton failed\n"))
		goto close_prog;
	fexit_skel = fexit_test__open_and_load();
	if (CHECK(!fexit_skel, "fexit_skel_load", "fexit skeleton failed\n"))
		goto close_prog;

	err = fentry_test__attach(fentry_skel);
	if (CHECK(err, "fentry_attach", "fentry attach failed: %d\n", err))
		goto close_prog;
	err = fexit_test__attach(fexit_skel);
	if (CHECK(err, "fexit_attach", "fexit attach failed: %d\n", err))
		goto close_prog;

	pkt_fd = bpf_program__fd(pkt_skel->progs.test_pkt_access);
	err = bpf_prog_test_run(pkt_fd, 1, &pkt_v6, sizeof(pkt_v6),
				NULL, NULL, &retval, &duration);
	CHECK(err || retval, "ipv6",
	      "err %d errno %d retval %d duration %d\n",
	      err, errno, retval, duration);

	fentry_res = (__u64 *)fentry_skel->bss;
	fexit_res = (__u64 *)fexit_skel->bss;
	printf("%lld\n", fentry_skel->bss->test1_result);
	for (i = 0; i < 6; i++) {
		CHECK(fentry_res[i] != 1, "result",
		      "fentry_test%d failed err %lld\n", i + 1, fentry_res[i]);
		CHECK(fexit_res[i] != 1, "result",
		      "fexit_test%d failed err %lld\n", i + 1, fexit_res[i]);
	}

close_prog:
	test_pkt_access__destroy(pkt_skel);
	fentry_test__destroy(fentry_skel);
	fexit_test__destroy(fexit_skel);
}
