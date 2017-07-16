/* liblxcapi
 *
 * Copyright © 2017 Christian Brauner <christian.brauner@ubuntu.com>.
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <lxc/lxccontainer.h>

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "lxc/state.h"
#include "lxctest.h"

#define MYNAME "lxctest1"

static int set_clear_save_and_load(struct lxc_container *c, const char *key,
				   const char *value, const char *config_file)
{
	char retval[4096] = {0};
	int ret;

	if (!c->set_config_item(c, key, value)) {
		lxc_error("failed to set config item \"%s\" to \"%s\"\n", key,
			  value);
		return -1;
	}

	ret = c->get_config_item(c, key, retval, sizeof(retval));
	if (ret < 0) {
		lxc_error("failed to get config item \"%s\"\n", key);
		return -1;
	}

	if (config_file) {
		if (!c->save_config(c, config_file)) {
			lxc_error("%s\n", "failed to save config file");
			return -1;
		}

		c->clear_config(c);
		c->lxc_conf = NULL;

		if (!c->load_config(c, config_file)) {
			lxc_error("%s\n", "failed to load config file");
			return -1;
		}
	}

	if (!c->clear_config_item(c, key)) {
		lxc_error("failed to clear config item \"%s\"\n", key);
		return -1;
	}

	if (config_file)  {
		if (!c->save_config(c, config_file)) {
			lxc_error("%s\n", "failed to save config file");
			return -1;
		}

		c->clear_config(c);
		c->lxc_conf = NULL;

		if (!c->load_config(c, config_file)) {
			lxc_error("%s\n", "failed to load config file");
			return -1;
		}
	}

	c->clear_config(c);
	c->lxc_conf = NULL;

	return 0;
}

int main(int argc, char *argv[])
{
	struct lxc_container *c;
	int fd = -1;
	int ret = EXIT_FAILURE;
	char tmpf[] = "lxc-parse-config-file-XXXXXX";
	char retval[4096] = {0};

	c = lxc_container_new("lxc-parse-config-file-testxyz", NULL);
	if (!c) {
		lxc_error("%s\n", "failed to create new container");
		exit(EXIT_FAILURE);
	}

	fd = mkstemp(tmpf);
	if (fd < 0) {
		lxc_error("%s\n", "Could not create temporary file");
		goto non_test_error;
	}
	close(fd);

	/* lxc.arch */
	if (set_clear_save_and_load(c, "lxc.arch", "x86_64", tmpf) < 0) {
		lxc_error("%s\n", "lxc.arch");
		goto non_test_error;
	}

	/* lxc.pts */
	if (set_clear_save_and_load(c, "lxc.pts", "1000", tmpf) < 0) {
		lxc_error("%s\n", "lxc.pts");
		goto non_test_error;
	}

	/* lxc.tty */
	if (set_clear_save_and_load(c, "lxc.tty", "4", tmpf) < 0) {
		lxc_error("%s\n", "lxc.tty");
		goto non_test_error;
	}

	/* lxc.devttydir */
	if (set_clear_save_and_load(c, "lxc.devttydir", "not-dev", tmpf) < 0) {
		lxc_error("%s\n", "lxc.devttydir");
		goto non_test_error;
	}

	/* lxc.kmsg */
	if (set_clear_save_and_load(c, "lxc.kmsg", "1", tmpf) < 0) {
		lxc_error("%s\n", "lxc.kmsg");
		goto non_test_error;
	}

	/* lxc.aa_profile */
	if (set_clear_save_and_load(c, "lxc.aa_profile", "unconfined", tmpf) <
	    0) {
		lxc_error("%s\n", "lxc.aa_profile");
		goto non_test_error;
	}

	/* lxc.aa_allow_incomplete */
	if (set_clear_save_and_load(c, "lxc.aa_allow_incomplete", "1", tmpf) < 0) {
		lxc_error("%s\n", "lxc.aa_allow_incomplete");
		goto non_test_error;
	}

	/* lxc.cgroup.cpuset.cpus */
	if (set_clear_save_and_load(c, "lxc.cgroup.cpuset.cpus", "1-100",
				    tmpf) < 0) {
		lxc_error("%s\n", "lxc.cgroup.cpuset.cpus");
		goto non_test_error;
	}

	/* lxc.cgroup */
	if (!c->set_config_item(c, "lxc.cgroup.cpuset.cpus", "1-100")) {
		lxc_error("%s\n", "failed to set config item "
				  "\"lxc.cgroup.cpuset.cpus\" to \"1-100\"");
		return -1;
	}

	if (!c->set_config_item(c, "lxc.cgroup.memory.limit_in_bytes",
				"123456789")) {
		lxc_error(
		    "%s\n",
		    "failed to set config item "
		    "\"lxc.cgroup.memory.limit_in_bytes\" to \"123456789\"");
		return -1;
	}

	if (!c->get_config_item(c, "lxc.cgroup", retval, sizeof(retval))) {
		lxc_error("%s\n", "failed to get config item \"lxc.cgroup\"");
		return -1;
	}

	c->clear_config(c);
	c->lxc_conf = NULL;

	/* lxc.id_map
	 * We can't really save the config here since save_config() wants to
	 * chown the container's directory but we haven't created an on-disk
	 * container. So let's test set-get-clear.
	 */
	if (set_clear_save_and_load(c, "lxc.id_map", "u 0 100000 1000000000",
				    NULL) < 0) {
		lxc_error("%s\n", "lxc.id_map");
		goto non_test_error;
	}

	if (!c->set_config_item(c, "lxc.id_map", "u 1 100000 10000000")) {
		lxc_error("%s\n", "failed to set config item "
				  "\"lxc.id_map\" to \"u 1 100000 10000000\"");
		return -1;
	}

	if (!c->set_config_item(c, "lxc.id_map", "g 1 100000 10000000")) {
		lxc_error("%s\n", "failed to set config item "
				  "\"lxc.id_map\" to \"g 1 100000 10000000\"");
		return -1;
	}

	if (!c->get_config_item(c, "lxc.id_map", retval, sizeof(retval))) {
		lxc_error("%s\n", "failed to get config item \"lxc.cgroup\"");
		return -1;
	}

	c->clear_config(c);
	c->lxc_conf = NULL;

	/* lxc.loglevel */
	if (set_clear_save_and_load(c, "lxc.loglevel", "debug", tmpf) < 0) {
		lxc_error("%s\n", "lxc.loglevel");
		goto non_test_error;
	}

	/* lxc.logfile */
	if (set_clear_save_and_load(c, "lxc.logfile", "/some/path", tmpf) < 0) {
		lxc_error("%s\n", "lxc.logfile");
		goto non_test_error;
	}

	/* lxc.mount */
	if (set_clear_save_and_load(c, "lxc.mount", "/some/path", NULL) < 0) {
		lxc_error("%s\n", "lxc.mount");
		goto non_test_error;
	}

	/* lxc.mount.auto */
	if (set_clear_save_and_load(c, "lxc.mount.auto", "proc:rw sys:rw cgroup-full:rw", tmpf) < 0) {
		lxc_error("%s\n", "lxc.mount.auto");
		goto non_test_error;
	}

	/* lxc.mount.entry */
	if (set_clear_save_and_load(
		c, "lxc.mount.entry",
		"/dev/dri dev/dri none bind,optional,create=dir", tmpf) < 0) {
		lxc_error("%s\n", "lxc.mount.entry");
		goto non_test_error;
	}

	/* lxc.rootfs */
	if (set_clear_save_and_load(c, "lxc.rootfs", "/some/path", tmpf) < 0) {
		lxc_error("%s\n", "lxc.rootfs");
		goto non_test_error;
	}

	/* lxc.rootfs.mount */
	if (set_clear_save_and_load(c, "lxc.rootfs.mount", "/some/path", tmpf) < 0) {
		lxc_error("%s\n", "lxc.rootfs.mount");
		goto non_test_error;
	}

	/* lxc.rootfs.options */
	if (set_clear_save_and_load(c, "lxc.rootfs.options", "ext4,discard", tmpf) < 0) {
		lxc_error("%s\n", "lxc.rootfs.options");
		goto non_test_error;
	}

	/* lxc.rootfs.backend */
	if (set_clear_save_and_load(c, "lxc.rootfs.backend", "btrfs", tmpf) < 0) {
		lxc_error("%s\n", "lxc.rootfs.backend");
		goto non_test_error;
	}

	/* lxc.utsname */
	if (set_clear_save_and_load(c, "lxc.utsname", "the-shire", tmpf) < 0) {
		lxc_error("%s\n", "lxc.utsname");
		goto non_test_error;
	}

	/* lxc.hook.pre-start */
	if (set_clear_save_and_load(c, "lxc.hook.pre-start", "/some/pre-start", tmpf) < 0) {
		lxc_error("%s\n", "lxc.hook.pre-start");
		goto non_test_error;
	}

	/* lxc.hook.pre-mount */
	if (set_clear_save_and_load(c, "lxc.hook.pre-mount", "/some/pre-mount", tmpf) < 0) {
		lxc_error("%s\n", "lxc.hook.pre-mount");
		goto non_test_error;
	}

	/* lxc.hook.mount */
	if (set_clear_save_and_load(c, "lxc.hook.mount", "/some/mount", tmpf) < 0) {
		lxc_error("%s\n", "lxc.hook.mount");
		goto non_test_error;
	}

	/* lxc.hook.autodev */
	if (set_clear_save_and_load(c, "lxc.hook.autodev", "/some/autodev", tmpf) < 0) {
		lxc_error("%s\n", "lxc.hook.autodev");
		goto non_test_error;
	}

	/* lxc.hook.start */
	if (set_clear_save_and_load(c, "lxc.hook.start", "/some/start", tmpf) < 0) {
		lxc_error("%s\n", "lxc.hook.start");
		goto non_test_error;
	}

	/* lxc.hook.stop */
	if (set_clear_save_and_load(c, "lxc.hook.stop", "/some/stop", tmpf) < 0) {
		lxc_error("%s\n", "lxc.hook.stop");
		goto non_test_error;
	}

	/* lxc.hook.post-stop */
	if (set_clear_save_and_load(c, "lxc.hook.post-stop", "/some/post-stop", tmpf) < 0) {
		lxc_error("%s\n", "lxc.hook.post-stop");
		goto non_test_error;
	}

	/* lxc.hook.clone */
	if (set_clear_save_and_load(c, "lxc.hook.clone", "/some/clone", tmpf) < 0) {
		lxc_error("%s\n", "lxc.hook.clone");
		goto non_test_error;
	}

	/* lxc.hook.destroy */
	if (set_clear_save_and_load(c, "lxc.hook.destroy", "/some/destroy", tmpf) < 0) {
		lxc_error("%s\n", "lxc.hook.destroy");
		goto non_test_error;
	}

	/* lxc.cap.drop */
	if (set_clear_save_and_load(c, "lxc.cap.drop", "sys_module mknod setuid net_raw", tmpf) < 0) {
		lxc_error("%s\n", "lxc.cap.drop");
		goto non_test_error;
	}

	/* lxc.cap.keep */
	if (set_clear_save_and_load(c, "lxc.cap.keep", "sys_module mknod setuid net_raw", tmpf) < 0) {
		lxc_error("%s\n", "lxc.cap.keep");
		goto non_test_error;
	}

	/* lxc.console */
	if (set_clear_save_and_load(c, "lxc.console", "none", tmpf) < 0) {
		lxc_error("%s\n", "lxc.console");
		goto non_test_error;
	}

	/* lxc.console.logfile */
	if (set_clear_save_and_load(c, "lxc.console.logfile", "/some/logfile", tmpf) < 0) {
		lxc_error("%s\n", "lxc.console.logfile");
		goto non_test_error;
	}

	/* lxc.seccomp */
	if (set_clear_save_and_load(c, "lxc.seccomp", "/some/seccomp/file", tmpf) < 0) {
		lxc_error("%s\n", "lxc.seccomp");
		goto non_test_error;
	}

	/* lxc.autodev */
	if (set_clear_save_and_load(c, "lxc.autodev", "1", tmpf) < 0) {
		lxc_error("%s\n", "lxc.autodev");
		goto non_test_error;
	}

	/* lxc.haltsignal */
	if (set_clear_save_and_load(c, "lxc.haltsignal", "1", tmpf) < 0) {
		lxc_error("%s\n", "lxc.haltsignal");
		goto non_test_error;
	}

	/* lxc.rebootsignal */
	if (set_clear_save_and_load(c, "lxc.rebootsignal", "1", tmpf) < 0) {
		lxc_error("%s\n", "lxc.rebootsignal");
		goto non_test_error;
	}

	/* lxc.stopsignal */
	if (set_clear_save_and_load(c, "lxc.stopsignal", "1", tmpf) < 0) {
		lxc_error("%s\n", "lxc.stopsignal");
		goto non_test_error;
	}

	/* lxc.start.auto */
	if (set_clear_save_and_load(c, "lxc.start.auto", "1", tmpf) < 0) {
		lxc_error("%s\n", "lxc.start.auto");
		goto non_test_error;
	}

	/* lxc.start.delay */
	if (set_clear_save_and_load(c, "lxc.start.delay", "5", tmpf) < 0) {
		lxc_error("%s\n", "lxc.start.delay");
		goto non_test_error;
	}

	/* lxc.start.order */
	if (set_clear_save_and_load(c, "lxc.start.order", "1", tmpf) < 0) {
		lxc_error("%s\n", "lxc.start.order");
		goto non_test_error;
	}

	/* lxc.utsname */
	if (set_clear_save_and_load(c, "lxc.utsname", "get-schwifty", tmpf) <
	    0) {
		lxc_error("%s\n", "lxc.utsname");
		goto non_test_error;
	}

	/* lxc.monitor.unshare */
	if (set_clear_save_and_load(c, "lxc.monitor.unshare", "1", tmpf) < 0) {
		lxc_error("%s\n", "lxc.monitor.unshare");
		goto non_test_error;
	}

	/* lxc.group */
	if (set_clear_save_and_load(c, "lxc.group", "some,container,groups", tmpf) < 0) {
		lxc_error("%s\n", "lxc.group");
		goto non_test_error;
	}

	/* lxc.environment */
	if (set_clear_save_and_load(c, "lxc.environment", "FOO=BAR", tmpf) < 0) {
		lxc_error("%s\n", "lxc.environment");
		goto non_test_error;
	}

	/* lxc.init_cmd */
	if (set_clear_save_and_load(c, "lxc.init_cmd", "/bin/bash", tmpf) < 0) {
		lxc_error("%s\n", "lxc.init_cmd");
		goto non_test_error;
	}

	/* lxc.init_uid */
	if (set_clear_save_and_load(c, "lxc.init_uid", "1000", tmpf) < 0) {
		lxc_error("%s\n", "lxc.init_uid");
		goto non_test_error;
	}

	/* lxc.init_gid */
	if (set_clear_save_and_load(c, "lxc.init_gid", "1000", tmpf) < 0) {
		lxc_error("%s\n", "lxc.init_gid");
		goto non_test_error;
	}

	/* lxc.ephemeral */
	if (set_clear_save_and_load(c, "lxc.ephemeral", "1", tmpf) < 0) {
		lxc_error("%s\n", "lxc.ephemeral");
		goto non_test_error;
	}

	ret = EXIT_SUCCESS;
non_test_error:
	c->destroy(c);
	lxc_container_put(c);
	exit(ret);
}
